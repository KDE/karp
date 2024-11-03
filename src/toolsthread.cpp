// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "toolsthread.h"
#include "deafedconfig.h"
#include <QDebug>
#include <QProcess>

using namespace Qt::Literals::StringLiterals;

ToolsThread::ToolsThread()
    : QThread()
{
}

ToolsThread::~ToolsThread()
{
}

void ToolsThread::lookForTools()
{
    m_mode = ToolsFindAll;
    start();
}

void ToolsThread::lookForQPDF(const QString &qpdfPath)
{
    m_arg1 = qpdfPath;
    m_mode = ToolsFindQPDF;
    start();
}

void ToolsThread::lookForGS(const QString &gsPath)
{
    m_arg1 = gsPath;
    m_mode = ToolsFindGS;
    start();
}

QString ToolsThread::qpdfVersion() const
{
    return m_qpdfVersion;
}

QString ToolsThread::gsVersion() const
{
    return m_gsVersion;
}

void ToolsThread::run()
{
    if (m_mode == ToolsFindAll)
        findPdfTools();
    else if (m_mode == ToolsFindQPDF) {
        findQpdf(m_arg1);
        m_mode = ToolsIdle;
        Q_EMIT lookingDone();
    } else if (m_mode == ToolsFindGS) {
        findGhostScript(m_arg1);
        m_mode = ToolsIdle;
        Q_EMIT lookingDone();
    }
}

void ToolsThread::findPdfTools()
{
    auto conf = deafedConfig::self();
    conf->setQpdfPath(findQpdf());
    conf->setGsPath(findGhostScript());

    m_mode = ToolsIdle;
    Q_EMIT lookingDone();
}

QString ToolsThread::findQpdf(const QString &qpdfPath)
{
    QProcess p;
    p.setProgram(qpdfPath.isEmpty() ? u"qpdf"_s : qpdfPath);
    p.setArguments(QStringList() << u"--version"_s);
    p.start();
    p.waitForFinished();
    m_qpdfVersion.clear();
    auto versionLine = p.readLine().split(' ');
    // qpdf --version prints version with executable name given in command line
    // so if executable name is different than qpdf it will be first text.
    // To find out is it qpdf or not:
    // we just checking is 2nd string in 1st line 'version'
    if (!versionLine.isEmpty() && versionLine.size() >= 3) {
        if (versionLine[1].compare("version", Qt::CaseInsensitive) == 0) {
            m_qpdfVersion = QString::fromLocal8Bit(versionLine[2]);
            m_qpdfVersion.replace(u"\n"_s, QString());
        }
    }
    p.close();
    if (m_qpdfVersion.isEmpty()) {
        qDebug() << "[ToolsThread]" << "qpdf not found";
        return QString();
    } else {
#if defined(Q_OS_UNIX)
        p.setProgram(u"whereis"_s);
        p.setArguments(QStringList() << u"qpdf"_s);
        p.start();
        p.waitForFinished();
        auto paths = p.readAll().split(' ');
        if (paths.size() > 1)
            return QString::fromLocal8Bit(paths[1]);
#else
        return p.program();
#endif
    }
    return QString();
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
        qDebug() << "[ToolsThread]" << "Ghostscript not found";
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
