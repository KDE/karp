// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QAbstractItemModel>
#include <QQmlEngine>

class Outline;
class QPdfDocument;
class QPdfBookmarkModel;
class QPDF;
class QPDFObjectHandle;

/**
 * @brief BookmarkModel class is based on @p QPdfBookmarkModel
 * but it is able to merge bookmarks from many PDF files.
 *
 * It implements saving bookmarks/outlines to QPDF class @p saveBookmarks()
 * and allows to insert and edit bookmarks.
 */
class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)

    friend class Outline;

public:
    enum class Role : int {
        Title = Qt::UserRole,
        Level,
        Page,
        Location,
        Zoom,
        RoleCount,
    };
    Q_ENUM(Role)

    /**
     * Possible insert actions for @p insertBookmark()
     */
    enum class Insert : int {
        Edit = 0, /**< Just edits existing bookmark  */
        AtEnd, /**< Appends at the end of root level */
        Above,
        Below,
        Inside, /**< Creates new subsection */
    };
    Q_ENUM(Insert)

    /**
     * Describes status of bookmarks - how they will be stored
     */
    enum class Status : quint8 {
        NoBookmarks = 0, /**< Any added file had no bookmarks */
        Unchanged, /**< Single file and none operation changed bookmarks structure */
        Removed, /**< There were bookmarks but all were removed */
        Modified, /**< bookmarks were modified or merged and have to be saved */
    };

    explicit BookmarkModel(QObject *parent = nullptr);
    ~BookmarkModel() override;

    int pageCount() const;

    Status status() const;

    /**
     * Appends bookmarks (if any) from @p pdf document to the model
     */
    void appendPdf(QPdfDocument *pdf);
    void prependPdf(QPdfDocument *pdf);

    /**
     * Removes all bookmarks/data from the model
     */
    void clear();

    void saveBookmarks(QPDF &qpdf);

    int bookmarksCount() const;

    Q_INVOKABLE void insertBookmark(const QModelIndex &idx, int where, const QString &title, int page);
    Q_INVOKABLE void removeOutline(const QModelIndex &idx);

    QModelIndex indexFromOutline(Outline *o);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void walkThrough(Outline *parentNode, const std::function<void(Outline *)> &funct);

Q_SIGNALS:
    void pageCountChanged();
    void statusChanged();
    void outlineAdded(Outline *);
    void aboutToRemove(Outline *);

    /**
     * Emitted when outline is edited - it will receive new title and/or page
     */
    void aboutToChange(Outline *, const QString &, int);

protected:
    static BookmarkModel *self()
    {
        return m_self;
    }

    void setPageCount(int pgCnt);

    void setStatus(Status st);

    /**
     * This method recursively searches @p QPdfBookmarkModel
     * and adds bookmark data to this model
     */
    void addBookmarksFromModel(const QModelIndex &index, const QAbstractItemModel *model, Outline *parentBookmark, bool doPrepend = false);

    /**
     * Iterates through all outlines in this model
     * and calls @p funct for every @p QModelIndex in the model.
     * if @p funct returns @p false iteration is continued
     * retuning @p true means terminate.
     */
    void iterate(const QModelIndex &index, const std::function<bool(const QModelIndex &)> &funct);

    void addNode(Outline *node);

private:
    static BookmarkModel *m_self;
    QScopedPointer<Outline> m_rootNode;
    quint32 m_counter = 0;
    QHash<int, QByteArray> m_roleNames;
    int m_pageOffset = 0;
    int m_pageCount = 0;
    Status m_status = Status::NoBookmarks;
};
