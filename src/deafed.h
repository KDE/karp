// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QObject>
#include <QQmlEngine>

class QQuickWindow;
class ToolsThread;

class DeafEd : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PDFED)
    QML_SINGLETON

    Q_PROPERTY(bool pdfLoaded READ pdfLoaded NOTIFY pdfLoadedChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString qpdfVersion READ qpdfVersion NOTIFY toolsVersionChanged)
    Q_PROPERTY(QString gsVersion READ gsVersion NOTIFY toolsVersionChanged)

public:
    explicit DeafEd(QObject *parent = nullptr);

    Q_INVOKABLE void restoreWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main"));

    Q_INVOKABLE void saveWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main")) const;

    Q_INVOKABLE QString getPdfFile();

    Q_INVOKABLE QStringList getInitFileList();

    // properties

    bool pdfLoaded() const;
    void setPdfLoaded(bool isLoaded);

    QString name() const;
    void setName(QString pdfName);

    QString path() const;
    void setPath(QString pdfPath);

    QString qpdfVersion() const;
    QString gsVersion() const;

    // helpers
    Q_INVOKABLE QColor alpha(const QColor &c, int alpha);

Q_SIGNALS:
    void pdfLoadedChanged();
    void nameChanged();
    void pathChanged();
    void toolsVersionChanged();
    void toolIsMissing(const QString &);

protected:
    void findToolsSlot();

private:
    bool m_pdfLoaded = false;
    QString m_name;
    QString m_path;
    ToolsThread *m_tools;
};
