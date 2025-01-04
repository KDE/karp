// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfeditmodel.h"
#include "bookmarkmodel.h"
#include "karp_debug.h"
#include "karpconfig.h"
#include "pdffile.h"
#include "pdfmetadata.h"
#include "qpdfproxy.h"
#include "toolsthread.h"
#include <KLazyLocalizedString>
#include <QFileDialog>
#include <QFileInfo>
#include <QPdfDocument>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <qalgorithms.h>
#include <qlogging.h>
#include <qtmetamacros.h>
#include <qtypes.h>

using namespace Qt::Literals::StringLiterals;

#define INIT_COLUMN_COUNT (4)

QColor alpha(const QColor &c)
{
    return QColor(c.red(), c.green(), c.blue(), 0x80);
}

PdfEditModel *PdfEditModel::m_self = nullptr;

PdfEditModel::PdfEditModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_self = this;
    m_metaData = new PdfMetaData();
    m_prefPageWidth = qApp->screens().first()->size().width() / 4;
    m_columns = INIT_COLUMN_COUNT;
    m_labelColors << alpha(Qt::black) << alpha(Qt::darkMagenta) << alpha(Qt::darkYellow) << alpha(Qt::darkCyan) << alpha(Qt::darkBlue) << alpha(Qt::darkGreen);
    m_pageRange.reset();
    m_bookmarks = new BookmarkModel(this);
    connect(m_bookmarks, &BookmarkModel::statusChanged, this, &PdfEditModel::editedChanged);
}

PdfEditModel::~PdfEditModel()
{
    qDeleteAll(m_pageList);
    qDeleteAll(m_deletedList);
    qDeleteAll(m_pdfList);
    delete m_metaData;
    m_self = nullptr;
}

void PdfEditModel::loadPdfFile(const QString &pdfFile)
{
    auto newPdf = new PdfFile(pdfFile, pdfCount());
    if (!(newPdf->error() == QPdfDocument::Error::None || newPdf->error() == QPdfDocument::Error::IncorrectPassword)) {
        qCDebug(KARP_LOG) << "[PdfEditModel]" << "Cannot load PDF document" << pdfFile << newPdf->error();
        newPdf->deleteLater();
        return;
    }
    appendPdfFileToModel(newPdf);
    updateCreationTimeInMetadata(newPdf);
    karpConfig::self()->setLastDir(m_pdfList.last()->dir());
    m_bookmarks->appendPdf(newPdf);
}

void PdfEditModel::prependPdfs(QVector<PdfFile *> &pdfList)
{
    if (pdfList.isEmpty())
        return;
    for (int p = pdfList.count() - 1; p >= 0; --p) {
        auto pdf = pdfList[p];
        prependPdfFileToModel(pdf);
        updateCreationTimeInMetadata(pdf);
    }
    karpConfig::self()->setLastDir(pdfList.last()->dir());
    if (pdfCount() > 1)
        Q_EMIT editedChanged();
    for (auto &pdf : pdfList) {
        m_bookmarks->prependPdf(pdf);
    }
}

void PdfEditModel::appendPdfs(QVector<PdfFile *> &pdfList)
{
    if (pdfList.isEmpty())
        return;
    for (auto &pdf : pdfList) {
        appendPdfFileToModel(pdf);
        updateCreationTimeInMetadata(pdf);
    }
    karpConfig::self()->setLastDir(pdfList.last()->dir());
    if (pdfCount() > 1)
        Q_EMIT editedChanged();
    for (auto &pdf : pdfList) {
        m_bookmarks->appendPdf(pdf);
    }
}

QVector<PdfFile *> &PdfEditModel::pdfs()
{
    return m_pdfList;
}

QString PdfEditModel::getPdfName(int fileId)
{
    return fileId < pdfCount() ? m_pdfList[fileId]->name() : QString();
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
    Q_EMIT dataChanged(index(0, 0), index(m_pages - 1, 0), QList<int>() << RoleImage);
}

qreal PdfEditModel::maxPageWidth() const
{
    return m_maxPageWidth;
}

qreal PdfEditModel::maxPageHeight() const
{
    if (m_pdfList.isEmpty())
        return 1.0;
    auto pageSize = m_pdfList.first()->pagePointSize(0);
    return m_maxPageWidth * (pageSize.height() / pageSize.width());
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
    Q_EMIT dataChanged(index(0, 0), index(m_pages - 1, 0), QList<int>() << RoleImage);
}

bool PdfEditModel::edited() const
{
    return m_rotatedCount || !m_deletedList.empty() || m_wasMoved || m_optimizeImages || pdfCount() > 1 || m_reduceSize || !m_passKey.isEmpty()
        || m_metaData->modified() || m_pdfVersion > 0.0 || pdfCount() > 1 || m_bookmarks->status() == BookmarkModel::Status::Modified
        || m_bookmarks->status() == BookmarkModel::Status::Removed;
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

qreal PdfEditModel::pdfVersion() const
{
    return m_pdfVersion;
}

void PdfEditModel::setPdfVersion(qreal pV)
{
    if (m_pdfVersion == pV)
        return;

    m_pdfVersion = pV;
    Q_EMIT pdfVersionChanged();
    Q_EMIT editedChanged();
}

QString PdfEditModel::passKey() const
{
    return m_passKey;
}

void PdfEditModel::setPassKey(const QString &pass)
{
    m_passKey = pass;
    Q_EMIT passKeyChanged();
    Q_EMIT editedChanged();
}

qreal PdfEditModel::progress() const
{
    return m_progress;
}

void PdfEditModel::setProgress(qreal prog)
{
    m_progress = prog;
    Q_EMIT progressChanged();
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

void PdfEditModel::rotatePage(int pageId, int angle)
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
    m_pageList[pageId]->setRotated(angle);
    Q_EMIT dataChanged(index(pageId, 0), index(pageId, 0), QList<int>() << RoleRotated);
    Q_EMIT editedChanged();
}

void PdfEditModel::rotatePages(const PageRange &range, int angle)
{
    if (rangeIsInvalid(range))
        return;

    int from, to;
    int step = range.everyN() ? range.n() : 1;
    if (range.allInRange() || range.everyN()) {
        from = range.from() - 1;
        to = range.to();
    } else {
        from = 0;
        to = range.from();
    }
    for (int p = from; p < to; p += step) {
        int a = m_pageList[p]->rotated() + angle;
        if (a > 270)
            a = a - 360;
        m_pageList[p]->setRotated(a);
        Q_EMIT dataChanged(index(p, 0), index(p, 0), QList<int>() << RoleRotated);
        if (a)
            m_rotatedCount++;
        else
            m_rotatedCount--;
    }
    if (range.allOutOfRange()) {
        from = range.to(); // we start 1 page after the range.to
        to = m_pages;
        for (int p = from; p < to; p += step) {
            int a = m_pageList[p]->rotated() + angle;
            if (a > 270)
                a = a - 360;
            m_pageList[p]->setRotated(a);
            Q_EMIT dataChanged(index(p, 0), index(p, 0), QList<int>() << RoleRotated);
            if (a)
                m_rotatedCount++;
            else
                m_rotatedCount--;
        }
    }
    Q_EMIT editedChanged();
}

void PdfEditModel::deletePage(int pageId)
{
    if (pageId < 0 || pageId >= m_pages)
        return;
    beginRemoveRows(QModelIndex(), pageId, pageId);
    m_pageList[pageId]->setDeleted(true);
    m_deletedList << m_pageList.takeAt(pageId);
    m_pages--;
    endRemoveRows();
    if (pageId < m_pages)
        Q_EMIT dataChanged(index(pageId, 0), index(m_pages - 1, 0));
    Q_EMIT pageCountChanged();
    Q_EMIT editedChanged();
    setSelection(0, 0);
}

/**
 * NOTICE: @p PageRange is numbering pages from 1, 0 is invalid
 */
void PdfEditModel::deletePages(const PageRange &range)
{
    if (rangeIsInvalid(range))
        return;

    // qCDebug(KARP_LOG) << range.from() << range.to() << range.type() << range.n();
    int from, to;
    int step = range.everyN() ? range.n() : 1;
    if (range.allOutOfRange()) {
        // At first, delete what is after the range to preserve numbering at the beginning
        from = range.to();
        to = m_pages;
        beginRemoveRows(QModelIndex(), from, to - 1);
        for (int p = from; p < to; ++p) {
            m_deletedList << m_pageList.takeAt(from);
            m_pages--;
        }
        endRemoveRows();
    }
    from = range.allOutOfRange() ? 0 : range.from() - 1;
    to = range.allOutOfRange() ? range.from() - 2 : range.to() - 1;
    QVector<int> toTakeList;
    for (int p = from; p <= to; p += step) {
        toTakeList << p;
    }
    while (!toTakeList.empty()) {
        const int pageToRemove = toTakeList.takeLast();
        beginRemoveRows(QModelIndex(), pageToRemove, pageToRemove);
        m_deletedList << m_pageList.takeAt(pageToRemove);
        m_pages--;
        endRemoveRows();
    }
    Q_EMIT dataChanged(index(0, 0), index(m_pages - 1, 0));
    Q_EMIT pageCountChanged();
    Q_EMIT editedChanged();
    setSelection(0, 0);
}

/**
 * Returns target page number or -1 if move can't be performed.
 */
int PdfEditModel::movePage(int fromPage, int toPage)
{
    if (fromPage < 0 || fromPage >= m_pages || toPage < 0 || toPage >= m_pages)
        return -1;
    if (fromPage == toPage)
        return -1;
    int off = 0;
    const int pageDiff = toPage - fromPage;
    if (pageDiff >= 1)
        off = 1;
    if (!beginMoveRows(QModelIndex{}, fromPage, fromPage, QModelIndex{}, toPage + off))
        return -1;
    m_pageList.move(fromPage, toPage);
    endMoveRows();
    const int startPage = qMin(fromPage, toPage);
    const int endPage = qMax(fromPage, toPage);
    // update all affected pages
    Q_EMIT dataChanged(index(startPage, 0), index(endPage, 0));
    m_wasMoved = true;
    Q_EMIT editedChanged();
    return toPage + off;
}

void PdfEditModel::movePages(const PageRange &range, int targetPage)
{
    if (rangeIsInvalid(range))
        return;

    const int from = range.from() - 1;
    const int to = range.to() - 1;
    int targetNr = qAbs(targetPage);
    if (targetNr > from)
        targetNr = targetNr - (to - from) - 1;
    if (targetPage < 0)
        targetNr--;
    if (targetNr < 0) {
        qCDebug(KARP_LOG) << "PdfEditModel" << "Wrong target page:" << targetNr << "FIXME!";
        return;
    }
    int rowTarget = qAbs(targetPage);
    if (targetPage < 0)
        rowTarget = qMax(rowTarget - 1, 0);
    if (!beginMoveRows(QModelIndex{}, from, to, QModelIndex{}, rowTarget)) {
        qCDebug(KARP_LOG) << "PdfEditModel" << "Cannot start begin move:" << from << to << rowTarget << "FIXME!";
        return;
    }
    // 1. take pages which are going to be moved
    QVector<PdfPage *> pagesToMove;
    for (int p = from; p <= to; ++p) {
        pagesToMove << m_pageList.takeAt(from);
    }
    // 2. Insert selected pages after or before target page
    for (int p = from; p <= to; ++p) {
        m_pageList.insert(targetNr + (p - from), pagesToMove.takeFirst());
    }
    endMoveRows();
    const int startPage = qMin(from, targetNr);
    const int endPage = qMax(to, targetNr + (to - from));
    Q_EMIT dataChanged(index(startPage, 0), index(endPage, 0));
    // update selected page numbers
    const int moveTarget = rowTarget + 1;
    const int selectedPageCount = m_pageRange.pageCount();
    if (moveTarget < m_pageRange.from())
        setSelection(moveTarget, moveTarget + selectedPageCount - 1);
    else
        setSelection(moveTarget - selectedPageCount, moveTarget - 1);
    m_wasMoved = true;
    Q_EMIT editedChanged();
}

void PdfEditModel::moveSelected(int targetPage)
{
    if (targetPage >= m_pageRange.from() && targetPage <= m_pageRange.to()) {
        // qCDebug(KARP_LOG) << "[PdfEditModel]" << "Cannot move selection!" << targetPage << m_pageRange.from() << m_pageRange.to();
        return;
    }
    movePages(m_pageRange, targetPage);
}

/**
 * WARNING: be careful with page numbers!
 * Range starts from 1, @p m_pageList starts from 0
 */
void PdfEditModel::selectPage(int pageNr, bool selected, bool append)
{
    if (pageNr < 0 || pageNr >= m_pages)
        return;

    const int pageToSelect = pageNr + 1;
    if (append) { // multi-page mode
        if (selected) {
            const int from = m_pageRange.isValid() ? qMin(pageToSelect, m_pageRange.from()) : pageToSelect;
            setSelection(from, qMax(m_pageRange.to(), pageToSelect));
        } else { // deselect
            if (pageToSelect == m_pageRange.from()) {
                if (pageToSelect == m_pageRange.to())
                    setSelection(0, 0);
                else
                    setSelection(pageToSelect + 1, m_pageRange.to());
            } else {
                if (pageToSelect == m_pageRange.to())
                    setSelection(m_pageRange.from(), pageToSelect - 1);
                else
                    setSelection(pageToSelect, m_pageRange.to());
            }
        }
    } else { // single page
        if (selected)
            setSelection(pageToSelect, pageToSelect);
        else
            setSelection(0, 0);
    }
}

QString PdfEditModel::getMetaDataKey(int keyId)
{
    static const KLazyLocalizedString fNames[]{kli18n("Title"),
                                               kli18n("Subject"),
                                               kli18n("Author"),
                                               kli18n("Keyword"),
                                               kli18n("Producer"),
                                               kli18n("Creator"),
                                               kli18n("Creation Date"),
                                               kli18n("Modification Date")};

    if (keyId < 0 || keyId > 7)
        return QString();
    return KLocalizedString(fNames[keyId]).toString();
}

QVariantList PdfEditModel::getMetaDataModel(int fileId) const
{
    QVariantList mdm;
    if (m_pdfList.isEmpty() || fileId >= pdfCount() || fileId < 0)
        return mdm;

    auto pdf = m_pdfList[fileId];
    for (int i = 0; i <= static_cast<int>(QPdfDocument::MetaDataField::ModificationDate); ++i) {
        auto fieldType = static_cast<QPdfDocument::MetaDataField>(i);
        mdm << pdf->metaData(fieldType);
    }
    return mdm;
}

QVariantList PdfEditModel::getTargetMetaData() const
{
    return m_metaData->model();
}

void PdfEditModel::setTargetMetaData(const QVariantList &metaList)
{
    m_metaData->setData(metaList);
    Q_EMIT editedChanged();
}

PdfMetaData *PdfEditModel::metaData()
{
    return m_metaData;
}

void PdfEditModel::generate()
{
    if (m_pdfList.isEmpty() || m_pageList.isEmpty()) {
        setProgress(1.0);
        Q_EMIT pdfGenerated();
        return;
    }

    auto conf = karpConfig::self();
    setProgress(0.05);
    auto pdf = m_pdfList[m_pageList.first()->referenceFile()];
    if (conf->askForOutFile()) {
        QFileInfo inInfo(pdf->filePath());
        m_outFile = QFileDialog::getSaveFileName(nullptr, i18n("PDF file to edit"), inInfo.filePath(), u"*.pdf"_s);
        if (m_outFile.isEmpty()) {
            qCDebug(KARP_LOG) << "[PdfEditModel]" << "Output file not provided!";
            setProgress(1.0);
            Q_EMIT pdfGenerated();
            return;
        }
    } else {
        m_outFile = pdf->filePath();
        // takes first file name and adds prefix/suffix
        if (conf->appendXfix())
            m_outFile.insert(m_outFile.length() - 4, conf->outFileXfix());
        else {
            int sepPos = m_outFile.lastIndexOf(QDir::separator());
            m_outFile.insert(sepPos + 1, conf->outFileXfix());
        }
    }

    auto qpdf = new QpdfProxy(this);
    connect(qpdf, &QpdfProxy::finished, this, [=] {
        qpdf->deleteLater();
        if (m_reduceSize) {
            setProgress(0.1);
            auto tools = ToolsThread::self();
            connect(tools, &ToolsThread::progressChanged, this, &PdfEditModel::toolProgressSlot);
            tools->resizeByGs(m_outFile, m_pages);
        } else {
            setProgress(1.0);
            Q_EMIT pdfGenerated();
        }
    });
    qpdf->doJob();
}

void PdfEditModel::cancel()
{
    ToolsThread::self()->cancel();
}

void PdfEditModel::clearAll()
{
    beginResetModel();
    qDeleteAll(m_pageList);
    m_pageList.clear();
    qDeleteAll(m_deletedList);
    m_deletedList.clear();
    qDeleteAll(m_pdfList);
    m_pdfList.clear();
    m_columns = INIT_COLUMN_COUNT;
    m_pages = 0;
    m_rotatedCount = 0;
    m_wasMoved = false;
    m_pageRange.reset();
    endResetModel();
    m_bookmarks->clear();
    Q_EMIT pageCountChanged();
    Q_EMIT pdfCountChanged();
    Q_EMIT editedChanged();
    setOptimizeImages(false);
    setReduceSize(false);
}

int PdfEditModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_pages;
}

QColor PdfEditModel::labelColor(int fileId)
{
    return m_labelColors[fileId % m_labelColors.count()];
}

void PdfEditModel::setPdfPassword(int fileId, const QString &pass)
{
    if (fileId < 0 || fileId >= pdfCount())
        return;
    auto pdf = m_pdfList[fileId];
    pdf->setPassword(pass);
    pdf->setFile(pdf->filePath());
    if (pdf->error() != QPdfDocument::Error::None) {
        qCDebug(KARP_LOG) << "[PdfEditModel] Wrong password!" << pdf->error();
        m_pdfList.remove(fileId);
        pdf->deleteLater();
    } else {
        appendPdfPages(pdf);
    }
}

QAbstractItemModel *PdfEditModel::getBookmarkModel()
{
    return m_bookmarks;
}

QString PdfEditModel::outFile() const
{
    return m_outFile;
}

void PdfEditModel::saveBookmarks(QPDF &qpdf)
{
    m_bookmarks->saveBookmarks(qpdf);
}

QVariant PdfEditModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_pages)
        return QVariant();
    int pageNr = index.row();
    PdfFile *pdf = nullptr;
    PdfPage *page = nullptr;
    if (pageNr < m_pages) {
        page = m_pageList.at(pageNr);
        int refFileId = page->referenceFile();
        if (refFileId < pdfCount())
            pdf = m_pdfList[refFileId];
    }
    switch (role) {
    case RoleImage: {
        if (!pdf || !page)
            return QVariant::fromValue(QImage());
        if (page->nullImage() && pdf) {
            // TODO: find current screen
            QSizeF pSize = pdf->pagePointSize(page->origPage());
            qreal pageRatio = pSize.height() / pSize.width();
            pdf->requestPage(page, QSize(m_prefPageWidth, qFloor(m_prefPageWidth * pageRatio)), pageNr);
        }
        return QVariant::fromValue(page->image());
    }
    case RoleRotated: {
        return page ? page->rotated() : 0;
    }
    case RoleOrigNr:
        return page ? page->origPage() : 0;
    case RolePageNr:
        return pageNr;
    case RolePageRatio: {
        if (!pdf || !page)
            return 1;
        auto pageSize = pdf->pagePointSize(page->origPage());
        return pageSize.height() / pageSize.width();
    }
    case RoleFileId:
        return page ? page->referenceFile() : 0;
    case RoleSelected:
        return page ? page->selected() : false;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{RoleImage, "pageImg"_ba},
            {RoleRotated, "rotated"_ba},
            {RoleOrigNr, "origPage"_ba},
            {RolePageNr, "pageNr"_ba},
            {RolePageRatio, "pageRatio"_ba},
            {RoleFileId, "fileId"_ba},
            {RoleSelected, "selected"_ba}};
}

Qt::ItemFlags PdfEditModel::flags(const QModelIndex &index) const
{
    int pageNr = index.row() * m_columns + index.column();
    if (pageNr < m_pages)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    return Qt::NoItemFlags;
}

void PdfEditModel::changeColumnCount(int colCnt)
{
    if (colCnt < 1 || colCnt == m_columns)
        return;
    updateMaxPageWidth();
    beginResetModel();
    m_columns = colCnt;
    updateMaxPageWidth();
    endResetModel();
    Q_EMIT columnsChanged();
}

void PdfEditModel::updateMaxPageWidth()
{
    if (m_columns == 0)
        return;
    qreal oldMax = m_maxPageWidth;
    m_maxPageWidth = (m_viewWidth - ((m_columns + 3) * m_spacing)) / static_cast<qreal>(m_columns);
    if (oldMax != m_maxPageWidth)
        Q_EMIT maxPageWidthChanged();
}

void PdfEditModel::pageRenderedSlot(quint16 pageNr, PdfPage *pdfPage)
{
    Q_UNUSED(pdfPage)
    Q_EMIT dataChanged(index(pageNr, 0), index(pageNr, 0), QList<int>() << RoleImage);
    if (pageNr == 0)
        Q_EMIT maxPageWidthChanged();
}

void PdfEditModel::appendPdfFileToModel(PdfFile *pdf)
{
    pdf->setReferenceId(pdfCount());
    m_pdfList << pdf;
    pdf->setState(PdfFile::PdfLoaded); // TODO = handle other states
    connect(pdf, &PdfFile::pageRendered, this, &PdfEditModel::pageRenderedSlot);
    if (pdf->error() == QPdfDocument::Error::IncorrectPassword) {
        // It occurs only when app is called with one file argument
        Q_EMIT passwordRequired(pdf->name(), pdfCount() - 1);
        return;
    }
    appendPdfPages(pdf);
}

void PdfEditModel::appendPdfPages(PdfFile *pdf)
{
    int pagesToAdd = pdf->pageCount();
    for (int i = 0; i < pagesToAdd; ++i) {
        m_pageList << new PdfPage(i, pdf->referenceFileId());
    }
    addPagesToModel(pagesToAdd);
}

void PdfEditModel::prependPdfFileToModel(PdfFile *pdf)
{
    pdf->setReferenceId(pdfCount());
    m_pdfList.append(pdf);
    pdf->setState(PdfFile::PdfLoaded);
    connect(pdf, &PdfFile::pageRendered, this, &PdfEditModel::pageRenderedSlot);
    prependPdfPages(pdf);
}

void PdfEditModel::prependPdfPages(PdfFile *pdf)
{
    int pagesToAdd = pdf->pageCount();
    for (int i = pagesToAdd - 1; i >= 0; --i) {
        m_pageList.prepend(new PdfPage(i, pdf->referenceFileId()));
    }
    beginInsertRows(QModelIndex(), 0, pagesToAdd - 1);
    m_pages += pagesToAdd;
    endInsertRows();
    Q_EMIT pageCountChanged();
    Q_EMIT pdfCountChanged();
    Q_EMIT dataChanged(index(pagesToAdd, 0), index(m_pages - 1, 0));
}

void PdfEditModel::addPagesToModel(int pagesToAdd)
{
    beginInsertRows(QModelIndex(), m_pages, m_pages + pagesToAdd);
    m_pages += pagesToAdd;
    endInsertRows();
    Q_EMIT pageCountChanged();
    Q_EMIT pdfCountChanged();
}

void PdfEditModel::toolProgressSlot(qreal prog)
{
    setProgress(0.1 + 0.9 * prog);
    if (prog >= 1.0) {
        disconnect(ToolsThread::self(), &ToolsThread::progressChanged, this, &PdfEditModel::toolProgressSlot);
        setProgress(1.0);
        QTimer::singleShot(300, this, [=] {
            Q_EMIT pdfGenerated();
        });
    } else if (prog == GS_REDUCE_NOT_WORKED)
        Q_EMIT reductionNotWorked();
}

bool PdfEditModel::rangeIsInvalid(const PageRange &range)
{
    if (range.from() < 1 || range.from() > m_pages || range.to() < 1 || range.to() > m_pages) {
        qCDebug(KARP_LOG) << "[PdfEditModel]" << "Wrong page range! FIXME!" << range.from() << range.to() << range.type() << range.n() << "pages:" << m_pages;
        return true;
    }
    return false;
}

void PdfEditModel::updateCreationTimeInMetadata(PdfFile *pdf)
{
    auto newDateTime = pdf->metaData(QPdfDocument::MetaDataField::CreationDate).toDateTime();
    if (newDateTime.toSecsSinceEpoch() < m_metaData->creationDate().toSecsSinceEpoch())
        m_metaData->setCreationDate(newDateTime, false);
}

void PdfEditModel::setSelection(int from, int to)
{
    if (from == 0 && to == 0) { // reset
        if (!m_pageRange.isValid())
            return; // nothing to do
    } else if (from < 1 || to > m_pages || from > m_pages || to < 1 || from > to) {
        qCDebug(KARP_LOG) << "{PdfEditModel}" << "Wrong page range!" << "from" << from << "to" << to << "pages" << m_pages;
        return;
    }
    // qDebug() << "SET SELECTION" << "from" << from << "to" << to << "pages" << m_pages;
    int updateFrom = 0, updateTo = 0;
    // deselect previously selected pages
    if (m_pageRange.isValid()) {
        updateFrom = m_pageRange.from() - 1;
        updateTo = m_pageRange.to() - 1;
        for (int p = updateFrom; p <= updateTo; ++p) {
            if (p < m_pageList.count()) {
                m_pageList[p]->setSelected(false);
            }
        }
        Q_EMIT dataChanged(index(updateFrom, 0), index(updateTo, 0), QList<int>() << RoleSelected);
    }
    if (from == 0 && to == 0) {
        m_pageRange.reset();
        Q_EMIT selectionChanged();
        return;
    }
    m_pageRange.setRange(from, to);
    updateFrom = from - 1;
    updateTo = to - 1;
    for (int p = updateFrom; p <= updateTo; ++p) {
        m_pageList[p]->setSelected(true);
    }
    Q_EMIT dataChanged(index(updateFrom, 0), index(updateTo, 0), QList<int>() << RoleSelected);
    Q_EMIT selectionChanged();
}

#include "moc_pdfeditmodel.cpp"
