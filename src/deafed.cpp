// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "deafed.h"
#include "deafedconfig.h"
#include "pdfeditmodel.h"
#include "toolsthread.h"
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QDebug>
#include <QFileDialog>
#include <QQuickWindow>

using namespace Qt::Literals::StringLiterals;

DeafEd::DeafEd(QObject *parent)
    : QObject(parent)
{
    m_tools = new ToolsThread();
    connect(m_tools, &ToolsThread::lookingDone, this, &DeafEd::findToolsSlot);
    m_tools->lookForTools();
}

void DeafEd::restoreWindowGeometry(QQuickWindow *window, const QString &group)
{
    auto conf = deafedConfig::self();
    KConfig dataResource(u"data"_s, KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, u"Window-"_s + group);
    KWindowConfig::restoreWindowSize(window, windowGroup);
    KWindowConfig::restoreWindowPosition(window, windowGroup);
    if (conf->fixedLastDir().isEmpty())
        conf->setFixedLastDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
}

void DeafEd::saveWindowGeometry(QQuickWindow *window, const QString &group) const
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

QString DeafEd::getPdfFile()
{
    QString lastPath = deafedConfig::self()->lastDir();
    if (lastPath.isEmpty()) {
        lastPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    }
    auto pdfFile = QFileDialog::getOpenFileName(nullptr, i18n("PDF file to edit"), lastPath, u"*.pdf"_s);
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
QStringList DeafEd::getInitFileList()
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

bool DeafEd::pdfLoaded() const
{
    return m_pdfLoaded;
}

void DeafEd::setPdfLoaded(bool isLoaded)
{
    if (isLoaded == m_pdfLoaded)
        return;
    m_pdfLoaded = isLoaded;
    Q_EMIT pdfLoadedChanged();
}

QString DeafEd::name() const
{
    return m_name;
}

void DeafEd::setName(QString pdfName)
{
    if (pdfName == m_name)
        return;
    m_name = pdfName;
    Q_EMIT nameChanged();
}

QString DeafEd::path() const
{
    return m_path;
}

void DeafEd::setPath(QString pdfPath)
{
    if (pdfPath == m_path)
        return;
    m_path = pdfPath;
    Q_EMIT pathChanged();
}

QString DeafEd::qpdfVersion() const
{
    return m_tools->qpdfVersion();
}

QString DeafEd::gsVersion() const
{
    return m_tools->gsVersion();
}

QColor DeafEd::alpha(const QColor &c, int alpha)
{
    return QColor(c.red(), c.green(), c.blue(), alpha);
}

void DeafEd::findToolsSlot()
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

#include "moc_deafed.cpp"
