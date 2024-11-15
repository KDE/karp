// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QObject>
#include <QQmlEngine>

class QQuickWindow;
class ToolsThread;
class PageRange;

class App : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(APP)
    QML_SINGLETON

    Q_PROPERTY(bool pdfLoaded READ pdfLoaded NOTIFY pdfLoadedChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString qpdfVersion READ qpdfVersion NOTIFY toolsVersionChanged)
    Q_PROPERTY(QString gsVersion READ gsVersion NOTIFY toolsVersionChanged)

public:
    explicit App(QObject *parent = nullptr);

    Q_INVOKABLE void restoreWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main"));

    Q_INVOKABLE void saveWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main")) const;

    Q_INVOKABLE QString getPdfFile();
    Q_INVOKABLE QStringList getPdfFiles();

    Q_INVOKABLE QStringList getInitFileList();

    /**
     * Directory path depends on user preferences: static or last opened
     */
    Q_INVOKABLE QString getOpenDIr() const;

    Q_INVOKABLE void checkQPDF(const QString &qpdfPath);
    Q_INVOKABLE void checkGS(const QString &gsPath);

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

    Q_INVOKABLE PageRange range(int from, int to);

Q_SIGNALS:
    void pdfLoadedChanged();
    void nameChanged();
    void pathChanged();
    void toolsVersionChanged();
    void toolIsMissing(const QString &);
    void toolCheckMessage(const QString &);

protected:
    void findToolsSlot();

private:
    bool m_pdfLoaded = false;
    QString m_name;
    QString m_path;
    ToolsThread *m_tools;
};
