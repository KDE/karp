// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "deafed.h"
#include "deafedconfig.h"
#include "pdfeditmodel.h"
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QDebug>
#include <QFileDialog>
#include <QQuickWindow>

using namespace Qt::Literals::StringLiterals;

DeaFEd::DeaFEd(QObject *parent)
    : QObject(parent)
{
}

void DeaFEd::restoreWindowGeometry(QQuickWindow *window, const QString &group) const
{
    KConfig dataResource(u"data"_s, KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, u"Window-"_s + group);
    KWindowConfig::restoreWindowSize(window, windowGroup);
    KWindowConfig::restoreWindowPosition(window, windowGroup);
}

void DeaFEd::saveWindowGeometry(QQuickWindow *window, const QString &group) const
{
    if (!m_path.isEmpty()) {
        QFileInfo lastPathInfo(m_path);
        deafedConfig::self()->setLastDir(lastPathInfo.absoluteDir().path());
    }
    deafedConfig::self()->save();
    KConfig dataResource(u"data"_s, KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, u"Window-"_s + group);
    KWindowConfig::saveWindowPosition(window, windowGroup);
    KWindowConfig::saveWindowSize(window, windowGroup);
    dataResource.sync();
}

QString DeaFEd::getPdfFile()
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

bool DeaFEd::pdfLoaded() const
{
    return m_pdfLoaded;
}

void DeaFEd::setPdfLoaded(bool isLoaded)
{
    if (isLoaded == m_pdfLoaded)
        return;
    m_pdfLoaded = isLoaded;
    Q_EMIT pdfLoadedChanged();
}

QString DeaFEd::name() const
{
    return m_name;
}

void DeaFEd::setName(QString pdfName)
{
    if (pdfName == m_name)
        return;
    m_name = pdfName;
    Q_EMIT nameChanged();
}

QString DeaFEd::path() const
{
    return m_path;
}

void DeaFEd::setPath(QString pdfPath)
{
    if (pdfPath == m_path)
        return;
    m_path = pdfPath;
    Q_EMIT pathChanged();
}

QColor DeaFEd::alpha(const QColor &c, int alpha)
{
    return QColor(c.red(), c.green(), c.blue(), alpha);
}

#include "moc_deafed.cpp"
