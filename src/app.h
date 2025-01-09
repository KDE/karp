// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <AbstractKirigamiApplication>
#include <QtQmlIntegration/qqmlintegration.h>

class QQuickWindow;
class ToolsThread;
class PageRange;

class App : public AbstractKirigamiApplication
{
    Q_OBJECT
    QML_NAMED_ELEMENT(APP)
    QML_SINGLETON

    Q_PROPERTY(bool pdfLoaded READ pdfLoaded NOTIFY pdfLoadedChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString qpdfVersion READ qpdfVersion NOTIFY toolsVersionChanged)
    Q_PROPERTY(QString gsVersion READ gsVersion NOTIFY toolsVersionChanged)
    Q_PROPERTY(bool ctrlPressed READ ctrlPressed NOTIFY ctrlPressedChanged)

public:
    explicit App(QObject *parent = nullptr);

    Q_INVOKABLE void restoreWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main"));

    Q_INVOKABLE void saveWindowGeometry(QQuickWindow *window, const QString &group = QLatin1String("main")) const;

    Q_INVOKABLE QString getPdfFile();
    Q_INVOKABLE QStringList getPdfFiles();

    Q_INVOKABLE QStringList getInitFileList();

    /**
     * Directory path depends on user preferences: static or recently opened
     */
    Q_INVOKABLE QString getOpenDIr() const;

    Q_INVOKABLE void checkGS(const QString &gsPath);

    // properties

    bool pdfLoaded() const;
    void setPdfLoaded(bool isLoaded);

    QString name() const;
    void setName(const QString &pdfName);

    QString path() const;
    void setPath(const QString &pdfPath);

    QString qpdfVersion() const;
    QString gsVersion() const;

    bool ctrlPressed() const;
    void setCtrlPressed(bool ctrlOn);

    // helpers
    Q_INVOKABLE QColor alpha(const QColor &c, int alpha);

    Q_INVOKABLE PageRange range(int from, int to);

Q_SIGNALS:
    void pdfLoadedChanged();
    void nameChanged();
    void pathChanged();
    void toolsVersionChanged();
    void toolIsMissing(const QString &);
    void ctrlPressedChanged();
    // Main Actions
    void wantClearAll();
    void wantSettings();
    // Main Page Actions
    void wantSavePdf();
    void wantOpenPdf();
    // PDF options
    void wantOptimize();
    void wantReduceSize();
    void wantSetPassword();
    void wantPdfMeta();

protected:
    void findToolsSlot();
    void setupActions() override;

    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    bool m_pdfLoaded = false;
    QString m_name;
    QString m_path;
    ToolsThread *m_tools = nullptr;
    bool m_ctrlPressed = false;
};
