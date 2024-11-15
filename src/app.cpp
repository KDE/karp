// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "app.h"
#include "deafedconfig.h"
#include "pagerange.h"
#include "pdfeditmodel.h"
#include "toolsthread.h"
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QDebug>
#include <QFileDialog>
#include <QQuickWindow>

using namespace Qt::Literals::StringLiterals;

App::App(QObject *parent)
    : QObject(parent)
{
    m_tools = new ToolsThread();
    connect(m_tools, &ToolsThread::lookingDone, this, &App::findToolsSlot);
    m_tools->lookForTools();
}

void App::restoreWindowGeometry(QQuickWindow *window, const QString &group)
{
    auto conf = deafedConfig::self();
    KConfig dataResource(u"data"_s, KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, u"Window-"_s + group);
    KWindowConfig::restoreWindowSize(window, windowGroup);
    KWindowConfig::restoreWindowPosition(window, windowGroup);
    if (conf->fixedLastDir().isEmpty())
        conf->setFixedLastDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
}

void App::saveWindowGeometry(QQuickWindow *window, const QString &group) const
{
    auto conf = deafedConfig::self();
    if (!m_path.isEmpty()) {
        QFileInfo lastPathInfo(m_path);
        conf->setLastDir(lastPathInfo.absoluteDir().path());
    }
    conf->save();
    KConfig dataResource(u"data"_s, KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, u"Window-"_s + group);
    KWindowConfig::saveWindowPosition(window, windowGroup);
    KWindowConfig::saveWindowSize(window, windowGroup);
    dataResource.sync();
}

QString App::getPdfFile()
{
    auto pdfFile = QFileDialog::getOpenFileName(nullptr, i18n("PDF file to edit"), getOpenDIr(), u"*.pdf"_s);
    if (pdfFile.isEmpty())
        return QString();

    setPath(pdfFile);
    QFileInfo pdfFileInfo(pdfFile);
    setName(pdfFileInfo.fileName());
    setPdfLoaded(true);
    return m_path;
}

/**
 * Look up for files in command line arguments.
 * If PDF file list will be passed by file browser we cannot use @p QCommandLineParser
 * which requires some flag for every value, so iterate args and check is any a PDF file.
 */
QStringList App::getInitFileList()
{
    QStringList initFiles;
    for (int a = 1; a < qApp->arguments().count(); ++a) {
        auto argString = qApp->arguments().at(a);
        QFileInfo someFile(argString);
        if (someFile.exists() && someFile.suffix().compare(u"pdf"_s, Qt::CaseInsensitive) == 0)
            initFiles << argString;
    }
    return initFiles;
}

QStringList App::getPdfFiles()
{
    return QFileDialog::getOpenFileNames(nullptr, i18n("Select PDF files"), getOpenDIr(), u"*.pdf"_s);
}

QString App::getOpenDIr() const
{
    auto conf = deafedConfig::self();
    if (conf->openLastDir())
        return conf->lastDir();
    else if (conf->fixedLastDir().isEmpty())
        return QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    else
        return conf->fixedLastDir();
}

void App::checkQPDF(const QString &qpdfPath)
{
    m_tools->lookForQPDF(qpdfPath);
}

void App::checkGS(const QString &gsPath)
{
    m_tools->lookForGS(gsPath);
}

bool App::pdfLoaded() const
{
    return m_pdfLoaded;
}

void App::setPdfLoaded(bool isLoaded)
{
    if (isLoaded == m_pdfLoaded)
        return;
    m_pdfLoaded = isLoaded;
    Q_EMIT pdfLoadedChanged();
}

QString App::name() const
{
    return m_name;
}

void App::setName(QString pdfName)
{
    if (pdfName == m_name)
        return;
    m_name = pdfName;
    Q_EMIT nameChanged();
}

QString App::path() const
{
    return m_path;
}

void App::setPath(QString pdfPath)
{
    if (pdfPath == m_path)
        return;
    m_path = pdfPath;
    Q_EMIT pathChanged();
}

QString App::qpdfVersion() const
{
    return m_tools->qpdfVersion();
}

QString App::gsVersion() const
{
    return m_tools->gsVersion();
}

QColor App::alpha(const QColor &c, int alpha)
{
    return QColor(c.red(), c.green(), c.blue(), alpha);
}

PageRange App::range(int from, int to)
{
    PageRange pr;
    pr.setFrom(from);
    pr.setTo(to);
    return pr;
}

void App::findToolsSlot()
{
    QString message;
    if (m_tools->qpdfVersion().isEmpty())
        message = i18n("QPDF is missing.");
    if (m_tools->gsVersion().isEmpty()) {
        if (!message.isEmpty())
            message.append(u"\n"_s);
        message.append(i18n("Ghostscript is missing."));
    }
    if (!message.isEmpty()) {
        message.append(u"\n"_s + i18n("Install programs mentioned above, otherwise not all functions will be available."));
        Q_EMIT toolIsMissing(message);
    }
    Q_EMIT toolsVersionChanged();
}

#include "moc_app.cpp"
