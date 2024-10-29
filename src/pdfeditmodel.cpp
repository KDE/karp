// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfeditmodel.h"
#include "deafedconfig.h"
#include "pdffile.h"
#include <KLazyLocalizedString>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QPdfDocument>
#include <QPdfPageRenderer>
#include <QProcess>
#include <QScreen>
#include <QStandardPaths>

using namespace Qt::Literals::StringLiterals;

#define INIT_COLUM_COUNT (4)

QColor alpha(const QColor &c)
{
    return QColor(c.red(), c.green(), c.blue(), 0x80);
}

PdfEditModel::PdfEditModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_prefPageWidth = qApp->screens().first()->size().width() / 4;
    m_columns = INIT_COLUM_COUNT;
    m_labelColors << alpha(Qt::black) << alpha(Qt::darkMagenta) << alpha(Qt::darkYellow) << alpha(Qt::darkCyan) << alpha(Qt::darkBlue) << alpha(Qt::darkGreen);
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
 * Returns -1 if move can't be performed or target page number.
 */
int PdfEditModel::addMove(int pageNr, int toPage)
{
    if (pageNr < 0 || pageNr >= m_pages || toPage < 0 || toPage >= m_pages)
        return -1;

    m_wasMoved = true;
    if (toPage / m_columns < pageNr / m_columns) // QVector::move method workaround when moving backward from other row
        toPage++;
    m_pageList.move(pageNr, toPage);
    Q_EMIT editedChanged();
    int startPage = qMin(pageNr, toPage);
    int endPage = qMax(pageNr, toPage);
    // update all cells in affected rows
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
    if (m_pdfList.isEmpty() || m_pageList.isEmpty())
        return;

    auto conf = deafedConfig::self();
    if (conf->qpdfPath().isEmpty())
        return;
    // TODO but allow gs if available
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

    QStringList args;
    args << pdf->filePath();
    args << u"--pages"_s << u"."_s;

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
    args << u"--"_s;
    // images optimization
    if (m_optimizeImages) {
        // args << u"-recompress-flate"_s << u"--compression-level=9"_s << u"--compress-streams=y"_s << u"--object-streams=generate"_s;
        args << u"--optimize-images"_s;
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
    setCommand(u"qpdf"_s + u" "_s + args.join(u" "_s));
    qDebug().noquote() << m_command;
    p.setArguments(args);
    p.start();
    p.waitForFinished();
    qDebug().noquote().nospace() << p.readAll();
    p.close();

    if (m_reduceSize && !conf->gsPath().isEmpty()) {
        p.setProgram(conf->gsPath());
        args.clear();
        QString tmpPath = QStandardPaths::standardLocations(QStandardPaths::TempLocation).first() + u"/"_s;
        QFileInfo outInfo(out);
        auto psFile = tmpPath + outInfo.baseName() + u".ps"_s;
        auto outFileSize = outInfo.size();
        // gs -q -dNOPAUSE -dBATCH -P- -dSAFER -sDEVICE=ps2write -sOutputFile=file.ps -c save pop -f input.pdf
        args << u"-q"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-P-"_s << u"-dSAFER"_s << u"-sDEVICE=ps2write"_s << u"-sOutputFile="_s + psFile << u"-c"_s
             << u"save"_s << u"pop"_s << u"-f"_s << out;
        qDebug() << args;
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        // perform pdf2ps - store *.ps file in /tmp
        p.close();
        args.clear();
        // gs -q -P- -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sstdout=%stderr -sOutputFile=file.pdf file.ps
        args << u"-q"_s << u"-P-"_s << u"-dNOPAUSE"_s << u"-dBATCH"_s << u"-sDEVICE=pdfwrite"_s << u"-sstdout=%stderr"_s
             << u"-sOutputFile="_s + tmpPath + outInfo.fileName() << psFile;
        qDebug() << args;
        p.setArguments(args);
        p.start();
        p.waitForFinished();
        p.close();
        outInfo.setFile(tmpPath + outInfo.fileName());
        // qDebug() << outFileSize / 1024 << outInfo.size() / 1024;
        if (outInfo.size() < outFileSize) {
            // override out file with new size, but delete existing file first
            if (QFile::exists(out))
                QFile::remove(out);
            qDebug() << "[PdfEditModel]" << "PDF file size successfully reduced.";
            QFile::copy(outInfo.filePath(), out);
        }
        QFile::remove(psFile); // remove /tmp/file-out.ps
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

QColor PdfEditModel::labelColor(int fileId)
{
    return m_labelColors[fileId % m_labelColors.count()];
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
            return QSize();
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
    Q_EMIT pdfCountChanged();
}

QStringList PdfEditModel::getQPDFargs(const QVector<QVector<quint16>> &chunks)
{
    QStringList args;
    QString rangeArgs;
    for (int c = 0; c < chunks.count(); ++c) {
        auto &fileChunk = chunks[c];
        if (c > 0)
            args << m_pdfList[fileChunk[0]]->filePath();
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

#include "moc_pdfeditmodel.cpp"
