// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "app.h"
#include "karpconfig.h"
#include "pagerange.h"
#include "pdfeditmodel.h"
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
    setupActions();
    qApp->installEventFilter(this);
    m_tools = new ToolsThread();
    connect(m_tools, &ToolsThread::lookingDone, this, &App::findToolsSlot);
    m_tools->lookForTools();
}

void App::restoreWindowGeometry(QQuickWindow *window, const QString &group)
{
    auto conf = karpConfig::self();
    KConfig dataResource(u"data"_s, KConfig::SimpleConfig, QStandardPaths::AppDataLocation);
    KConfigGroup windowGroup(&dataResource, u"Window-"_s + group);
    KWindowConfig::restoreWindowSize(window, windowGroup);
    KWindowConfig::restoreWindowPosition(window, windowGroup);
    if (conf->fixedLastDir().isEmpty())
        conf->setFixedLastDir(QDir::homePath());
}

void App::saveWindowGeometry(QQuickWindow *window, const QString &group) const
{
    auto conf = karpConfig::self();
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

QString App::name() const
{
    return m_name;
}

void App::setName(const QString &pdfName)
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

void App::setupActions()
{
    AbstractKirigamiApplication::setupActions();

    auto actionName = "save_pdf"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &App::wantSavePdf);
        // action->setText(); // Handle it by QML
        action->setIcon(QIcon::fromTheme(u"document-save"_s));
        mainCollection()->addAction(action->objectName(), action);
        mainCollection()->setDefaultShortcut(action, QKeySequence::StandardKey::Save);
    }

    actionName = "open_pdf"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &App::wantOpenPdf);
        action->setText(i18nc("@action:inmenu", "Add PDF files"));
        action->setIcon(QIcon::fromTheme(u"list-add"_s));
        action->setToolTip(action->text());
        mainCollection()->addAction(action->objectName(), action);
        mainCollection()->setDefaultShortcut(action, QKeySequence::StandardKey::Open);
    }

    actionName = "clear_all"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &App::wantClearAll);
        action->setText(i18nc("@action:inmenu", "Clear all files"));
        action->setIcon(QIcon::fromTheme(u"edit-clear-all"_s));
        mainCollection()->addAction(action->objectName(), action);
    }

    actionName = "options_configure"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = KStandardActions::preferences(this, &App::wantSettings, this);
        mainCollection()->addAction(action->objectName(), action);
    }

    actionName = "optimize"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &App::wantOptimize);
        action->setText(i18nc("@action:inmenu", "Optimize PDF"));
        action->setIcon(QIcon::fromTheme(u"image-x-generic"_s));
        action->setCheckable(true);
        mainCollection()->addAction(action->objectName(), action);
    }

    actionName = "reduce_size"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &App::wantReduceSize);
        action->setText(i18nc("@action:inmenu", "Reduce size"));
        action->setIcon(QIcon::fromTheme(u"application-x-compressed-tar"_s));
        action->setCheckable(true);
        mainCollection()->addAction(action->objectName(), action);
    }

    actionName = "set_password"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &App::wantSetPassword);
        action->setText(i18nc("@action:inmenu", "Set password"));
        action->setIcon(QIcon::fromTheme(u"lock"_s));
        action->setCheckable(true);
        mainCollection()->addAction(action->objectName(), action);
    }

    actionName = "pdf_meta"_L1;
    if (KAuthorized::authorizeAction(actionName)) {
        auto action = mainCollection()->addAction(actionName, this, &App::wantPdfMeta);
        action->setText(i18nc("@action:inmenu", "PDF metadata"));
        action->setIcon(QIcon::fromTheme(u"viewpdf"_s));
        mainCollection()->addAction(action->objectName(), action);
    }
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
