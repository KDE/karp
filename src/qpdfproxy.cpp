// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "qpdfproxy.h"
#include "pdfeditmodel.h"
#include "pdffile.h"
#include <QDebug>
#include <qpdf/QPDFUsage.hh>

using namespace Qt::Literals::StringLiterals;

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
        auto nextPage = m_pdfModel->page(p);
        if (nextPage->referenceFile() != fileId) {
            fileId = nextPage->referenceFile();
            fromPage = nextPage->origPage();
            chunks << (QVector<quint16>() << fileId << fromPage);
            continue;
        }
        chunks.last() << nextPage->origPage();
    }
    // auto ranges = getPageRanges(chunks);
    // qDebug() << ranges;

    try {
        QPDFJob qpdfJob;
        auto jobConf = qpdfJob.config();
        jobConf->inputFile(firstPdf->filePath().toStdString())->outputFile(m_pdfModel->outFile().toStdString());
        auto qpdfPages = jobConf->pages();
        for (int c = 0; c < chunks.count(); ++c) {
            appendRangeToJob(chunks[c], qpdfPages.get(), c == 0);
        }
        qpdfPages->endPages();
        if (!m_pdfModel->passKey().isEmpty()) {
            jobConf
                ->encrypt(256, m_pdfModel->passKey().toStdString(), m_pdfModel->passKey().toStdString())
                // ->print("low")
                ->endEncrypt();
        }
        // ->objectStreams("generate")
        jobConf->checkConfiguration();
        qpdfJob.run();
    } catch (QPDFUsage &e) {
        qDebug() << "[QpdfProxy]" << "configuration error: " << e.what();
        return;
    } catch (std::exception &e) {
        qDebug() << "[QpdfProxy]" << "other error: " << e.what();
        return;
    }
    m_thread->quit();
}

void QpdfProxy::appendRangeToJob(const QVector<quint16> &range, QPDFJob::PagesConfig *qpdfPages, bool isFirst)
{
    PdfFile *pdf = nullptr;
    QString pageRange;
    std::string file, pass;
    if (isFirst) {
        // qpdfPages->file("."); // pnly above qpdf 10.9.0 version
        file = "."; // for first file we referring to input file of QPDFJob "."
        pdf = m_pdfModel->pdfs()[m_pdfModel->page(0)->referenceFile()];
    } else {
        pdf = m_pdfModel->pdfs()[range.first()];
        // qpdfPages->file(pdf->filePath().toStdString());
        file = pdf->filePath().toStdString();
    }
    int fromPage = range[1];
    int toPage = fromPage + 1;
    pageRange.append(QString::number(fromPage + 1));
    for (int p = 2; p < range.count(); ++p) {
        if (range[p] == toPage) {
            if (p == range.count() - 1) {
                int span = toPage - fromPage;
                if (span > 0)
                    pageRange.append(QString(u"-%1"_s).arg(fromPage + span + 1));
            }
            toPage++;
        } else {
            int span = toPage - fromPage - 1;
            if (span > 0)
                pageRange.append(QString(u"-%1"_s).arg(fromPage + span + 1));
            fromPage = range[p];
            toPage = fromPage + 1;
            pageRange.append(u","_s);
            pageRange.append(QString::number(fromPage + 1));
        }
    }
    // qpdfPages->range(pageRange.toStdString());
    if (!pdf->password().isEmpty())
        pass = pdf->password().toStdString();
    // qpdfPages->password(pdf->password().toStdString());
    qpdfPages->pageSpec(file, pageRange.toStdString(), pass.data());
}
