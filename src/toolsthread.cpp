// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "toolsthread.h"
#include "deafedconfig.h"
#include "pdffile.h"
#include "pdfmetadata.h"
#include "version-deafed.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

using namespace Qt::Literals::StringLiterals;

ToolsThread *ToolsThread::m_self = nullptr;

ToolsThread::ToolsThread()
    : QThread()
{
    m_self = this;
}

ToolsThread::~ToolsThread()
{
}

void ToolsThread::lookForTools()
{
    m_mode = ToolsFindAll;
    start();
}

void ToolsThread::lookForQPDF(const QString &qpdfPath)
{
    m_pathArg = qpdfPath;
    m_mode = ToolsFindQPDF;
    start();
}

void ToolsThread::lookForGS(const QString &gsPath)
{
    m_pathArg = gsPath;
    m_mode = ToolsFindGS;
    start();
}

void ToolsThread::resizeByGs(const QString &filePath, int pages)
{
    m_pathArg = filePath;
    m_pageCountArg = pages;
    m_mode = ToolsResizeByGs;
    start();
}

void ToolsThread::cancel()
{
    if (!isRunning())
        qDebug() << "[ToolsThread]" << "is not running";
    m_doCancel = true;
}

void ToolsThread::applyMetadata(const QString &pdfPath, PdfMetaData *metadata)
{
    m_pathArg = pdfPath;
    m_metadataArg = metadata;
    m_mode = ToolsMetadata;
    start();
}

QString ToolsThread::qpdfVersion() const
{
    return m_qpdfVersion;
}

QString ToolsThread::gsVersion() const
{
    return m_gsVersion;
}

void ToolsThread::run()
{
    if (m_mode == ToolsFindAll)
        findPdfTools();
    else if (m_mode == ToolsFindQPDF) {
        findQpdf(m_pathArg);
        m_mode = ToolsIdle;
        Q_EMIT lookingDone();
    } else if (m_mode == ToolsFindGS) {
        findGhostScript(m_pathArg);
        m_mode = ToolsIdle;
        Q_EMIT lookingDone();
    } else if (m_mode == ToolsResizeByGs) {
        resizeByGsThread();
        m_mode = ToolsIdle;
        Q_EMIT progressChanged(1.0);
    } else if (m_mode == ToolsMetadata) {
        doMetadata();
        m_mode = ToolsIdle;
    }
    m_doCancel = false;
}

void ToolsThread::findPdfTools()
{
    auto conf = deafedConfig::self();
    conf->setQpdfPath(findQpdf());
    conf->setGsPath(findGhostScript());

    m_mode = ToolsIdle;
    Q_EMIT lookingDone();
}

QString ToolsThread::findQpdf(const QString &qpdfPath)
{
    QProcess p;
    p.setProgram(qpdfPath.isEmpty() ? u"qpdf"_s : qpdfPath);
    p.setArguments(QStringList() << u"--version"_s);
    p.start();
    p.waitForFinished();
    m_qpdfVersion.clear();
    auto versionLine = p.readLine().split(' ');
    // qpdf --version prints version with executable name given in command line
    // so if executable name is different than qpdf it will be first text.
    // To find out is it qpdf or not:
    // we just checking is 2nd string in 1st line 'version'
    if (!versionLine.isEmpty() && versionLine.size() >= 3) {
        if (versionLine[1].compare("version", Qt::CaseInsensitive) == 0) {
            m_qpdfVersion = QString::fromLocal8Bit(versionLine[2]);
            m_qpdfVersion.replace(u"\n"_s, QString());
        }
    }
    p.close();
    if (m_qpdfVersion.isEmpty()) {
        qDebug() << "[ToolsThread]" << "qpdf not found";
        return QString();
    } else {
#if defined(Q_OS_UNIX)
        p.setProgram(u"whereis"_s);
        p.setArguments(QStringList() << u"qpdf"_s);
        p.start();
        p.waitForFinished();
        auto paths = p.readAll().split(' ');
        if (paths.size() > 1)
            return QString::fromLocal8Bit(paths[1]);
#else
        return p.program();
#endif
    }
    return QString();
}

QString ToolsThread::findGhostScript(const QString &gsfPath)
{
    QProcess p;
#if defined(Q_OS_UNIX)
    p.setProgram(gsfPath.isEmpty() ? u"gs"_s : gsfPath);
#else
    p.setProgram(gsfPath.isEmpty() ? u"gswin64.exe"_s : gsfPath);
#endif
    p.setArguments(QStringList() << u"-v"_s);
    p.start();
    p.waitForFinished();
    m_gsVersion.clear();
    auto versionLine = p.readLine().split(' ');
    if (versionLine.size() > 2) {
        if (versionLine[1].compare("Ghostscript", Qt::CaseInsensitive) == 0) {
            versionLine.remove(0);
            versionLine.remove(0);
            m_gsVersion = QString::fromLocal8Bit(versionLine.join(' '));
            m_gsVersion.replace(u"\n"_s, QString());
        }
    }
    p.close();
    QString gsPath;
    if (m_gsVersion.isEmpty()) {
        qDebug() << "[ToolsThread]" << "Ghostscript not found";
    } else {
#if defined(Q_OS_UNIX)
        p.setProgram(u"whereis"_s);
        p.setArguments(QStringList() << u"gs"_s);
        p.start();
        p.waitForFinished();
        auto paths = p.readAll().split(' ');
        if (paths.size() > 1)
            gsPath = QString::fromLocal8Bit(paths[1]);
#else
        return u"gswin64.exe"_s; // TODO: handle windows
#endif
    }
    return gsPath;
}

bool ToolsThread::resizeByGsThread()
{
    if (m_pageCountArg < 1 || m_pathArg.isEmpty())
        return false;
    QProcess p;
    QStringList args;
    auto conf = deafedConfig::self();
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProgram(conf->gsPath());

    QString tmpPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first() + QDir::separator();
    QFileInfo outInfo(m_pathArg);
    auto outFileSize = outInfo.size();
    auto ps = u"ps"_s;
    auto pdf = u"pdf"_s;
    QStringList pages;
    for (int i = 0; i < m_pageCountArg; ++i) {
        if (m_doCancel)
            break;
        auto pagePath = tmpPath + QString(u"deafed-%1."_s).arg(i, 4, 10, QLatin1Char('0'));
        pages << pagePath + pdf;
        auto pageNr = QString::number(i + 1);
        // gs -q -dNOPAUSE -dBATCH -P- -dSAFER -sDEVICE=ps2write -sOutputFile=page.ps -c save pop -dFirstPage=i -dLastPage=i -f input.pdf
        args << u"-q"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-P-"_s << u"-dSAFER"_s << u"-sDEVICE=ps2write"_s
             << QLatin1String("-sOutputFile=") + pagePath + ps << u"-c"_s << u"save"_s << u"pop"_s << QLatin1String("-dFirstPage=") + pageNr
             << QLatin1String("-dLastPage=") + pageNr << u"-f"_s << m_pathArg;
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        p.close();
        args.clear();
        // gs -q -P- -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sstdout=%stderr -sOutputFile=file.pdf file.ps
        args << u"-q"_s << u"-P-"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-sDEVICE=pdfwrite"_s << u"-sstdout=%stderr"_s
             << QLatin1String("-sOutputFile=") + pagePath + pdf << pagePath + ps;
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        p.close();
        args.clear();
        QFile::remove(pagePath + ps);
        Q_EMIT progressChanged((i + 1) / static_cast<qreal>(m_pageCountArg + 2));
    }

    if (!m_doCancel) {
        p.setProgram(conf->qpdfPath());
        args << u"--empty"_s << u"--pages"_s << pages << u"--"_s << tmpPath + outInfo.fileName();
        p.setArguments(args);
        qDebug().noquote() << p.program() << p.arguments();
        p.start();
        p.waitForFinished();
        qDebug() << p.readAll();
        p.close();
        outInfo.setFile(tmpPath + outInfo.fileName());
        qDebug() << outFileSize / 1024 << outInfo.size() / 1024;
        if (outInfo.size() < outFileSize) {
            // override out file with new size, but delete existing file first
            if (QFile::exists(m_pathArg))
                QFile::remove(m_pathArg);
            qDebug() << "[PdfEditModel]" << "PDF file size successfully reduced." << outInfo.filePath();
            QFile::copy(outInfo.filePath(), m_pathArg);
        }
        QFile::remove(outInfo.filePath()); // remove /tmp/file-out.pdf
    }
    for (auto &tp : pages)
        QFile::remove(tp);

    return true;
}

void ToolsThread::doMetadata()
{
    if (m_pathArg.isEmpty())
        return;
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    auto conf = deafedConfig::self();
    QString tmpPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first() + QDir::separator();
    p.setProgram(conf->qpdfPath());
    QStringList args;
    args << u"--json"_s << m_pathArg << tmpPath + QLatin1String("pdf_metadata_out.json");
    p.setArguments(args);
    p.start();
    p.waitForFinished();
    qDebug().noquote() << p.readAll();
    p.close();
    qDebug() << p.program() << args;

    QFile jsonFile(args.last());
    if (!jsonFile.open(QIODevice::ReadOnly))
        return;

    QByteArray metaKey, rootKey;
    bool metaFound = false;
    while (!jsonFile.atEnd()) {
        auto line = jsonFile.readLine();
        if (line.contains("obj:"_ba)) {
            if (metaFound)
                break;
            else
                metaKey = line;
        } else {
            for (auto &k : PdfMetaData::tags()) {
                if (line.contains(k)) {
                    metaFound = true;
                    break;
                }
            }
        }
    }
    int maxObjectId = -1;
    if (metaFound) {
        auto pos = metaKey.indexOf("obj:"_ba);
        metaKey = metaKey.sliced(pos);
        pos = metaKey.lastIndexOf("\""_ba);
        metaKey.truncate(pos);
    } else {
        jsonFile.seek(0);
        while (!jsonFile.atEnd()) {
            auto line = jsonFile.readLine();
            if (line.contains("maxobjectid"_ba)) {
                auto s = line.split(':');
                if (s.size() < 2)
                    return;
                maxObjectId = QString::fromLatin1(s[1]).toInt();
                metaKey = QString(u"%1 0 R"_s).arg(maxObjectId + 1).toStdString().data();
            } else if (line.contains("/Root"_ba)) {
                auto s = line.split(':');
                if (s.size() < 2)
                    return;
                auto pos = s[1].indexOf("\""_ba);
                rootKey = s[1].sliced(pos + 1);
                pos = rootKey.lastIndexOf("\""_ba);
                rootKey.truncate(pos);
                break;
            }
        }
    }
    jsonFile.close();
    // qDebug().noquote() << metaKey;

    QJsonObject rootJSON;
    QJsonObject qpdfJSON;
    qpdfJSON.insert("jsonversion"_L1, QJsonValue::fromVariant(2));
    QJsonArray qpdfArray;
    QJsonObject metaJSON;
    if (maxObjectId > -1) {
        metaJSON.insert(QLatin1String("obj:"_ba + metaKey), m_metadataArg->toJSON());
        QJsonObject infoJSON;
        infoJSON.insert("/Info"_L1, QJsonValue::fromVariant(metaKey));
        infoJSON.insert("/Root"_L1, QJsonValue::fromVariant(rootKey));
        QJsonObject valueJSON;
        valueJSON.insert("value"_L1, infoJSON);
        metaJSON.insert("trailer"_L1, valueJSON);
    } else {
        metaJSON.insert(QLatin1String(metaKey), m_metadataArg->toJSON());
    }
    qpdfArray.push_back(qpdfJSON);
    qpdfArray.push_back(metaJSON);
    rootJSON.insert(u"qpdf"_s, qpdfArray);
    QJsonDocument json(rootJSON);
    // qDebug().noquote() << json.toJson();
    jsonFile.remove();
    if (!jsonFile.open(QIODevice::WriteOnly))
        return;
    jsonFile.write(json.toJson());
    jsonFile.close();
    args.clear();
    args << m_pathArg << u"--replace-input"_s << "--update-from-json="_L1 + jsonFile.fileName();
    p.setArguments(args);
    p.start();
    p.waitForFinished();
    // qDebug() << p.readAll();
    jsonFile.remove();
}
