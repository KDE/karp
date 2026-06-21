// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "app.h"
#include "karpconfig.h"
#include "pagerange.h"
#include "toolsthread.h"
#include <KAuthorized>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QFileDialog>
#include <QQuickWindow>

using namespace Qt::Literals::StringLiterals;

App::App(QObject *parent)
    : AbstractKirigamiApplication(parent)
{
    qApp->installEventFilter(this);
    m_tools = new ToolsThread();
    connect(m_tools, &ToolsThread::lookingDone, this, &App::findToolsSlot);
    m_tools->lookForTools();
}

QString App::getPdfFile()
{
    auto pdfFile = QFileDialog::getOpenFileName(nullptr, i18n("PDF file to edit"), getOpenDIr(), u"*.pdf"_s);
    if (pdfFile.isEmpty())
        return QString();

    setPath(pdfFile);
    QFileInfo pdfFileInfo(pdfFile);
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
    auto conf = karpConfig::self();
    if (conf->openLastDir())
        return conf->lastDir();
    else if (conf->fixedLastDir().isEmpty())
        return QDir::homePath();
    else
        return conf->fixedLastDir();
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

QString App::path() const
{
    return m_path;
}

void App::setPath(const QString &pdfPath)
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

bool App::ctrlPressed() const
{
    return m_ctrlPressed;
}

void App::setCtrlPressed(bool ctrlOn)
{
    if (ctrlOn == m_ctrlPressed)
        return;
    m_ctrlPressed = ctrlOn;
    Q_EMIT ctrlPressedChanged();
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

bool App::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent *>(ev);
        if (keyEvent->key() == Qt::Key_Control)
            setCtrlPressed(true);
    } else if (ev->type() == QEvent::KeyRelease) {
        auto keyEvent = static_cast<QKeyEvent *>(ev);
        if (keyEvent->key() == Qt::Key_Control)
            setCtrlPressed(false);
    }
    return AbstractKirigamiApplication::eventFilter(obj, ev);
}

#include "moc_app.cpp"
