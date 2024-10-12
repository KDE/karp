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
    // TODO: improve scaling
    beginRemoveRows(QModelIndex(), 0, m_rows - 1);
    m_rows = 0;
    endRemoveRows();
    int tmpRowCount = m_pdfDoc->pageCount();
    beginInsertRows(QModelIndex(), 0, tmpRowCount - 1);
    m_rows = tmpRowCount;
    endInsertRows();
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
    case PdfEditImage: {
        QSizeF pSize = m_pdfDoc->pagePointSize(index.row());
        qreal pageRatio = pSize.height() / pSize.width();
        return QVariant::fromValue(m_pdfDoc->render(index.row(), QSize(m_maxPageWidth, qFloor(m_maxPageWidth * pageRatio))));
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{PdfEditImage, "pageImg"}};
}

#include "moc_pdfeditmodel.cpp"
