// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "toolsthread.h"
#include "karp_debug.h"
#include "karpconfig.h"
#include "pdfeditmodel.h"
#include "pdffile.h"
#include "qpdfproxy.h"
#include "version-karp.h"
#include <QDir>
#include <QFileInfo>
#include <qpdf/QPDFJob.hh>
#include <qpdf/QPDFUsage.hh>

using namespace Qt::Literals::StringLiterals;

GsThread::GsThread(int page)
    : QProcess()
    , m_pageNr(page)
{
    m_thread = new QThread();
    moveToThread(m_thread);
    setProgram(karpConfig::self()->gsPath());

    connect(
        m_thread,
        &QThread::started,
        this,
        [=] {
            start();
            waitForFinished();
            m_thread->quit();
        },
        Qt::DirectConnection);

    connect(
        m_thread,
        &QThread::finished,
        this,
        [=] {
            Q_EMIT gsFinished(this);
        },
        Qt::DirectConnection);
}

GsThread::~GsThread()
{
    m_thread->deleteLater();
}

void GsThread::doGS()
{
    m_thread->quit();
    m_thread->start();
}

// #################################################################################################
// ###################      class ToolsThread           ############################################
// #################################################################################################

#define TO_PS_ARGS_COUNT (14)
#define TO_PDF_ARGS_COUNT (8)
#define PAGENR_ARG_ID (10)
#define OUTFILE_ARG_ID (6)

ToolsThread *ToolsThread::m_self = nullptr;

ToolsThread::ToolsThread()
    : QThread()
{
    m_self = this;
}

ToolsThread::~ToolsThread()
{
}

void ToolsThread::lookForTools()
{
    m_mode = ToolsFindAll;
    start();
}

void ToolsThread::lookForGS(const QString &gsPath)
{
    m_pathArg = gsPath;
    m_mode = ToolsFindGS;
    start();
}

void ToolsThread::resizeByGs(const QString &filePath, int pages)
{
    m_pathArg = filePath;
    m_pageCountArg = pages;
    m_mode = ToolsResizeByGs;
    start();
}

void ToolsThread::cancel()
{
    m_doCancel = true;
}

QString ToolsThread::qpdfVersion() const
{
    return QString::fromStdString(QPDF::QPDFVersion());
}

const QString &ToolsThread::gsVersion() const
{
    return m_gsVersion;
}

void ToolsThread::run()
{
    if (m_mode == ToolsFindAll)
        findPdfTools();
    else if (m_mode == ToolsFindGS) {
        findGhostScript(m_pathArg);
        m_mode = ToolsIdle;
        Q_EMIT lookingDone();
    } else if (m_mode == ToolsResizeByGs) {
        resizeByGsThread();
    }
    m_doCancel = false;
}

void ToolsThread::findPdfTools()
{
    auto conf = karpConfig::self();
    conf->setGsPath(findGhostScript());

    m_mode = ToolsIdle;
    Q_EMIT lookingDone();
}

QString ToolsThread::findGhostScript(const QString &gsfPath)
{
    QProcess p;
#if defined(Q_OS_UNIX)
    p.setProgram(gsfPath.isEmpty() ? u"gs"_s : gsfPath);
#else
    p.setProgram(gsfPath.isEmpty() ? u"gswin64c.exe"_s : gsfPath);
#endif
    p.setArguments(QStringList() << u"-v"_s);
    p.start();
    p.waitForFinished();
    m_gsVersion.clear();
    auto versionLine = p.readLine().split(' ');
    if (versionLine.size() > 2) {
        if (versionLine[1].compare("Ghostscript", Qt::CaseInsensitive) == 0) {
            versionLine.remove(0);
            versionLine.remove(0);
            m_gsVersion = QString::fromLocal8Bit(versionLine.join(' '));
            m_gsVersion.replace(u"\n"_s, QString());
        }
    }
    p.close();
    QString gsPath;
    if (m_gsVersion.isEmpty()) {
        qCDebug(KARP_LOG) << "[ToolsThread]" << "Ghostscript not found";
    } else {
#if defined(Q_OS_UNIX)
        p.setProgram(u"whereis"_s);
        p.setArguments(QStringList() << u"gs"_s);
        p.start();
        p.waitForFinished();
        auto paths = p.readAll().split(' ');
        if (paths.size() > 1)
            gsPath = QString::fromLocal8Bit(paths[1]);
#else
        return u"gswin64.exe"_s; // TODO: handle windows
#endif
    }
    return gsPath;
}

/**
 * gs -q -dNOPAUSE -dBATCH -P- -dSAFER -sDEVICE=ps2write -sOutputFile=page.ps -c save pop -dFirstPage=i -dLastPage=i -f input.pdf
 */
QStringList ToolsThread::toPsArgs(int pageNr)
{
    static const QString tmpPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first() + QDir::separator();
    const auto pagePath = tmpPath + QString(u"karp-%1."_s).arg(pageNr, 4, 10, QLatin1Char('0'));
    const auto pageNrStr = QString::number(pageNr + 1);
    QStringList args;
    args << u"-q"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-P-"_s << u"-dSAFER"_s << u"-sDEVICE=ps2write"_s << "-sOutputFile="_L1 + pagePath + "ps"_L1
         << u"-c"_s << u"save"_s << u"pop"_s << "-dFirstPage="_L1 + pageNrStr << "-dLastPage="_L1 + pageNrStr << u"-f"_s << m_pathArg;
    return args;
}

/**
 * gs -q -P- -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sstdout=%stderr -sOutputFile=file.pdf file.ps
 */
QStringList ToolsThread::toPdfArgs(const QString &pagePath)
{
    QStringList args;
    args << u"-q"_s << u"-P-"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-sDEVICE=pdfwrite"_s << u"-sstdout=%stderr"_s << "-sOutputFile="_L1 + pagePath + "pdf"_L1
         << pagePath + "ps"_L1;
    return args;
}

void ToolsThread::gsFinishedSlot(GsThread *gs)
{
    if (!gs)
        return;

    gs->close();
    if (gs->arguments().count() == TO_PS_ARGS_COUNT) {
        // converts ps file into pdf
        auto pathStr = gs->arguments().at(OUTFILE_ARG_ID).split(u"="_s);
        if (pathStr.count() != 2)
            return;
        if (m_doCancel) {
            QFile::remove(pathStr.last());
            Q_EMIT gsProcessed();
            return;
        }
        const auto pagePath = pathStr.last().chopped(2); // remove 'ps'
        gs->setArguments(toPdfArgs(pagePath));
        if (gs->pageNr() < m_pageCountArg)
            gs->doGS();
    } else if (gs->arguments().count() == TO_PDF_ARGS_COUNT) {
        Q_EMIT progressChanged(m_pagesDone / static_cast<qreal>(m_pageCountArg + 2));
        auto pathStr = gs->arguments().at(OUTFILE_ARG_ID).split(u"="_s);
        if (pathStr.count() == 2) {
            const auto pagePath = pathStr.last().chopped(3); // remove 'pdf'
            QFile::remove(pagePath + "ps"_L1);
        }
        if (m_doCancel) {
            Q_EMIT gsProcessed();
            return;
        }
        const int nextPage = QThread::idealThreadCount() + gs->pageNr();
        m_pagesDone++;
        if (nextPage < m_pageCountArg) {
            gs->setPageNr(nextPage);
            gs->setArguments(toPsArgs(nextPage));
            gs->doGS();
        } else if (m_pagesDone == m_pageCountArg) {
            Q_EMIT gsProcessed();
        }
    }
}

bool ToolsThread::resizeByGsThread()
{
    if (m_pageCountArg < 1 || m_pathArg.isEmpty())
        return false;

    connect(this, &ToolsThread::gsProcessed, this, &ToolsThread::afterGSprocess);
    m_pagesDone = 0;
    const int optThreads = qMin(QThread::idealThreadCount(), m_pageCountArg);
    for (int t = 0; t < optThreads; ++t) {
        auto gs = new GsThread(t);
        connect(gs, &GsThread::gsFinished, this, &ToolsThread::gsFinishedSlot);
        gs->setArguments(toPsArgs(t));
        m_gsWorkers << gs;
        gs->doGS();
    }
    return true;
}

bool ToolsThread::afterGSprocess()
{
    disconnect(this, &ToolsThread::gsProcessed, this, &ToolsThread::afterGSprocess);
    QString tmpPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first() + QDir::separator();

    auto pdfModel = PdfEditModel::self();
    QStringList pages;
    for (int i = 0; i < m_pageCountArg; ++i) {
        auto pagePath = tmpPath + QString(u"karp-%1."_s).arg(i, 4, 10, QLatin1Char('0'));
        pages << pagePath + "pdf"_L1;
    }
    if (!m_doCancel && pdfModel) {
        QFileInfo outInfo(m_pathArg);
        auto outFileSize = outInfo.size();
        outInfo.setFile(tmpPath + outInfo.fileName());
        try {
            QPDFJob qpdfJob;
            auto jobConf = qpdfJob.config();
            jobConf->emptyInput()->outputFile(outInfo.filePath().toStdString());
            auto qpdfPages = jobConf->pages();
            for (const auto &pg : pages) {
                qpdfPages->pageSpec(pg.toStdString(), "");
            }
            qpdfPages->endPages();
            if (pdfModel->pdfVersion() > 0.0) {
                QpdfProxy::forcePdfVersion(jobConf.get(), pdfModel->pdfVersion());
            }
            if (!pdfModel->passKey().isEmpty()) {
                QpdfProxy::setPdfPassword(jobConf.get(), pdfModel->passKey().toStdString());
            }
            jobConf->checkConfiguration();
            auto qpdfSP = qpdfJob.createQPDF();
            auto &qpdf = *qpdfSP;
            QpdfProxy::addMetaToJob(qpdf, pdfModel->metaData());
            pdfModel->saveBookmarks(qpdf, true);
            qpdfJob.writeQPDF(qpdf);
        } catch (QPDFUsage &e) {
            qCDebug(KARP_LOG) << "[ToolsThread]" << "QPDF configuration error: " << e.what();
        } catch (std::exception &e) {
            qCDebug(KARP_LOG) << "[ToolsThread]" << "QPDF error:" << e.what();
        }
        // qCDebug(KARP_LOG) << outFileSize / 1024 << outInfo.size() / 1024;
        if (outInfo.size() > outFileSize)
            Q_EMIT progressChanged(GS_REDUCE_NOT_WORKED);
        else
            Q_EMIT progressChanged(1.0);
        // override out file with new size, but delete existing file first
        if (QFile::exists(m_pathArg))
            QFile::remove(m_pathArg);
        QFile::copy(outInfo.filePath(), m_pathArg);
        QFile::remove(outInfo.filePath()); // remove /tmp/file-out.pdf
    }
    for (const auto &tp : pages)
        QFile::remove(tp);

    for (GsThread *gs : m_gsWorkers) {
        gs->waitForFinish();
        gs->deleteLater();
    }
    m_gsWorkers.clear();
    m_mode = ToolsIdle;

    return true;
}

#include "moc_toolsthread.cpp"
