// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#pragma once

#include <QAbstractTableModel>
#include <QQmlEngine>

class QPdfDocument;

/**
 * @brief @p PdfEditModel handles pages from PDF document
 */
class PdfEditModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(qreal viewWidth READ viewWidth WRITE setViewWidth NOTIFY viewWidthChanged)
    Q_PROPERTY(qreal maxPageWidth READ maxPageWidth NOTIFY maxPageWidthChanged)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(bool edited READ edited NOTIFY editedChanged)
    Q_PROPERTY(QString command READ command NOTIFY commandChanged)
    Q_PROPERTY(bool optimizeImages READ optimizeImages WRITE setOptimizeImages NOTIFY optimizeImagesChanged)
    Q_PROPERTY(bool reduceSize READ reduceSize WRITE setReduceSize NOTIFY reduceSizeChanged)

public:
    explicit PdfEditModel(QObject *parent = nullptr);
    ~PdfEditModel() override;

    Q_INVOKABLE void loadPdfFile(const QString &pdfFile);

    int pageCount() const;

    qreal viewWidth() const;
    void setViewWidth(qreal vw);

    qreal maxPageWidth() const;

    qreal spacing() const;
    void setSpacing(qreal sp);

    bool edited() const;

    QString command() const;
    void setCommand(const QString &cmd);

    bool optimizeImages() const;
    void setOptimizeImages(bool optImgs);

    bool reduceSize() const;
    void setReduceSize(bool redS);

    /**
     * In fact @p zoomIn() and @p zoomOut() change columns number
     */
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();

    /**
     * Maps given page @p nr to origin number
     */
    int map(int nr) const
    {
        return m_pageMap[nr];
    }

    Q_INVOKABLE void addRotation(int pageId, int angle);
    Q_INVOKABLE void addDeletion(int pageId, bool doDel);
    Q_INVOKABLE int addMove(int pageNr, int toPage);

    Q_INVOKABLE QStringList metaDataModel();

    Q_INVOKABLE void generate();

    QPdfDocument *pdfDocument() const;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    enum PdfEditRoles {
        RoleImage = Qt::UserRole,
        RoleRotated,
        RoleDeleted,
        RoleOrigNr,
        RolePageNr,
    };

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void pageCountChanged();
    void viewWidthChanged();
    void maxPageWidthChanged();
    void spacingChanged();
    void editedChanged();
    void commandChanged();
    void optimizeImagesChanged();
    void reduceSizeChanged();

protected:
    QString getPagesForRotation(int angle, const QVector<quint16> &pageList);
    void changeColumnCount(int colCnt);

    /**
     * Page image width according to @p viewWidth(), @p spacing() and @p columnCount()
     */
    void updateMaxPageWidth();

private:
    QString m_pdfFile;
    QPdfDocument *m_pdfDoc = nullptr;
    int m_pages = 0;
    int m_rows = 0;
    int m_columns = 0;
    qreal m_maxPageWidth = 1.0;
    qreal m_viewWidth = 1.0;
    qreal m_spacing = 1.0;
    QString m_command;
    // PDF modifications
    quint16 m_rotatedCount = 0;
    quint16 *m_rotated = nullptr;
    quint16 m_deletedCount = 0;
    bool *m_deleted = nullptr;
    bool m_wasMoved = false;
    QVector<quint16> m_pageMap;
    bool m_optimizeImages = false;
    bool m_reduceSize = false;
};
