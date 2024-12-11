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
#include <QProcess>
#include <qpdf/QPDFJob.hh>
#include <qpdf/QPDFUsage.hh>

using namespace Qt::Literals::StringLiterals;

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
    if (!isRunning())
        qCDebug(KARP_LOG) << "[ToolsThread]" << "is not running";
    m_doCancel = true;
}

QString ToolsThread::qpdfVersion() const
{
    return QString::fromStdString(QPDF::QPDFVersion());
}

QString ToolsThread::gsVersion() const
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
        m_mode = ToolsIdle;
        Q_EMIT progressChanged(1.0);
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
    p.setProgram(gsfPath.isEmpty() ? u"gswin64.exe"_s : gsfPath);
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

bool ToolsThread::resizeByGsThread()
{
    if (m_pageCountArg < 1 || m_pathArg.isEmpty())
        return false;

    QProcess p;
    QStringList args;
    auto conf = karpConfig::self();
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProgram(conf->gsPath());

    QString tmpPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first() + QDir::separator();
    QFileInfo outInfo(m_pathArg);
    auto outFileSize = outInfo.size();
    auto ps = u"ps"_s;
    auto pdf = u"pdf"_s;
    QStringList pages;
    for (int i = 0; i < m_pageCountArg; ++i) {
        if (m_doCancel)
            break;
        auto pagePath = tmpPath + QString(u"karp-%1."_s).arg(i, 4, 10, QLatin1Char('0'));
        pages << pagePath + pdf;
        auto pageNr = QString::number(i + 1);
        // gs -q -dNOPAUSE -dBATCH -P- -dSAFER -sDEVICE=ps2write -sOutputFile=page.ps -c save pop -dFirstPage=i -dLastPage=i -f input.pdf
        args << u"-q"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-P-"_s << u"-dSAFER"_s << u"-sDEVICE=ps2write"_s
             << QLatin1String("-sOutputFile=") + pagePath + ps << u"-c"_s << u"save"_s << u"pop"_s << QLatin1String("-dFirstPage=") + pageNr
             << QLatin1String("-dLastPage=") + pageNr << u"-f"_s << m_pathArg;
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        p.close();
        args.clear();
        // gs -q -P- -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sstdout=%stderr -sOutputFile=file.pdf file.ps
        args << u"-q"_s << u"-P-"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-sDEVICE=pdfwrite"_s << u"-sstdout=%stderr"_s
             << QLatin1String("-sOutputFile=") + pagePath + pdf << pagePath + ps;
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        p.close();
        args.clear();
        QFile::remove(pagePath + ps);
        Q_EMIT progressChanged((i + 1) / static_cast<qreal>(m_pageCountArg + 2));
    }

    auto pdfModel = PdfEditModel::self();
    if (!m_doCancel && pdfModel) {
        outInfo.setFile(tmpPath + outInfo.fileName());
        try {
            QPDFJob qpdfJob;
            auto jobConf = qpdfJob.config();
            jobConf->emptyInput()->outputFile(outInfo.filePath().toStdString());
            auto qpdfPages = jobConf->pages();
            for (auto &p : pages) {
                qpdfPages->pageSpec(p.toStdString(), "");
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
            qpdfJob.writeQPDF(qpdf);
        } catch (QPDFUsage &e) {
            qCDebug(KARP_LOG) << "[ToolsThread]" << "QPDF configuration error: " << e.what();
        } catch (std::exception &e) {
            qCDebug(KARP_LOG) << "[ToolsThread]" << "QPDF error:" << e.what();
        }
        // qCDebug(KARP_LOG) << outFileSize / 1024 << outInfo.size() / 1024;
        if (outInfo.size() > outFileSize)
            Q_EMIT progressChanged(GS_REDUCE_NOT_WORKED);
        // override out file with new size, but delete existing file first
        if (QFile::exists(m_pathArg))
            QFile::remove(m_pathArg);
        QFile::copy(outInfo.filePath(), m_pathArg);
        QFile::remove(outInfo.filePath()); // remove /tmp/file-out.pdf
    }
    for (auto &tp : pages)
        QFile::remove(tp);

    return true;
}

#include "moc_toolsthread.cpp"
