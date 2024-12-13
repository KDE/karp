// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfeditmodel.h"
#include "karp_debug.h"
#include "karpconfig.h"
#include "pagerange.h"
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
    : QAbstractTableModel(parent)
{
    m_self = this;
    m_metaData = new PdfMetaData();
    m_prefPageWidth = qApp->screens().first()->size().width() / 4;
    m_columns = INIT_COLUMN_COUNT;
    m_labelColors << alpha(Qt::black) << alpha(Qt::darkMagenta) << alpha(Qt::darkYellow) << alpha(Qt::darkCyan) << alpha(Qt::darkBlue) << alpha(Qt::darkGreen);
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
}

int PdfEditModel::pageCount() const
{
    return m_pages;
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
    return m_rotatedCount || !m_deletedList.empty() || m_wasMoved || m_optimizeImages || pdfCount() > 1 || m_reduceSize || !m_passKey.isEmpty()
        || m_metaData->modified() || m_pdfVersion > 0.0 || pdfCount() > 1;
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
    int r = pageId / m_columns;
    int c = pageId % m_columns;
    Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleRotated);
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
        int r = p / m_columns;
        int c = p % m_columns;
        Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleRotated);
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
            int r = p / m_columns;
            int c = p % m_columns;
            Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleRotated);
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
    m_pageList[pageId]->setDeleted(true);
    beginResetModel();
    m_deletedList << m_pageList.takeAt(pageId);
    m_pages--;
    m_rows = m_pages / m_columns + (m_pages % m_columns > 0 ? 1 : 0);
    endResetModel();
    Q_EMIT pageCountChanged();
    Q_EMIT editedChanged();
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
    beginResetModel();
    if (range.allOutOfRange()) {
        // At first, delete what is after the range to preserve numbering at the beginning
        from = range.to();
        to = m_pages;
        for (int p = from; p < to; ++p) {
            m_deletedList << m_pageList.takeAt(from);
            m_pages--;
        }
    }
    from = range.allOutOfRange() ? 0 : range.from() - 1;
    to = range.allOutOfRange() ? range.from() - 2 : range.to() - 1;
    QVector<int> toTakeList;
    for (int p = from; p <= to; p += step) {
        toTakeList << p;
    }
    while (!toTakeList.empty()) {
        m_deletedList << m_pageList.takeAt(toTakeList.takeLast());
        m_pages--;
    }
    endResetModel();
    Q_EMIT pageCountChanged();
    Q_EMIT editedChanged();
}

/**
 * Returns target page number or -1 if move can't be performed.
 */
int PdfEditModel::movePage(int pageNr, int toPage)
{
    if (pageNr < 0 || pageNr >= m_pages || toPage < 0 || toPage >= m_pages)
        return -1;

    m_wasMoved = true;
    if (toPage / m_columns < pageNr / m_columns) // QVector::workaround for 'move' method when moving backward from other row
        toPage++;
    m_pageList.move(pageNr, toPage);
    Q_EMIT editedChanged();
    int startPage = qMin(pageNr, toPage);
    int endPage = qMax(pageNr, toPage);
    // update all cells in affected rows
    Q_EMIT dataChanged(index(startPage / m_columns, 0), index(endPage / m_columns, m_columns - 1));
    return toPage;
}

void PdfEditModel::movePages(const PageRange &range, int targetPage)
{
    if (rangeIsInvalid(range))
        return;

    int from = range.from() - 1;
    int to = range.to() - 1;
    int targetNr = qAbs(targetPage);
    // 1. take pages which are going to be moved
    QVector<PdfPage *> pagesToMove;
    for (int p = from; p <= to; ++p) {
        pagesToMove << m_pageList.takeAt(from);
    }
    if (targetNr > from)
        targetNr = targetNr - (to - from) - 1;
    if (targetPage < 0)
        targetNr--;
    if (targetNr < 0) {
        qCDebug(KARP_LOG) << "PdfEditModel" << "Wrong target page:" << targetNr << "FIXME!";
        return;
    }
    // 2. Insert selected pages after or before target page
    for (int p = from; p <= to; ++p) {
        m_pageList.insert(targetNr + (p - from), pagesToMove.takeFirst());
    }
    int startPage = qMin(from, targetNr);
    int endPage = qMax(to, targetNr + (to - from));
    Q_EMIT dataChanged(index(startPage / m_columns, 0), index(endPage / m_columns, m_columns - 1));
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
    m_rows = 0;
    m_columns = INIT_COLUMN_COUNT;
    m_pages = 0;
    m_rotatedCount = 0;
    m_wasMoved = false;
    endResetModel();
    Q_EMIT pageCountChanged();
    Q_EMIT pdfCountChanged();
    Q_EMIT editedChanged();
    setOptimizeImages(false);
    setReduceSize(false);
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

QString PdfEditModel::outFile() const
{
    return m_outFile;
}

QVariant PdfEditModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows || index.column() < 0 || index.column() >= m_columns)
        return QVariant();
    int pageNr = index.row() * m_columns + index.column();
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
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{RoleImage, QByteArrayLiteral("pageImg")},
            {RoleRotated, QByteArrayLiteral("rotated")},
            {RoleOrigNr, QByteArrayLiteral("origPage")},
            {RolePageNr, QByteArrayLiteral("pageNr")},
            {RolePageRatio, QByteArrayLiteral("pageRatio")},
            {RoleFileId, QByteArrayLiteral("fileId")}};
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
    m_rows = m_pages / m_columns + (m_pages % m_columns > 0 ? 1 : 0);
    endResetModel();
}

void PdfEditModel::updateMaxPageWidth()
{
    if (m_columns == 0)
        return;
    qreal oldMax = m_maxPageWidth;
    m_maxPageWidth = (m_viewWidth - ((m_columns + 1) * m_spacing)) / static_cast<qreal>(m_columns);
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
    addPagesToModel(pagesToAdd);
}

void PdfEditModel::addPagesToModel(int pagesToAdd)
{
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
        qCDebug(KARP_LOG) << "[PdfEditModel]" << "Wrong page range! FIXME!" << range.from() << range.to() << range.type() << range.n();
        return true;
    }
    return false;
}

void PdfEditModel::updateCreationTimeInMetadata(PdfFile *pdf)
{
    auto newDateTime = pdf->metaData(QPdfDocument::MetaDataField::CreationDate).toDateTime();
    if (newDateTime.toSecsSinceEpoch() < m_metaData->creationDate().toSecsSinceEpoch())
        m_metaData->setCreationDate(newDateTime);
}

#include "moc_pdfeditmodel.cpp"
