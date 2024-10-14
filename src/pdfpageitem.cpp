// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "pdfpageitem.h"
#include <QDebug>
#include <QPainter>

PdfPageItem::PdfPageItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

QVariant PdfPageItem::image() const
{
    return QVariant::fromValue(m_image);
}

void PdfPageItem::setImage(QVariant img)
{
    m_image = qvariant_cast<QImage>(img);
    setSize(m_image.size());
    update();
}

void PdfPageItem::paint(QPainter *painter)
{
    painter->fillRect(0, 0, width() - 1, height() - 1, Qt::white);
    painter->drawImage(0, 0, m_image);
}

#include "moc_pdfpageitem.cpp"
