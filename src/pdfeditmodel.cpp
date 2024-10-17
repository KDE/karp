// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "pdfeditmodel.h"
#include <KLazyLocalizedString>
#include <QDebug>
#include <QFileInfo>
#include <QPdfDocument>
#include <QProcess>
#include <QStandardPaths>

using namespace Qt::Literals::StringLiterals;

PdfEditModel::PdfEditModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_pdfDoc = new QPdfDocument(this);
}

PdfEditModel::~PdfEditModel()
{
    if (m_rotated)
        delete[] m_rotated;
    if (m_deleted)
        delete[] m_deleted;
}

void PdfEditModel::loadPdfFile(const QString &pdfFile)
{
    auto res = m_pdfDoc->load(pdfFile);
    if (res != QPdfDocument::Error::None) {
        qDebug() << "[PdfEditModel]" << "Cannot load PDF document" << pdfFile;
        return;
    }
    m_pdfFile = pdfFile;

    m_pages = m_pdfDoc->pageCount();
    m_rotatedCount = 0;
    m_rotated = new quint16[m_pages]{0};
    m_deleted = new bool[m_pages]{false};
    m_deletedCount = 0;
    m_pageMap.clear();
    m_wasMoved = false;
    for (int i = 0; i < m_pages; ++i) {
        m_pageMap << i;
    }
    m_columns = 3;
    m_rows = m_pages / m_columns + (m_pages % m_columns > 0 ? 1 : 0);
    beginInsertRows(QModelIndex(), 0, m_rows - 1);
    endInsertRows();
    beginInsertColumns(QModelIndex(), 0, m_columns - 1);
    endInsertColumns();
    Q_EMIT pageCountChanged();
}

int PdfEditModel::pageCount() const
{
    return m_pages;
}

qreal PdfEditModel::maxPageWidth() const
{
    return m_maxPageWidth;
}

void PdfEditModel::setMaxPageWidth(qreal maxPW)
{
    if (maxPW == m_maxPageWidth)
        return;
    m_maxPageWidth = maxPW;
    Q_EMIT maxPageWidthChanged();
    Q_EMIT dataChanged(index(0, 0), index(m_rows - 1, m_columns - 1), QList<int>() << RoleImage);
}

bool PdfEditModel::edited() const
{
    return m_rotatedCount || m_deletedCount || m_wasMoved || m_optimizeImages || m_reduceSize;
}

QString PdfEditModel::command() const
{
    return m_command;
}

void PdfEditModel::setCommand(const QString &cmd)
{
    m_command = cmd;
    Q_EMIT commandChanged();
}

bool PdfEditModel::optimizeImages() const
{
    return m_optimizeImages;
}

void PdfEditModel::setOptimizeImages(bool optImgs)
{
    if (m_optimizeImages == optImgs)
        return;
    m_optimizeImages = optImgs;
    Q_EMIT optimizeImagesChanged();
    Q_EMIT editedChanged();
}

bool PdfEditModel::reduceSize() const
{
    return m_reduceSize;
}

void PdfEditModel::setReduceSize(bool redS)
{
    if (m_reduceSize == redS)
        return;

    m_reduceSize = redS;
    Q_EMIT reduceSizeChanged();
    Q_EMIT editedChanged();
}

void PdfEditModel::addRotation(int pageId, int angle)
{
    if (pageId < 0 || pageId >= m_rows)
        return;
    if (angle < 0) {
        switch (angle) {
        case -90:
            angle = 270;
            break;
        case -180:
            angle = 180;
            break;
        case -270:
            angle = 90;
            break;
        }
    }
    if (angle)
        m_rotatedCount++;
    else
        m_rotatedCount--;
    m_rotated[map(pageId)] = angle;
    Q_EMIT editedChanged();
}

void PdfEditModel::addDeletion(int pageId, bool doDel)
{
    if (pageId < 0 || pageId >= m_pages)
        return;
    m_deleted[map(pageId)] = doDel;
    if (doDel) {
        m_deletedCount++;
    } else {
        if (m_deletedCount == 0) {
            qDebug() << "[PdfEditModel]" << "No page was deleted yet!";
            return;
        } else {
            m_deletedCount--;
        }
    }
    Q_EMIT editedChanged();
    int r = pageId / m_rows;
    int c = pageId % m_columns;
    Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleDeleted);
}

/**
 * Moves given @p pageNr to @p toPage.
 * If next page after @p toPage is deleted, moves @p pageNr after deleted page.
 * Returns -1 if target page can be determined or target page number.
 * It skip all deleted page in row if they occur after @p toPage target.
 */
int PdfEditModel::addMove(int pageNr, int toPage)
{
    if (pageNr < 0 || pageNr >= m_pages || toPage < 0 || toPage >= m_pages)
        return -1;

    m_wasMoved = true;
    if (toPage < m_pages - 1 && m_deleted[map(toPage + 1)]) {
        // if not the last page - check is page after target  one deleted
        // if so, move page after deleted
        int cnt = toPage + 1;
        while (cnt < m_pages) {
            if (!m_deleted[map(cnt)]) {
                toPage = cnt - 1;
                break;
            }
            cnt++;
        }
        if (cnt >= m_pages)
            return -1;
    }
    m_pageMap.move(pageNr, toPage);
    Q_EMIT editedChanged();
    Q_EMIT dataChanged(index(0, 0), index(m_rows - 1, m_columns - 1));
    return toPage;
}

QStringList PdfEditModel::metaDataModel()
{
    QStringList mdm;
    if (m_pdfFile.isEmpty())
        return mdm;
    static const KLazyLocalizedString fNames[]{kli18n("Title"),
                                               kli18n("Author"),
                                               kli18n("Subject"),
                                               kli18n("Keyword"),
                                               kli18n("Producer"),
                                               kli18n("Creator"),
                                               kli18n("Creation Date"),
                                               kli18n("Modification Date")};
    for (int i = 0; i <= static_cast<int>(QPdfDocument::MetaDataField::ModificationDate); ++i) {
        QString value;
        auto fieldType = static_cast<QPdfDocument::MetaDataField>(i);
        if (fieldType == QPdfDocument::MetaDataField::ModificationDate || fieldType == QPdfDocument::MetaDataField::CreationDate) {
            value = m_pdfDoc->metaData(fieldType).toDateTime().toString(u"yyyy.MM.dd hh:mm:ss"_s);
        } else {
            value = m_pdfDoc->metaData(fieldType).toString();
        }
        mdm << KLocalizedString(fNames[i]).toString() + u"|"_s + value;
    }
    return mdm;
}

void PdfEditModel::generate()
{
    if (m_deletedCount >= m_pages)
        return;
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProgram(u"qpdf"_s);
    // p.setArguments(QStringList() << u"--version"_s);
    // p.start();
    // p.waitForFinished();
    // qDebug().noquote() << p.readLine();
    // p.close();

    QStringList args;
    args << m_pdfFile;
    auto out = m_pdfFile;
    out.insert(m_pdfFile.length() - 4, u"-out"_s);
    // pages order and skipping deleted
    if (m_deletedCount > 0 || m_wasMoved) {
        QString delArgs;
        args << u"--pages"_s << u"."_s;
        QVector<QPair<int, int>> pageRanges;
        bool lastRangeClosed = false;
        int cnt = 0;
        while (cnt < m_pages && m_deleted[map(cnt)]) {
            cnt++;
        }
        // cnt++;
        if (cnt >= m_pages)
            return;
        int fromPage = map(cnt);
        int toPage = fromPage + 1;
        cnt++;
        for (int d = cnt; d < m_pages; ++d) {
            int nr = map(d);
            if (m_deleted[nr]) {
                if (d == m_pages - 1)
                    lastRangeClosed = false;
                continue;
            }
            if (nr == toPage) {
                toPage++;
                lastRangeClosed = false;
                continue;
            } else {
                toPage--;
                pageRanges << QPair<int, int>(fromPage, toPage);
                lastRangeClosed = d < m_pages - 1;
                fromPage = nr;
                toPage = fromPage + 1;
            }
        }
        if (!pageRanges.isEmpty() && !lastRangeClosed)
            pageRanges << QPair<int, int>(fromPage, toPage - 1);
        for (auto &r : pageRanges) {
            if (!delArgs.isEmpty())
                delArgs.append(u","_s);
            if (r.first == r.second) {
                delArgs.append(QString::number(r.first + 1));
            } else {
                delArgs.append(QString(u"%1-%2"_s).arg(r.first + 1).arg(r.second + 1));
            }
        }
        args << delArgs << u"--"_s;
    }
    // images optimization
    if (m_optimizeImages) {
        // args << u"-recompress-flate"_s << u"--compression-level=9"_s << u"--compress-streams=y"_s << u"--object-streams=generate"_s;
        args << u"--optimize-images"_s;
    }
    // Rotation of pages - aggregate angles
    QVector<quint16> r90, r180, r270;
    for (int r = 0; r < m_pages; ++r) {
        if (m_rotated[r] == 90)
            r90 << r;
        else if (m_rotated[r] == 180)
            r180 << r;
        else if (m_rotated[r] == 270)
            r270 << r;
    }
    if (!r90.isEmpty())
        args << getPagesForRotation(90, r90);
    if (!r180.isEmpty())
        args << getPagesForRotation(180, r180);
    if (!r270.isEmpty())
        args << getPagesForRotation(270, r270);
    args << out;
    setCommand(u"qpdf"_s + u" "_s + args.join(u" "_s));
    qDebug().noquote() << m_command;
    p.setArguments(args);
    p.start();
    p.waitForFinished();
    qDebug().noquote().nospace() << p.readAll();
    p.close();

    if (m_reduceSize) {
        p.setProgram(u"pdf2ps"_s);
        args.clear();
        args << out;
        QString tmpPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first() + u"/"_s;
        QFileInfo outInfo(out);
        auto outFileSize = outInfo.size();
        args << tmpPath + outInfo.baseName() + u".ps"_s;
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        // perform pdf2ps - store *.ps file in /tmp
        p.close();
        p.setProgram(u"ps2pdf"_s);
        args.remove(0);
        args << tmpPath + outInfo.fileName();
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        p.close();
        outInfo.setFile(args[1]);
        // qDebug() << outFileSize / 1024 << outInfo.size() / 1024;
        if (outInfo.size() < outFileSize) {
            // override out file with new size, but delete existing file first
            if (QFile::exists(out))
                QFile::remove(out);
            qDebug() << "[PdfEditModel]" << "PDF file size successfully reduced.";
            QFile::copy(outInfo.filePath(), out);
        }
        QFile::remove(tmpPath + outInfo.baseName() + u".ps"_s); // remove /tmp/file-out.ps
        QFile::remove(outInfo.filePath()); // remove /tmp/file-out.pdf
    }
}

QPdfDocument *PdfEditModel::pdfDocument() const
{
    return m_pdfDoc;
}

int PdfEditModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_rows;
}

int PdfEditModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_columns;
}

QVariant PdfEditModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows || index.column() < 0 || index.column() >= m_columns)
        return QVariant();
    int pageNr = index.row() * m_columns + index.column();
    int origPage = 0;
    if (pageNr < m_pages)
        origPage = map(pageNr);
    switch (role) {
    case RoleImage: {
        if (pageNr >= m_pages)
            return QVariant::fromValue(QImage());
        QSizeF pSize = m_pdfDoc->pagePointSize(origPage);
        qreal pageRatio = pSize.height() / pSize.width();
        return QVariant::fromValue(m_pdfDoc->render(origPage, QSize(m_maxPageWidth, qFloor(m_maxPageWidth * pageRatio))));
    }
    case RoleRotated: {
        if (pageNr >= m_pages)
            return 0;
        return m_rotated[origPage];
    }
    case RoleDeleted: {
        if (pageNr >= m_pages)
            return false;
        return m_deleted[origPage];
    }
    case RoleOrigNr:
        return origPage;
    case RolePageNr:
        return pageNr;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{RoleImage, "pageImg"}, {RoleRotated, "rotated"}, {RoleDeleted, "deleted"}, {RoleOrigNr, "origPage"}, {RolePageNr, "pageNr"}};
}

QString PdfEditModel::getPagesForRotation(int angle, const QVector<quint16> &pageList)
{
    QString pRange;
    if (!pageList.isEmpty()) {
        pRange.append(QString(u"--rotate=+%1:"_s).arg(angle));
        pRange.append(QString::number(map(pageList[0]) + 1));
    }
    int p = 1;
    while (p < pageList.count()) {
        if (!pRange.isEmpty())
            pRange.append(u","_s);
        pRange.append(QString::number(map(pageList[p]) + 1));
        p++;
    }
    return pRange;
}

#include "moc_pdfeditmodel.cpp"
