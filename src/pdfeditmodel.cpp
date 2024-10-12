// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "pdfeditmodel.h"
#include <QDebug>
#include <QPdfDocument>

PdfEditModel::PdfEditModel(const QString &pdfFile, QObject *parent)
    : QAbstractListModel(parent)
{
    m_pdfDoc = new QPdfDocument(this);
    auto res = m_pdfDoc->load(pdfFile);
    if (res != QPdfDocument::Error::None) {
        qDebug() << "[PdfEditModel]" << "Cannot load PDF document" << pdfFile;
        return;
    }
    m_rows = m_pdfDoc->pageCount();
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

QVariant PdfEditModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case PdfEditImage:
        return QVariant::fromValue(m_pdfDoc->render(index.row(), QSize(210, 297) * 2));
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{PdfEditImage, "pageImg"}};
}

#include "moc_pdfeditmodel.cpp"
