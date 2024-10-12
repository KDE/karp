// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

class QPdfDocument;

/**
 * @brief @p PdfEditModel handles pages from PDF document
 */
class PdfEditModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PdfEditModel(const QString &pdfFile = QString(), QObject *parent = nullptr);

    QPdfDocument *pdfDocument() const;

    int rowCount(const QModelIndex &parent) const override;

    enum PdfEditRoles {
        PdfEditImage = Qt::UserRole,
    };

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    QPdfDocument *m_pdfDoc = nullptr;
    int m_rows = 0;
};
