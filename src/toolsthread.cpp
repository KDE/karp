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
    m_mode = ToolsFind;
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
    if (m_mode == ToolsFind)
        findPdfTools();
}

void ToolsThread::findPdfTools()
{
    auto conf = deafedConfig::self();
    conf->setQpdfPath(findQpdf());
    conf->setGsPath(findGhostScript());

    m_mode = ToolsIdle;
    Q_EMIT lookingDone();
}

QString ToolsThread::findQpdf()
{
    QProcess p;
    p.setProgram(u"qpdf"_s);
    p.setArguments(QStringList() << u"--version"_s);
    p.start();
    p.waitForFinished();
    m_qpdfVersion.clear();
    // m_qpdfPath.clear();
    auto versionLine = p.readLine().split(' ');
    if (!versionLine.isEmpty()) {
        if (versionLine.first().compare("qpdf", Qt::CaseInsensitive) == 0) {
            if (versionLine.size() <= 3) {
                m_qpdfVersion = QString::fromLocal8Bit(versionLine[2]);
                m_qpdfVersion.replace(u"\n"_s, QString());
            }
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

QString ToolsThread::findGhostScript()
{
    QProcess p;
    p.setProgram(u"gs"_s);
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
