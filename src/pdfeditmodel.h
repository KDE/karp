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
    ~PdfEditModel() override;

    qreal maxPageWidth() const;
    void setMaxPageWidth(qreal maxPW);

    bool edited() const;

    Q_INVOKABLE void addRotation(int pageId, int angle);

    Q_INVOKABLE void generate();

    QPdfDocument *pdfDocument() const;

    int rowCount(const QModelIndex &parent) const override;

    enum PdfEditRoles {
        RoleImage = Qt::UserRole,
        RoleRotated,
    };

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void maxPageWidthChanged();
    void editedChanged();

protected:
    QString getPagesForRotation(int angle, const QVector<quint16> &pageList);

private:
    QString m_pdfFile;
    QPdfDocument *m_pdfDoc = nullptr;
    int m_rows = 0;
    qreal m_maxPageWidth = 1.0;
    // PDF modifications
    quint16 *m_rotated = nullptr;
};
