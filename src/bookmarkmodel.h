// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QAbstractItemModel>

class BookmarkNode;
class QPdfDocument;
class QPdfBookmarkModel;

/**
 * @brief BookmarkModel class is based on @p QPdfBookmarkModel
 * but it is able to merge bookmarks from many PDF files
 */
class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT

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

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

protected:
    /**
     * This method recursively searches @p QPdfBookmarkModel
     * and adds bookmark data to this model
     */
    void findBookmark(const QModelIndex &index, const QAbstractItemModel *model, BookmarkNode *parentBookmark);

private:
    QScopedPointer<BookmarkNode> m_rootNode;
    QHash<int, QByteArray> m_roleNames;
    int m_pageOffset = 0;
    int m_pagesCount = 0;
};
