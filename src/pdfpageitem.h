// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczzuk <seelook@gmail.com>

#pragma once

#include <QImage>
#include <QQmlEngine>
#include <QQuickPaintedItem>

class QPainter;

class PdfPageItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant image READ image WRITE setImage NOTIFY imageChanged)

public:
    explicit PdfPageItem(QQuickItem *parent = nullptr);

    QVariant image() const;
    void setImage(QVariant img);

    void paint(QPainter *painter) override;

Q_SIGNALS:
    void imageChanged();

private:
    QImage m_image;
};
