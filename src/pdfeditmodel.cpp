// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfeditmodel.h"
#include "pdffile.h"
#include <KLazyLocalizedString>
#include <QDebug>
#include <QFileInfo>
#include <QPdfDocument>
#include <QPdfPageRenderer>
#include <QProcess>
#include <QScreen>
#include <QStandardPaths>

using namespace Qt::Literals::StringLiterals;

#define INIT_COLUM_COUNT (3)

PdfEditModel::PdfEditModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_prefPageWidth = qApp->screens().first()->size().width() / 4;
    m_columns = INIT_COLUM_COUNT;
}

PdfEditModel::~PdfEditModel()
{
    qDeleteAll(m_pdfList);
}

void PdfEditModel::loadPdfFile(const QString &pdfFile)
{
    auto newPdf = new PdfFile(pdfFile, pdfCount());
    if (newPdf->pageCount() < 1) {
        qDebug() << "[PdfEditModel]" << "Cannot load PDF document" << pdfFile;
        newPdf->deleteLater();
        return;
    }
    addPdfFileToModel(newPdf);
}

void PdfEditModel::addPdfs(QVector<PdfFile *> &pdfList)
{
    for (auto &pdf : pdfList) {
        addPdfFileToModel(pdf);
    }
}

int PdfEditModel::pageCount() const
{
    return m_pages;
}

QVector<PdfFile *> &PdfEditModel::pdfs()
{
    return m_pdfList;
}

qreal PdfEditModel::viewWidth() const
{
    return m_viewWidth;
}

void PdfEditModel::setViewWidth(qreal vw)
{
    if (vw == m_viewWidth)
        return;
    m_viewWidth = vw;
    if (m_columns == 0)
        return;
    updateMaxPageWidth();
    Q_EMIT viewWidthChanged();
    Q_EMIT dataChanged(index(0, 0), index(m_rows - 1, m_columns - 1), QList<int>() << RoleImage);
}

qreal PdfEditModel::maxPageWidth() const
{
    return m_maxPageWidth;
}

qreal PdfEditModel::spacing() const
{
    return m_spacing;
}

void PdfEditModel::setSpacing(qreal sp)
{
    if (sp == m_spacing)
        return;
    m_spacing = sp;
    if (m_columns == 0)
        return;
    updateMaxPageWidth();
    Q_EMIT spacingChanged();
    Q_EMIT dataChanged(index(0, 0), index(m_rows - 1, m_columns - 1), QList<int>() << RoleImage);
}

bool PdfEditModel::edited() const
{
    return m_rotatedCount || !m_deletedList.empty() || m_wasMoved || m_optimizeImages || m_reduceSize;
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

void PdfEditModel::zoomIn()
{
    if (m_columns > 1)
        changeColumnCount(m_columns - 1);
}

void PdfEditModel::zoomOut()
{
    changeColumnCount(m_columns + 1);
}

// TODO argument for PDF id
QDateTime PdfEditModel::creationDate() const
{
    return m_pdfList.first()->metaData(QPdfDocument::MetaDataField::CreationDate).toDateTime();
}

void PdfEditModel::addRotation(int pageId, int angle)
{
    if (pageId < 0 || pageId >= m_pages)
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
    m_pageList[pageId].setRotated(angle);
    int r = pageId / m_columns;
    int c = pageId % m_columns;
    Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleRotated);
    Q_EMIT editedChanged();
}

void PdfEditModel::addDeletion(int pageId)
{
    if (pageId < 0 || pageId >= m_pages)
        return;
    m_pageList[pageId].setDeleted(true);
    beginResetModel();
    m_deletedList << m_pageList.takeAt(pageId);
    m_pages--;
    m_rows = m_pages / m_columns + (m_pages % m_columns > 0 ? 1 : 0);
    endResetModel();
    Q_EMIT editedChanged();
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
    if (toPage < pageNr) // QVector::move method workaround when moving backward
        toPage++;
    m_pageList.move(pageNr, toPage);
    Q_EMIT editedChanged();
    int startPage = qMin(pageNr, toPage);
    int endPage = qMax(pageNr, toPage);
    // update all affected rows entirely
    Q_EMIT dataChanged(index(startPage / m_columns, 0), index(endPage / m_columns, m_columns - 1));
    return toPage;
}

// TODO: multiple documents
QStringList PdfEditModel::metaDataModel()
{
    QStringList mdm;
    if (m_pdfList.isEmpty())
        return mdm;
    static const KLazyLocalizedString fNames[]{kli18n("Title"),
                                               kli18n("Author"),
                                               kli18n("Subject"),
                                               kli18n("Keyword"),
                                               kli18n("Producer"),
                                               kli18n("Creator"),
                                               kli18n("Creation Date"),
                                               kli18n("Modification Date")};

    auto pdf = m_pdfList.first();
    for (int i = 0; i <= static_cast<int>(QPdfDocument::MetaDataField::ModificationDate); ++i) {
        QString value;
        auto fieldType = static_cast<QPdfDocument::MetaDataField>(i);
        if (fieldType == QPdfDocument::MetaDataField::ModificationDate || fieldType == QPdfDocument::MetaDataField::CreationDate) {
            value = pdf->metaData(fieldType).toDateTime().toString(u"yyyy.MM.dd hh:mm:ss"_s);
        } else {
            value = pdf->metaData(fieldType).toString();
        }
        mdm << KLocalizedString(fNames[i]).toString() + u"|"_s + value;
    }
    return mdm;
}

void PdfEditModel::generate()
{
    if (m_deletedList.count() >= m_pages)
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
    auto out = m_pdfList.first()->filePath();
    args << out;
    out.insert(out.length() - 4, u"-out"_s);
    // pages order and skipping deleted
    if (!m_deletedList.isEmpty() || m_wasMoved) {
        QString delArgs;
        args << u"--pages"_s << u"."_s;
        QVector<QPair<int, int>> pageRanges;
        bool lastRangeClosed = false;
        int cnt = 0;
        int fromPage = m_pageList[cnt].origPage();
        int toPage = fromPage + 1;
        cnt++;
        for (int d = cnt; d < m_pages; ++d) {
            int nr = m_pageList[d].origPage();
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
        if (m_pageList[r].rotated() == 90)
            r90 << r;
        else if (m_pageList[r].rotated() == 180)
            r180 << r;
        else if (m_pageList[r].rotated() == 270)
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
    PdfFile *pdf = nullptr;
    PdfPage *page = nullptr;
    if (pageNr < m_pages) {
        page = const_cast<PdfPage *>(&m_pageList.at(pageNr));
        if (role == RoleImage || role == RolePageRatio) {
            int refFileId = page->referenceFile();
            if (refFileId < pdfCount())
                pdf = m_pdfList[refFileId];
        }
    }
    switch (role) {
    case RoleImage: {
        if (pageNr >= m_pages)
            return QVariant::fromValue(QImage());
        if (page->nullImage() && pdf) {
            // TODO: find current screen
            QSizeF pSize = pdf->pagePointSize(page->origPage());
            qreal pageRatio = pSize.height() / pSize.width();
            pdf->requestPage(page, QSize(m_prefPageWidth, qFloor(m_prefPageWidth * pageRatio)), pageNr);
        }
        return QVariant::fromValue(m_pageList[pageNr].image());
    }
    case RoleRotated: {
        if (pageNr >= m_pages)
            return 0;
        return page->rotated();
    }
    case RoleOrigNr:
        if (pageNr >= m_pages)
            return 0;
        return page->origPage();
    case RolePageNr:
        return pageNr;
    case RolePageRatio: {
        if (pageNr >= m_pages)
            return QSize();
        auto pageSize = pdf->pagePointSize(page->origPage());
        return pageSize.height() / pageSize.width();
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{RoleImage, "pageImg"}, {RoleRotated, "rotated"}, {RoleOrigNr, "origPage"}, {RolePageNr, "pageNr"}, {RolePageRatio, "pageRatio"}};
}

Qt::ItemFlags PdfEditModel::flags(const QModelIndex &index) const
{
    int pageNr = index.row() * m_columns + index.column();
    if (pageNr < m_pages)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    return Qt::NoItemFlags;
}

QString PdfEditModel::getPagesForRotation(int angle, const QVector<quint16> &pageList)
{
    QString pRange;
    if (!pageList.isEmpty()) {
        pRange.append(QString(u"--rotate=+%1:"_s).arg(angle));
        pRange.append(QString::number(pageList[0] + 1));
    }
    int p = 1;
    while (p < pageList.count()) {
        if (!pRange.isEmpty())
            pRange.append(u","_s);
        pRange.append(QString::number(pageList[p] + 1));
        p++;
    }
    return pRange;
}

void PdfEditModel::changeColumnCount(int colCnt)
{
    if (colCnt < 1 || colCnt == m_columns)
        return;
    updateMaxPageWidth();
    beginResetModel();
    m_columns = colCnt;
    updateMaxPageWidth();
    m_rows = m_pages / m_columns + (m_pages % m_columns > 0 ? 1 : 0);
    endResetModel();
}

void PdfEditModel::updateMaxPageWidth()
{
    if (m_columns == 0)
        return;
    qreal oldMax = m_maxPageWidth;
    m_maxPageWidth = (m_viewWidth - ((m_columns + 5) * m_spacing)) / static_cast<qreal>(m_columns);
    if (oldMax != m_maxPageWidth)
        Q_EMIT maxPageWidthChanged();
}

void PdfEditModel::pageRenderedSlot(quint16 pageNr, PdfPage *pdfPage)
{
    Q_UNUSED(pdfPage)
    int r = pageNr / m_columns;
    int c = pageNr % m_columns;
    Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleImage);
}

void PdfEditModel::addPdfFileToModel(PdfFile *pdf)
{
    m_pdfList << pdf;
    pdf->setState(PdfFile::PdfLoaded); // TODO = handle other states
    connect(pdf, &PdfFile::pageRendered, this, &PdfEditModel::pageRenderedSlot);
    int pagesToAdd = pdf->pageCount();
    for (int i = 0; i < pagesToAdd; ++i) {
        m_pageList << PdfPage(i, pdf->referenceFileId());
    }
    m_pages += pagesToAdd;
    int newRowCount = m_pages / m_columns + (m_pages % m_columns > 0 ? 1 : 0);
    beginInsertRows(QModelIndex(), m_rows, newRowCount - 1);
    m_rows = newRowCount;
    endInsertRows();
    if (m_columns < 1) {
        beginInsertColumns(QModelIndex(), 0, m_columns - 1);
        endInsertColumns();
    }
    Q_EMIT pageCountChanged();
}

#include "moc_pdfeditmodel.cpp"
