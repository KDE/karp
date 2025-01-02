// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QAbstractItemModel>

class BookmarkNode;
class QPdfDocument;
class QPdfBookmarkModel;
class QPDF;
class QPDFObjectHandle;

/**
 * @brief BookmarkModel class is based on @p QPdfBookmarkModel
 * but it is able to merge bookmarks from many PDF files
 */
class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT

    friend class BookmarkNode;

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

    /**
     * Appends bookmarks (if any) from @p pdf document to the model
     */
    void appendPdf(QPdfDocument *pdf);

    /**
     * Removes all bookmarks/data from the model
     */
    void clear();

    void saveBookmarks(QPDF &qpdf);

    int bokmarksCount() const;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

protected:
    static BookmarkModel *self()
    {
        return m_self;
    }

    /**
     * This method recursively searches @p QPdfBookmarkModel
     * and adds bookmark data to this model
     */
    void findBookmark(const QModelIndex &index, const QAbstractItemModel *model, BookmarkNode *parentBookmark);

    void iterate(const QModelIndex &index, const std::function<void(const QModelIndex &)> &funct);

    void addNode(BookmarkNode *node);

private:
    static BookmarkModel *m_self;
    QScopedPointer<BookmarkNode> m_rootNode;
    QVector<BookmarkNode *> m_nodes;
    QHash<int, QByteArray> m_roleNames;
    int m_pageOffset = 0;
    int m_pagesCount = 0;
    Status m_status = Status::NoBookmarks;
};
