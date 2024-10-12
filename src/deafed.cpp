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

DeaFEd::DeaFEd(QObject *parent)
    : QObject(parent)
{
}

void DeaFEd::restoreWindowGeometry(QQuickWindow *window, const QString &group) const
{
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-") + group);
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
    KConfig dataResource(QStringLiteral("data"), KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, QStringLiteral("Window-") + group);
    KWindowConfig::saveWindowPosition(window, windowGroup);
    KWindowConfig::saveWindowSize(window, windowGroup);
    dataResource.sync();
}

void DeaFEd::getPdfFile()
{
    QString lastPath = deafedConfig::self()->lastDir();
    if (lastPath.isEmpty()) {
        lastPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    }
    auto pdfFile = QFileDialog::getOpenFileName(nullptr, i18n("PDF file to edit"), lastPath, QLatin1String("*.pdf"));
    if (pdfFile.isEmpty())
        return;

    setPath(pdfFile);
    QFileInfo pdfFileInfo(pdfFile);
    setName(pdfFileInfo.fileName());
    if (!m_pdfModel) {
        m_pdfModel = new PdfEditModel(m_path, this);
    }
    setPdfLoaded(true);
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

QVariant DeaFEd::pdfModel()
{
    return QVariant::fromValue(m_pdfModel);
}

#include "moc_deafed.cpp"
