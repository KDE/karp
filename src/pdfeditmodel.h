// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include "pdfpage.h"
#include <QAbstractTableModel>
#include <QQmlEngine>

class QPdfDocument;
class QPdfPageRenderer;
class PdfFile;
class PageRange;

/**
 * @brief @p PdfEditModel handles pages from PDF document
 */
class PdfEditModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(int pdfCount READ pdfCount NOTIFY pdfCountChanged)
    Q_PROPERTY(qreal viewWidth READ viewWidth WRITE setViewWidth NOTIFY viewWidthChanged)
    Q_PROPERTY(qreal maxPageWidth READ maxPageWidth NOTIFY maxPageWidthChanged)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(bool edited READ edited NOTIFY editedChanged)
    Q_PROPERTY(QString command READ command NOTIFY commandChanged)
    Q_PROPERTY(bool optimizeImages READ optimizeImages WRITE setOptimizeImages NOTIFY optimizeImagesChanged)
    Q_PROPERTY(bool reduceSize READ reduceSize WRITE setReduceSize NOTIFY reduceSizeChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)

public:
    explicit PdfEditModel(QObject *parent = nullptr);
    ~PdfEditModel() override;

    Q_INVOKABLE void loadPdfFile(const QString &pdfFile);

    void addPdfs(QVector<PdfFile *> &pdfList);

    int pageCount() const;

    QVector<PdfFile *> &pdfs();
    int pdfCount() const
    {
        return m_pdfList.count();
    }
    Q_INVOKABLE QString getPdfName(int fileId);

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

    qreal progress() const;
    void setProgress(qreal prog);

    /**
     * In fact @p zoomIn() and @p zoomOut() change columns number
     */
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();

    Q_INVOKABLE QDateTime creationDate() const;

    /**
     * Maps given page @p nr to origin number
     */
    int map(int nr) const
    {
        return m_pageList[nr].origPage();
    }

    Q_INVOKABLE void addRotation(int pageId, int angle);
    Q_INVOKABLE void addDeletion(int pageId);
    Q_INVOKABLE void deletePages(const PageRange &range);
    Q_INVOKABLE int addMove(int pageNr, int toPage);

    Q_INVOKABLE QStringList getMetaDataModel(int fileId);

    Q_INVOKABLE void generate();
    Q_INVOKABLE void cancel();

    /**
     * Removes all @p PdfFile instances and clears out the model
     */
    Q_INVOKABLE void clearAll();

    Q_INVOKABLE QColor labelColor(int fileId);

    Q_INVOKABLE void setPdfPassword(int fileId, const QString &pass);

    PdfPage *page(int p)
    {
        return &m_pageList[p];
    }

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    enum PdfEditRoles {
        RoleImage = Qt::UserRole,
        RoleRotated,
        RoleOrigNr,
        RolePageNr,
        RolePageRatio,
        RoleFileId,
    };

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void pageCountChanged();
    void pdfCountChanged();
    void viewWidthChanged();
    void maxPageWidthChanged();
    void spacingChanged();
    void editedChanged();
    void commandChanged();
    void optimizeImagesChanged();
    void reduceSizeChanged();
    void progressChanged();
    void pdfGenerated();

    /**
     * Emitted when PDF file requires password to open.
     * 1st argument is file name and 2nd is file id in the list
     */
    void passwordRequired(const QString &, int);

protected:
    QString getPagesForRotation(int angle, const QVector<quint16> &pageList);
    void changeColumnCount(int colCnt);

    /**
     * Page image width according to @p viewWidth(), @p spacing() and @p columnCount()
     */
    void updateMaxPageWidth();

    void pageRenderedSlot(quint16 pageNr, PdfPage *pdfPage);

    void addPdfFileToModel(PdfFile *pdf);
    void insertPdfPages(PdfFile *pdf);

    QStringList getQPDFargs(const QVector<QVector<quint16>> &chunks);

    void toolProgressSlot(qreal prog);

Q_SIGNALS:
    void wantRenderImage(int) const;
    void wantRenderPage(int, PdfPage *) const;

private:
    QVector<PdfFile *> m_pdfList;
    QVector<PdfPage> m_pageList;
    int m_pages = 0;
    int m_rows = 0;
    int m_columns = 0;
    qreal m_maxPageWidth = 100.0;
    qreal m_viewWidth = 1.0;
    qreal m_spacing = 1.0;
    QString m_command;
    int m_prefPageWidth = 200;
    // PDF modifications
    quint16 m_rotatedCount = 0;
    QVector<PdfPage> m_deletedList;
    bool m_wasMoved = false;
    bool m_optimizeImages = false;
    bool m_reduceSize = false;
    QVector<QColor> m_labelColors;
    qreal m_progress = 0.0;
};
