// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "pdfsorganizer.h"
#include "pdfeditmodel.h"
#include <KLocalizedString>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPdfDocument>
#include <QStandardPaths>

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
    QFileInfo pdfInfo(pdfFile);
    auto dir = pdfInfo.canonicalPath() + QDir::separator();
    auto name = pdfInfo.fileName();
    QPdfDocument pdfDoc;
    pdfDoc.load(pdfFile);
    quint16 pages = pdfDoc.pageCount();
    m_pdfInfos << PdfFile(dir, name, pages);
    beginInsertRows(QModelIndex(), m_rows, m_rows);
    m_rows++;
    endInsertRows();
    return pages;
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
        return m_pdfInfos[index.row()].dir();
    case RoleFileName:
        return m_pdfInfos[index.row()].name();
    case RolePageCount:
        return m_pdfInfos[index.row()].pages();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfListModel::roleNames() const
{
    return {{RoleDirName, "path"}, {RoleFileName, "fileName"}, {RolePageCount, "pageCount"}};
}

QVector<PdfFile> &PdfListModel::pdfInfos()
{
    return m_pdfInfos;
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
        m_totalPages += m_fileModel->appendFile(pdfFile);
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
    if (!m_fileModel->pdfInfos().isEmpty())
        lastPath = m_fileModel->pdfInfos().last().dir();
    if (lastPath.isEmpty())
        lastPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();

    auto pdfFiles = QFileDialog::getOpenFileNames(nullptr, i18n("Select PDF files to append"), lastPath, u"*.pdf"_s);
    qDebug() << pdfFiles;
    for (auto &pdfFile : pdfFiles) {
        m_totalPages += m_fileModel->appendFile(pdfFile);
    }
    if (!pdfFiles.isEmpty())
        Q_EMIT totalPagesChanged();
    return !pdfFiles.isEmpty();
}
