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

    Q_PROPERTY(qreal maxPageWidth READ maxPageWidth WRITE setMaxPageWidth NOTIFY maxPageWidthChanged)
    Q_PROPERTY(bool edited READ edited NOTIFY editedChanged)

public:
    explicit PdfEditModel(const QString &pdfFile = QString(), QObject *parent = nullptr);

    qreal maxPageWidth() const;
    void setMaxPageWidth(qreal maxPW);

    bool edited() const;

    class PageRotation
    {
    public:
        PageRotation(quint16 nr, qint16 ang)
            : pageNr(nr)
            , angle(ang)
        {
        }
        quint16 pageNr = 0;
        qint16 angle = 0;
    };

    Q_INVOKABLE void addRotation(int pageId, int angle);

    QPdfDocument *pdfDocument() const;

    int rowCount(const QModelIndex &parent) const override;

    enum PdfEditRoles {
        PdfEditImage = Qt::UserRole,
    };

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void maxPageWidthChanged();
    void editedChanged();

private:
    QPdfDocument *m_pdfDoc = nullptr;
    int m_rows = 0;
    qreal m_maxPageWidth = 1.0;
    // PDF modifications
    QVector<PageRotation> m_rotations;
};
