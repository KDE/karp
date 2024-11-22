// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfeditmodel.h"
#include "karpconfig.h"
#include "pagerange.h"
#include "pdffile.h"
#include "pdfmetadata.h"
#include "toolsthread.h"
#include <KLazyLocalizedString>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QPdfDocument>
#include <QPdfPageRenderer>
#include <QProcess>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>

using namespace Qt::Literals::StringLiterals;

#define INIT_COLUMN_COUNT (4)

QColor alpha(const QColor &c)
{
    return QColor(c.red(), c.green(), c.blue(), 0x80);
}

PdfEditModel::PdfEditModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_metaData = new PdfMetaData();
    m_prefPageWidth = qApp->screens().first()->size().width() / 4;
    m_columns = INIT_COLUMN_COUNT;
    m_labelColors << alpha(Qt::black) << alpha(Qt::darkMagenta) << alpha(Qt::darkYellow) << alpha(Qt::darkCyan) << alpha(Qt::darkBlue) << alpha(Qt::darkGreen);
}

PdfEditModel::~PdfEditModel()
{
    qDeleteAll(m_pdfList);
    delete m_metaData;
}

void PdfEditModel::loadPdfFile(const QString &pdfFile)
{
    auto newPdf = new PdfFile(pdfFile, pdfCount());
    if (!(newPdf->error() == QPdfDocument::Error::None || newPdf->error() == QPdfDocument::Error::IncorrectPassword)) {
        qDebug() << "[PdfEditModel]" << "Cannot load PDF document" << pdfFile << newPdf->error();
        newPdf->deleteLater();
        return;
    }
    addPdfFileToModel(newPdf);
    karpConfig::self()->setLastDir(m_pdfList.last()->dir());
}

void PdfEditModel::addPdfs(QVector<PdfFile *> &pdfList)
{
    for (auto &pdf : pdfList) {
        addPdfFileToModel(pdf);
    }
    karpConfig::self()->setLastDir(pdfList.last()->dir());
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
        || m_metaData->modified();
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
    m_pageList[pageId].setRotated(angle);
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
        m_pageList[p].setRotated(angle);
        int r = p / m_columns;
        int c = p % m_columns;
        Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleRotated);
        if (angle)
            m_rotatedCount++;
        else
            m_rotatedCount--;
    }
    if (range.allOutOfRange()) {
        from = range.to(); // we start 1 page after the range.to
        to = m_pages;
        for (int p = from; p < to; p += step) {
            m_pageList[p].setRotated(angle);
            int r = p / m_columns;
            int c = p % m_columns;
            Q_EMIT dataChanged(index(r, c), index(r, c), QList<int>() << RoleRotated);
            if (angle)
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
    m_pageList[pageId].setDeleted(true);
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

    // qDebug() << range.from() << range.to() << range.type() << range.n();
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
 * Returns -1 if move can't be performed or target page number.
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
    QVector<PdfPage> pagesToMove;
    for (int p = from; p <= to; ++p) {
        pagesToMove << m_pageList.takeAt(from);
    }
    if (targetNr > from)
        targetNr = targetNr - (to - from) - 1;
    if (targetPage < 0)
        targetNr--;
    if (targetNr < 0) {
        qDebug() << "PdfEditModel" << "Wrong target page:" << targetNr << "FIXME!";
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

QStringList PdfEditModel::getMetaDataModel(int fileId) const
{
    QStringList mdm;
    if (m_pdfList.isEmpty() || fileId >= pdfCount() || fileId < 0)
        return mdm;
    static const KLazyLocalizedString fNames[]{kli18n("Title"),
                                               kli18n("Subject"),
                                               kli18n("Author"),
                                               kli18n("Keyword"),
                                               kli18n("Producer"),
                                               kli18n("Creator"),
                                               kli18n("Creation Date"),
                                               kli18n("Modification Date")};

    auto pdf = m_pdfList[fileId];
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

QStringList PdfEditModel::getTargetMetaData() const
{
    return m_metaData->model();
}

void PdfEditModel::setTargetMetaData(const QVariant &metaList)
{
    m_metaData->setData(metaList.toStringList());
    Q_EMIT editedChanged();
}

void PdfEditModel::generate()
{
    if (m_pdfList.isEmpty() || m_pageList.isEmpty())
        return;

    auto conf = karpConfig::self();
    if (conf->qpdfPath().isEmpty())
        return;
    // TODO but allow gs if available
    setProgress(0.05);
    QString out;
    auto pdf = m_pdfList[m_pageList.first().referenceFile()];
    if (conf->askForOutFile()) {
        QFileInfo inInfo(pdf->filePath());
        out = QFileDialog::getSaveFileName(nullptr, i18n("PDF file to edit"), inInfo.filePath(), u"*.pdf"_s);
    } else {
        out = pdf->filePath();
        out.insert(out.length() - 4, conf->outFileXfix());
    }

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProgram(conf->qpdfPath());

    QString dash = u"--"_s;
    QStringList args;
    args << pdf->filePath();
    if (!pdf->password().isEmpty())
        args << dash + u"password="_s + pdf->password();
    args << dash + u"pages"_s << u"."_s;

    QVector<QVector<quint16>> chunks;
    auto &firstPage = m_pageList.first();
    int fromPage = firstPage.origPage();
    int fileId = firstPage.referenceFile();
    chunks << (QVector<quint16>() << fileId << fromPage);
    for (int p = 1; p < m_pages; ++p) {
        auto nextPage = m_pageList[p];
        if (nextPage.referenceFile() != fileId) {
            fileId = nextPage.referenceFile();
            fromPage = nextPage.origPage();
            chunks << (QVector<quint16>() << fileId << fromPage);
            continue;
        }
        chunks.last() << nextPage.origPage();
    }

    args << getQPDFargs(chunks);
    args << dash;
    // images optimization
    if (m_optimizeImages) {
        args << dash + u"recompress-flate"_s; // << dash + u"compression-level=9"_s << dash + u"compress-streams=y"_s << dash + u"object-streams=generate"_s;
        // args << dash + u"optimize-images"_s;
    }

    if (m_passKey.isEmpty() || m_reduceSize) { // TODO suppress password if reducing size - for now
        // if password is not set for output file, find if component file(s) have
        // and set --decrypt flag to reset any of their password(s)
        bool hasPass = false;
        for (auto &pf : m_pdfList) {
            if (!pf->password().isEmpty()) {
                hasPass = true;
                break;
            }
        }
        if (hasPass)
            args << dash + u"decrypt"_s;
    } else {
        args << dash + u"encrypt"_s << m_passKey << m_passKey << u"256"_s << dash;
    }
    // Rotation of pages - aggregate angles
    // NOTICE: page number (r) refers to number in m_pageList not orig page number in PDF file
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
    qDebug().noquote() << "qpdf" << args.join(u" "_s);
    p.setArguments(args);
    p.start();
    p.waitForFinished();
    qDebug().noquote().nospace() << p.readAll();
    p.close();

    auto tools = ToolsThread::self();
    if (m_reduceSize) {
        connect(tools, &ToolsThread::progressChanged, this, &PdfEditModel::toolProgressSlot);
        tools->resizeByGs(out, m_pages);
        // TODO: after gs manipulations output PDF has no proper metadata and no password
        return;
    } else
        toolProgressSlot(1.0);
    tools->applyMetadata(out, m_metaData);
}

void PdfEditModel::cancel()
{
    ToolsThread::self()->cancel();
}

void PdfEditModel::clearAll()
{
    beginResetModel();
    m_pageList.clear(); // TODO:qDeleteAll when we will switch to pointers
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
        qDebug() << "[PdfEditModel] Wrong password!" << pdf->error();
        m_pdfList.remove(fileId);
        pdf->deleteLater();
    } else {
        insertPdfPages(pdf);
    }
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
        if (role == RoleImage || role == RolePageRatio || role == RoleFileId) {
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
            return 1;
        auto pageSize = pdf->pagePointSize(page->origPage());
        return pageSize.height() / pageSize.width();
    }
    case RoleFileId:
        return pdf ? pdf->referenceFileId() : 0;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{RoleImage, "pageImg"},
            {RoleRotated, "rotated"},
            {RoleOrigNr, "origPage"},
            {RolePageNr, "pageNr"},
            {RolePageRatio, "pageRatio"},
            {RoleFileId, "fileId"}};
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
        pRange.append(QString(u"--"_s + u"rotate=+%1:"_s).arg(angle));
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
    if (pdf->error() == QPdfDocument::Error::IncorrectPassword) {
        // It occurs only when app is called with one file argument
        Q_EMIT passwordRequired(pdf->name(), pdfCount() - 1);
        return;
    }
    insertPdfPages(pdf);
}

void PdfEditModel::insertPdfPages(PdfFile *pdf)
{
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
    Q_EMIT pdfCountChanged();
}

QStringList PdfEditModel::getQPDFargs(const QVector<QVector<quint16>> &chunks)
{
    QStringList args;
    QString rangeArgs;
    for (int c = 0; c < chunks.count(); ++c) {
        auto &fileChunk = chunks[c];
        if (c > 0) {
            auto pdf = m_pdfList[fileChunk[0]];
            args << pdf->filePath();
            if (!pdf->password().isEmpty())
                args << u"--"_s + u"password="_s + pdf->password();
        }
        int fromPage = fileChunk[1];
        int toPage = fromPage + 1;
        rangeArgs = QString::number(fromPage + 1);
        for (int p = 2; p < fileChunk.count(); ++p) {
            if (fileChunk[p] == toPage) {
                if (p == fileChunk.count() - 1) {
                    int span = toPage - fromPage;
                    if (span > 0)
                        rangeArgs.append(QString(u"-%1"_s).arg(fromPage + span + 1));
                }
                toPage++;
            } else {
                int span = toPage - fromPage - 1;
                if (span > 0)
                    rangeArgs.append(QString(u"-%1"_s).arg(fromPage + span + 1));
                fromPage = fileChunk[p];
                toPage = fromPage + 1;
                rangeArgs.append(u","_s);
                rangeArgs.append(QString::number(fromPage + 1));
            }
        }
        args << rangeArgs;
    }
    return args;
}

void PdfEditModel::toolProgressSlot(qreal prog)
{
    setProgress(0.1 + 0.9 * prog);
    if (prog >= 1.0) {
        disconnect(ToolsThread::self(), &ToolsThread::progressChanged, this, &PdfEditModel::toolProgressSlot);
        ToolsThread::self()->applyMetadata(QString(), m_metaData);
        setProgress(1.0);
        QTimer::singleShot(300, this, [=] {
            Q_EMIT pdfGenerated();
        });
    }
}

bool PdfEditModel::rangeIsInvalid(const PageRange &range)
{
    if (range.from() < 1 || range.from() > m_pages || range.to() < 1 || range.to() > m_pages) {
        qDebug() << "[PdfEditModel]" << "Wrong page range! FIXME!" << range.from() << range.to() << range.type() << range.n();
        return true;
    }
    return false;
}

#include "moc_pdfeditmodel.cpp"
