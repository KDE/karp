// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfsorganizer.h"
#include "pdfeditmodel.h"
#include <KLocalizedString>
#include <QDebug>
#include <QFileDialog>
#include <QPdfDocument>
#include <QStandardPaths>
#include <QTimer>

using namespace Qt::Literals::StringLiterals;

// ################################################################################################
// ###################            PdfListModel         ############################################
// ################################################################################################
PdfListModel::PdfListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

/**
 * TODO: consider to move this to separate thread
 */
int PdfListModel::appendFile(const QString &pdfFile)
{
    auto newPdf = new PdfFile(pdfFile, m_rows);
    if (!(newPdf->error() == QPdfDocument::Error::None || newPdf->error() == QPdfDocument::Error::IncorrectPassword)) {
        newPdf->deleteLater();
        return 0;
    }
    m_pdfFiles << newPdf;
    beginInsertRows(QModelIndex(), m_rows, m_rows);
    m_rows++;
    endInsertRows();
    return newPdf->pageCount();
}

int PdfListModel::appendPdfFile(PdfFile *pdf)
{
    m_pdfFiles << pdf;
    beginInsertRows(QModelIndex(), m_rows, m_rows);
    m_rows++;
    endInsertRows();
    return pdf->pageCount();
}

int PdfListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_rows;
}

QVariant PdfListModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case RoleDirName:
        return m_pdfFiles[index.row()]->dir();
    case RoleFileName:
        return m_pdfFiles[index.row()]->name();
    case RolePageCount:
        return m_pdfFiles[index.row()]->pageCount();
    case RoleLocked:
        return m_pdfFiles[index.row()]->state() == PdfFile::PdfLoaded;
    case RoleAllPages:
        return m_pdfFiles[index.row()]->range().type() == PageRange::AllInRange;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfListModel::roleNames() const
{
    return {{RoleDirName, "path"}, {RoleFileName, "fileName"}, {RolePageCount, "pageCount"}, {RoleLocked, "locked"}, {RoleAllPages, "selectAll"}};
}

void PdfListModel::move(int fromId, int toId)
{
    if (fromId < 0 || fromId >= m_rows || toId < 0 || toId >= m_rows)
        return;
    m_pdfFiles.move(fromId, toId);
    int startId = qMin(fromId, toId);
    int endId = qMax(fromId, toId);
    for (int f = startId; f <= endId; ++f)
        m_pdfFiles[f]->setReferenceId(f);
    Q_EMIT dataChanged(index(startId, 0), index(endId, 0));
}

void PdfListModel::remove(int fileId)
{
    if (fileId < 0 || fileId >= m_rows)
        return;
    beginRemoveRows(QModelIndex(), fileId, fileId);
    auto toRemove = m_pdfFiles.takeAt(fileId);
    toRemove->deleteLater();
    m_rows--;
    for (int f = fileId; f < m_rows; ++f)
        m_pdfFiles[f]->setReferenceId(f);
    endRemoveRows();
}

int PdfListModel::setPdfPassword(int fileId, const QString &pass)
{
    if (fileId < 0 || fileId >= rows())
        return 0;
    auto pdf = getPdfFile(fileId);
    pdf->setPassword(pass);
    pdf->load(pdf->filePath());
    if (pdf->error() == QPdfDocument::Error::IncorrectPassword) {
        remove(fileId);
        return 0;
    }
    Q_EMIT dataChanged(index(fileId, 0), index(fileId, 0), QList<int>() << RolePageCount);
    return pdf->pageCount();
}
// #################################################################################################
// ###################            PdfsOrganizer         ############################################
// #################################################################################################
PdfsOrganizer::PdfsOrganizer(QObject *parent)
    : QObject(parent)
{
    m_fileModel = new PdfListModel(this);
}

QVariant PdfsOrganizer::editModel()
{
    return QVariant::fromValue(m_editModel);
}

/**
 * Dummy function - no need for this value - just @p setInitFiles() matter
 */
QVariant PdfsOrganizer::initFiles() const
{
    return QVariant();
}

void PdfsOrganizer::setInitFiles(const QVariant &filesVar)
{
    addPdfList(filesVar.toStringList());
}

/**
 * This method has to be called just once - when @p PdfsOrganizer is created.
 * To assign @p PdfEditModel
 */
void PdfsOrganizer::setEditModel(const QVariant &edMod)
{
    if (m_editModel) {
        qDebug() << "[PdfsOrganizer]" << "Edit model already set!";
        return;
    }
    m_editModel = qvariant_cast<PdfEditModel *>(edMod);
    if (m_editModel == nullptr) {
        qDebug() << "[PdfsOrganizer]" << "Wrong PDF Edit model! FIXME";
        return;
    }
    Q_EMIT editModelChanged();

    for (auto &pdfFile : m_editModel->pdfs()) {
        m_totalPages += m_fileModel->appendPdfFile(pdfFile);
    }
    Q_EMIT fileModelChanged();
    Q_EMIT totalPagesChanged();
}

QVariant PdfsOrganizer::fileModel() const
{
    return QVariant::fromValue(m_fileModel);
}

int PdfsOrganizer::totalPages() const
{
    return m_totalPages;
}

bool PdfsOrganizer::addMorePDFs()
{
    QString lastPath;
    if (m_fileModel->rows() > 0)
        lastPath = m_fileModel->lastPdf()->dir();
    if (lastPath.isEmpty())
        lastPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();

    auto pdfList = QFileDialog::getOpenFileNames(nullptr, i18n("Select PDF files"), lastPath, u"*.pdf"_s);
    addPdfList(pdfList);
    return !pdfList.isEmpty();
}

void PdfsOrganizer::applyNewFiles()
{
    QVector<PdfFile *> newFiles;
    for (int p = 0; p < m_fileModel->rows(); ++p) {
        auto pdf = m_fileModel->getPdfFile(p);
        if (pdf->state() == PdfFile::PdfNotAdded)
            newFiles << pdf;
    }
    if (newFiles.isEmpty())
        return;
    m_editModel->addPdfs(newFiles);
}

void PdfsOrganizer::setPdfPassword(int fileId, const QString &pass)
{
    int pagesAdded = m_fileModel->setPdfPassword(fileId, pass);
    if (pagesAdded) {
        m_totalPages += pagesAdded;
        Q_EMIT totalPagesChanged();
        auto passFileId = m_passFiles.takeFirst();
        if (passFileId != fileId)
            qDebug() << "[PdfsOrganizer]" << "Password Ids don't match. FIXME!" << fileId << passFileId;
        if (!m_passFiles.isEmpty()) {
            int nextFileId = m_passFiles.first();
            Q_EMIT passwordRequired(m_fileModel->getPdfFile(nextFileId)->name(), nextFileId);
        }
    }
}

void PdfsOrganizer::addPdfList(const QStringList &pdfList)
{
    for (auto &pdfFile : pdfList) {
        m_totalPages += m_fileModel->appendFile(pdfFile);
        auto pdf = m_fileModel->lastPdf();
        if (pdf->error() == QPdfDocument::Error::IncorrectPassword) {
            bool askForPass = m_passFiles.isEmpty();
            m_passFiles << m_fileModel->rows() - 1;
            if (askForPass) {
                // ask for password with some delay to instantiate all QML stuff
                QTimer::singleShot(200, this, [=] {
                    Q_EMIT passwordRequired(pdf->name(), m_passFiles.first());
                });
            }
        }
    }
    if (!pdfList.isEmpty())
        Q_EMIT totalPagesChanged();
}
