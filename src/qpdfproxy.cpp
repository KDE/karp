// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "qpdfproxy.h"
#include "karp_debug.h"
#include "pdfeditmodel.h"
#include "pdffile.h"
#include "pdfmetadata.h"
#include <QDir>
#include <QStandardPaths>
#include <qpdf/QPDFUsage.hh>

using namespace Qt::Literals::StringLiterals;
using namespace std::string_literals;

QpdfProxy::QpdfProxy(PdfEditModel *pdfModel, QObject *parent)
    : QObject(parent)
    , m_pdfModel(pdfModel)
    , m_thread(new QThread)
{
    moveToThread(m_thread);
    connect(m_thread, &QThread::started, this, &QpdfProxy::threadSlot);
    connect(m_thread, &QThread::finished, this, &QpdfProxy::finished);
}

QpdfProxy::~QpdfProxy()
{
    m_thread->terminate();
    m_thread->deleteLater();
}

void QpdfProxy::doJob()
{
    m_thread->start();
}

void QpdfProxy::addMetaToJob(QPDF &qpdf, PdfMetaData *metaData)
{
    bool noMetaData = metaData->checkIsEmpty();
    auto trailer = qpdf.getTrailer();
    auto info = trailer.getKey("/Info"s);
    if (info.isNull()) {
        if (noMetaData)
            return;
        auto newStream = qpdf.newStream();
        auto newDict = QPDFObjectHandle::newDictionary();
        metaData->setAllInfoKeys(newDict);
        qpdf.replaceObject(newStream.getObjectID(), newStream.getGeneration(), newDict);
        trailer.replaceKey("/Info"s, newDict);
    } else {
        if (noMetaData) {
            trailer.removeKey("/Info"s);
        } else {
            auto infoObj = qpdf.getObject(info.getObjectID(), info.getObjGen().getGen());
            metaData->setAllInfoKeys(infoObj);
        }
    }
}

void QpdfProxy::forcePdfVersion(QPDFJob::Config *jobConf, qreal ver)
{
    jobConf->forceVersion("1."s + std::to_string(static_cast<int>(ver * 10.0 - 10.0)));
}

void QpdfProxy::setPdfPassword(QPDFJob::Config *jobConf, const std::string &pass)
{
    jobConf
        ->encrypt(256, pass, pass)
        // ->print("low")
        ->endEncrypt();
}

void QpdfProxy::threadSlot()
{
    auto &pdfs = m_pdfModel->pdfs();
    auto firstPdf = pdfs[m_pdfModel->page(0)->referenceFile()];

    QVector<QVector<quint16>> chunks;
    auto firstPage = m_pdfModel->page(0);
    int fromPage = firstPage->origPage();
    int fileId = firstPage->referenceFile();
    chunks << (QVector<quint16>() << fileId << fromPage);
    for (int p = 1; p < m_pdfModel->pageCount(); ++p) {
        const auto *const nextPage = m_pdfModel->page(p);
        if (nextPage->referenceFile() != fileId) {
            fileId = nextPage->referenceFile();
            fromPage = nextPage->origPage();
            chunks << (QVector<quint16>() << fileId << fromPage);
            continue;
        }
        chunks.last() << nextPage->origPage();
    }

    // Rotation of pages - aggregate angles
    // NOTICE: page number (r) refers to number in m_pageList not orig page number in PDF file
    QVector<quint16> r90, r180, r270;
    for (int r = 0; r < m_pdfModel->pageCount(); ++r) {
        const auto *const page = m_pdfModel->page(r);
        if (page->rotated() == 90)
            r90 << r;
        else if (page->rotated() == 180)
            r180 << r;
        else if (page->rotated() == 270)
            r270 << r;
    }

    QString tempOut = QDir::tempPath() + QDir::separator() + "karp_out.pdf"_L1;
    try {
        QPDFJob qpdfJob;
        auto jobConf = qpdfJob.config();
        jobConf->inputFile(firstPdf->filePath().toStdString())->outputFile(tempOut.toStdString());
        // --pages
        auto qpdfPages = jobConf->pages();
        for (int c = 0; c < chunks.count(); ++c) {
            appendRangeToJob(chunks[c], qpdfPages.get(), c == 0);
        }
        qpdfPages->endPages();
        // optimizations TODO
        if (m_pdfModel->optimizeImages()) {
            jobConf->recompressFlate();
        }
        // --rotate
        if (!r90.isEmpty())
            jobConf->rotate(getPagesForRotation(90, r90));
        if (!r180.isEmpty())
            jobConf->rotate(getPagesForRotation(180, r180));
        if (!r270.isEmpty())
            jobConf->rotate(getPagesForRotation(270, r270));
        // PDF version (if not reduce size)
        if (m_pdfModel->pdfVersion() > 0.0 && !m_pdfModel->reduceSize()) {
            forcePdfVersion(jobConf.get(), m_pdfModel->pdfVersion());
        }
        // PDF password (if not reduce size)
        if (!m_pdfModel->passKey().isEmpty() && !m_pdfModel->reduceSize()) {
            setPdfPassword(jobConf.get(), m_pdfModel->passKey().toStdString());
        }
        // ->objectStreams("generate")
        jobConf->checkConfiguration();

        // Meta data aka info
        if (!m_pdfModel->reduceSize()) {
            auto qpdfSP = qpdfJob.createQPDF();
            auto &qpdf = *qpdfSP;
            addMetaToJob(qpdf, m_pdfModel->metaData());
            m_pdfModel->saveBookmarks(qpdf);
            qpdfJob.writeQPDF(qpdf);
        } else {
            qpdfJob.run();
        }

    } catch (QPDFUsage &e) {
        qCDebug(KARP_LOG) << "[QpdfProxy]" << "configuration error: " << e.what();
        return;
    } catch (std::exception &e) {
        qCDebug(KARP_LOG) << "[QpdfProxy]" << "other error: " << e.what();
        return;
    }
    if (QFile::exists(tempOut)) {
        if (QFile::exists(m_pdfModel->outFile()))
            QFile::remove(m_pdfModel->outFile());
        QFile::copy(tempOut, m_pdfModel->outFile());
        QFile::remove(tempOut);
    }
    m_thread->quit();
}

void QpdfProxy::appendRangeToJob(const QVector<quint16> &range, QPDFJob::PagesConfig *qpdfPages, bool isFirst)
{
    PdfFile *pdf = nullptr;
    QString pRange;
    std::string file, pass;
    if (isFirst) {
        // qpdfPages->file("."); // only above qpdf 10.9.0 version
        file = "."; // for first file we referring to input file of QPDFJob "."
        pdf = m_pdfModel->pdfs()[m_pdfModel->page(0)->referenceFile()];
    } else {
        pdf = m_pdfModel->pdfs()[range.first()];
        // qpdfPages->file(pdf->filePath().toStdString());
        file = pdf->filePath().toStdString();
    }
    int fromPage = range[1];
    int toPage = fromPage + 1;
    pRange.append(QString::number(fromPage + 1));
    for (int p = 2; p < range.count(); ++p) {
        if (range[p] == toPage) {
            if (p == range.count() - 1) {
                int span = toPage - fromPage;
                if (span > 0)
                    pRange.append(QString(u"-%1"_s).arg(fromPage + span + 1));
            }
            toPage++;
        } else {
            int span = toPage - fromPage - 1;
            if (span > 0)
                pRange.append(QString(u"-%1"_s).arg(fromPage + span + 1));
            fromPage = range[p];
            toPage = fromPage + 1;
            pRange.append(u","_s);
            pRange.append(QString::number(fromPage + 1));
        }
    }
    // qpdfPages->range(pageRange.toStdString());
    if (!pdf->password().isEmpty())
        pass = pdf->password().toStdString();
    // qpdfPages->password(pdf->password().toStdString());
    qpdfPages->pageSpec(file, pRange.toStdString(), pass.data());
}

std::string QpdfProxy::getPagesForRotation(int angle, const QVector<quint16> &pageList)
{
    std::string pRange;
    if (!pageList.isEmpty()) {
        pRange = std::to_string(angle) + ":"s + std::to_string(pageList[0] + 1);
    }
    int p = 1;
    while (p < pageList.count()) {
        if (!pRange.empty())
            pRange += ","s;
        pRange += std::to_string(pageList[p] + 1); //.append(QString::number(pageList[p] + 1));
        p++;
    }
    return pRange;
}

#include "moc_qpdfproxy.cpp"
