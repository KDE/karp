// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfpageitem.h"
#include <QPainter>

PdfPageItem::PdfPageItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

QVariant PdfPageItem::image() const
{
    return QVariant::fromValue(m_image);
}

void PdfPageItem::setImage(const QVariant &img)
{
    m_image = qvariant_cast<QImage>(img);
    update();
}

void PdfPageItem::paint(QPainter *painter)
{
    QImage fittedImage = m_image.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    qreal xOff = (width() - fittedImage.width()) / 2;
    qreal yOff = (height() - fittedImage.height()) / 2;

    painter->fillRect(0, 0, width() - 1, height() - 1, Qt::white);
    painter->drawImage(xOff, yOff, fittedImage);
}

#include "moc_pdfpageitem.cpp"
