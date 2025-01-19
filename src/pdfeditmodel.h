// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include "pagerange.h"
#include "pdfpage.h"
#include <QAbstractListModel>
#include <QQmlEngine>

class PdfFile;
class PdfMetaData;
class BookmarkModel;
class QPDF;
class Outline;

/**
 * This value of 0.98765 is reduce-size operation progress state.
 * When @p ToolThred emits such a progress it means that
 * PDF resize didn't produce smaller file, so
 * @p PdfEditModel::reductionNotWorked() is emitted then.
 */
#define GS_REDUCE_NOT_WORKED (0.98765)

/**
 * @brief @p PdfEditModel handles pages from PDF document
 */
class PdfEditModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(int pdfCount READ pdfCount NOTIFY pdfCountChanged)
    Q_PROPERTY(int columns READ columns NOTIFY columnsChanged)
    Q_PROPERTY(qreal viewWidth READ viewWidth WRITE setViewWidth NOTIFY viewWidthChanged)
    Q_PROPERTY(qreal maxPageWidth READ maxPageWidth NOTIFY maxPageWidthChanged)
    Q_PROPERTY(qreal maxPageHeight READ maxPageHeight NOTIFY maxPageWidthChanged)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(bool edited READ edited NOTIFY editedChanged)
    Q_PROPERTY(bool optimizeImages READ optimizeImages WRITE setOptimizeImages NOTIFY optimizeImagesChanged)
    Q_PROPERTY(bool reduceSize READ reduceSize WRITE setReduceSize NOTIFY reduceSizeChanged)
    Q_PROPERTY(qreal pdfVersion READ pdfVersion WRITE setPdfVersion NOTIFY pdfVersionChanged)
    Q_PROPERTY(QString passKey READ passKey WRITE setPassKey NOTIFY passKeyChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int firstSelected READ firstSelected NOTIFY selectionChanged)
    Q_PROPERTY(int lastSelected READ lastSelected NOTIFY selectionChanged)

public:
    explicit PdfEditModel(QObject *parent = nullptr);
    ~PdfEditModel() override;

    static PdfEditModel *self()
    {
        return m_self;
    }

    Q_INVOKABLE void loadPdfFile(const QString &pdfFile);

    void prependPdfs(const QVector<PdfFile *> &pdfList);
    void appendPdfs(const QVector<PdfFile *> &pdfList);

    int pageCount() const
    {
        return m_pages;
    }

    QVector<PdfFile *> &pdfs();
    int pdfCount() const
    {
        return m_pdfList.count();
    }
    Q_INVOKABLE QString getPdfName(int fileId);

    int columns() const
    {
        return m_columns;
    }

    qreal viewWidth() const;
    void setViewWidth(qreal vw);

    qreal maxPageWidth() const;
    qreal maxPageHeight() const;

    qreal spacing() const;
    void setSpacing(qreal sp);

    bool edited() const;

    bool optimizeImages() const;
    void setOptimizeImages(bool optImgs);

    bool reduceSize() const;
    void setReduceSize(bool redS);

    qreal pdfVersion() const;
    void setPdfVersion(qreal pV);

    const QString &passKey() const;
    void setPassKey(const QString &pass);

    qreal progress() const;
    void setProgress(qreal prog);

    int firstSelected() const
    {
        return m_pageRange.from();
    }

    int lastSelected() const
    {
        return m_pageRange.to();
    }

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
        return m_pageList[nr]->origPage();
    }

    Q_INVOKABLE void rotatePage(int pageId, int angle);
    Q_INVOKABLE void rotatePages(const PageRange &range, int angle);
    Q_INVOKABLE void deletePage(int pageId);
    Q_INVOKABLE void deletePages(const PageRange &range);
    Q_INVOKABLE int movePage(int pageNr, int toPage);

    /**
     * Moves selected pages in @p range before or after @P targetPage.
     * If @p targetPage is negative range is inserted before this page.
     */
    Q_INVOKABLE void movePages(const PageRange &range, int targetPage);
    Q_INVOKABLE void moveSelected(int targetPage);
    Q_INVOKABLE void selectPage(int pageNr, bool selected, bool append = false);

    Q_INVOKABLE QString getMetaDataKey(int keyId);
    Q_INVOKABLE QVariantList getMetaDataModel(int fileId) const;
    Q_INVOKABLE QVariantList getTargetMetaData() const;
    Q_INVOKABLE void setTargetMetaData(const QVariantList &metaList);
    PdfMetaData *metaData();

    Q_INVOKABLE void generate();
    Q_INVOKABLE void cancel();

    /**
     * Removes all @p PdfFile instances and clears out the model
     */
    Q_INVOKABLE void clearAll();

    Q_INVOKABLE QColor labelColor(int fileId);

    Q_INVOKABLE void setPdfPassword(int fileId, const QString &pass);

    Q_INVOKABLE QAbstractItemModel *getBookmarkModel();

    Q_INVOKABLE QStringList getPageOutlines(int p);

    /**
     * Returns @p index of QModelIndex in @p BookmarkModel of given
     * @p pageNr and @p outlineId in the outline list the page has.
     */
    Q_INVOKABLE QModelIndex indexFromOutline(int pageNr, int outlineId);
    Q_INVOKABLE QString outlineTitle(const QModelIndex &bookmarkModelIndex);
    Q_INVOKABLE int outlinePage(const QModelIndex &bookmarkModelIndex);
    Q_INVOKABLE void insertBookmark(const QModelIndex &idx, int where, const QString &title, int page);
    Q_INVOKABLE void removeOutline(const QModelIndex &bookmarkModelIndex);

    const QString &outFile() const;

    void saveBookmarks(QPDF &qpdf, bool force = false);

    PdfPage *page(int p)
    {
        return m_pageList[p];
    }

    int rowCount(const QModelIndex &parent) const override;

    enum PdfEditRoles {
        RoleImage = Qt::UserRole,
        RoleRotated,
        RoleOrigNr,
        RolePageNr,
        RolePageRatio,
        RoleFileId,
        RoleSelected,
        RoleOutline,
    };

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void pageCountChanged();
    void pdfCountChanged();
    void columnsChanged();
    void viewWidthChanged();
    void maxPageWidthChanged();
    void spacingChanged();
    void editedChanged();
    void optimizeImagesChanged();
    void reduceSizeChanged();
    void pdfVersionChanged();
    void passKeyChanged();
    void progressChanged();
    void pdfGenerated();
    void reductionNotWorked();
    void selectionChanged();

    /**
     * Emitted when PDF file requires password to open.
     * 1st argument is file name and 2nd is file id in the list
     */
    void passwordRequired(const QString &, int);

protected:
    void changeColumnCount(int colCnt);

    /**
     * Page image width according to @p viewWidth(), @p spacing() and @p columnCount()
     */
    void updateMaxPageWidth();

    void pageRenderedSlot(quint16 pageNr, PdfPage *pdfPage);

    void appendPdfFileToModel(PdfFile *pdf);
    void appendPdfPages(PdfFile *pdf);

    void prependPdfFileToModel(PdfFile *pdf);
    void prependPdfPages(PdfFile *pdf);

    void addPagesToModel(int pagesToAdd);

    void toolProgressSlot(qreal prog);

    bool rangeIsInvalid(const PageRange &range);

    /**
     * Checks creation time of added PDF and sets it
     * if it is older than metadata @p PdfMetaData has set
     */
    void updateCreationTimeInMetadata(PdfFile *pdf);

    /**
     * Set selected page range @p from @p to and update pages with @p RoleSelected.
     * When given range is [0 - 0] selection is reset.
     */
    void setSelection(int from, int to);

    void newOutlineSlot(Outline *o);
    void removeOutlineSlot(Outline *o);
    void changeOutlineSlot(Outline *o, const QString &, int newPage); /**< For now we don't use new title */

    /**
     * When page is moved, referring bookmark has to point to new number.
     * This method iterates from @p startPage to @p endPage pages
     * and fixes every outline page number.
     */
    void fixOutlinePages(int startPage, int endPage);

Q_SIGNALS:
    void wantRenderImage(int) const;
    void wantRenderPage(int, PdfPage *) const;

private:
    static PdfEditModel *m_self;
    QVector<PdfFile *> m_pdfList;
    QVector<PdfPage *> m_pageList;
    int m_pages = 0;
    int m_columns = 0;
    qreal m_maxPageWidth = 100.0;
    qreal m_viewWidth = 1.0;
    qreal m_spacing = 1.0;
    int m_prefPageWidth = 200;
    // PDF modifications
    QString m_outFile;
    quint16 m_rotatedCount = 0;
    QVector<PdfPage *> m_deletedList;
    bool m_wasMoved = false;
    bool m_optimizeImages = false;
    bool m_reduceSize = false;
    QVector<QColor> m_labelColors;
    QString m_passKey;
    PdfMetaData *m_metaData = nullptr;
    qreal m_pdfVersion = 0.0; /**< 0.0 for default version from input file */
    qreal m_progress = 0.0;
    PageRange m_pageRange;
    BookmarkModel *m_bookmarks = nullptr;
};
