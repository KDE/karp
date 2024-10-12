// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#pragma once

#include <QObject>
#include <QQmlEngine>

class QQuickWindow;
class PdfEditModel;

class DeaFEd : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PDFED)
    QML_SINGLETON

    Q_PROPERTY(bool pdfLoaded READ pdfLoaded NOTIFY pdfLoadedChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QVariant pdfModel READ pdfModel NOTIFY pdfLoadedChanged)

public:
    explicit DeaFEd(QObject *parent = nullptr);

    Q_INVOKABLE void restoreWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main")) const;

    Q_INVOKABLE void saveWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main")) const;

    Q_INVOKABLE void getPdfFile();

    // properties

    bool pdfLoaded() const;
    void setPdfLoaded(bool isLoaded);

    QString name() const;
    void setName(QString pdfName);

    QString path() const;
    void setPath(QString pdfPath);

    QVariant pdfModel();

Q_SIGNALS:
    void pdfLoadedChanged();
    void nameChanged();
    void pathChanged();

private:
    bool m_pdfLoaded = false;
    QString m_name;
    QString m_path;
    PdfEditModel *m_pdfModel = nullptr;
};
