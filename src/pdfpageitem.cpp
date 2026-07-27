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
    qreal dpr = m_image.devicePixelRatio();
    if (dpr <= 0) {
        dpr = 1.0; // fallback safety
    }
    QSize targetSize(width() * dpr, height() * dpr);

    QImage fittedImage = m_image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    fittedImage.setDevicePixelRatio(dpr);

    qreal xOff = (width() - fittedImage.width() / dpr) / 2;
    qreal yOff = (height() - fittedImage.height() / dpr) / 2;

    painter->drawImage(xOff, yOff, fittedImage);
}

#include "moc_pdfpageitem.cpp"
