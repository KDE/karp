// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "bookmarkmodel.h"
#include "karp_debug.h"
#include <QDebug>
#include <QMetaEnum>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFUsage.hh>

#include <QElapsedTimer>

using namespace Qt::Literals::StringLiterals;
using namespace std::string_literals;

/**
 * This class is borrowed from @p QPdfBookmarkModel implementation.
 * It is container for PDF bookmark/outline data
 */
class BookmarkNode
{
public:
    explicit BookmarkNode(BookmarkNode *parentNode = nullptr)
        : m_parentNode(parentNode)
    {
    }

    ~BookmarkNode()
    {
        clear();
    }

    void clear()
    {
        qDeleteAll(m_childNodes);
        m_childNodes.clear();
    }

    void appendChild(BookmarkNode *child)
    {
        m_childNodes.append(child);
    }

    BookmarkNode *child(int row) const
    {
        return m_childNodes.at(row);
    }

    int childCount() const
    {
        return m_childNodes.size();
    }

    int row() const
    {
        if (m_parentNode)
            return m_parentNode->m_childNodes.indexOf(const_cast<BookmarkNode *>(this));

        return 0;
    }

    BookmarkNode *parentNode() const
    {
        return m_parentNode;
    }

    void grabDataFromIndex(const QModelIndex &index, int pageOffset)
    {
        if (!index.isValid())
            return;
        m_title = index.data(static_cast<int>(QPdfBookmarkModel::Role::Title)).toString();
        m_level = index.data(static_cast<int>(QPdfBookmarkModel::Role::Level)).toInt();
        m_pageNumber = index.data(static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt() + pageOffset;
        m_location = index.data(static_cast<int>(QPdfBookmarkModel::Role::Location)).toPointF();
        m_zoom = index.data(static_cast<int>(QPdfBookmarkModel::Role::Zoom)).toReal();
    }

    QString title() const
    {
        return m_title;
    }

    void setTitle(const QString &title)
    {
        m_title = title;
    }

    int level() const
    {
        return m_level;
    }

    void setLevel(int level)
    {
        m_level = level;
    }

    int pageNumber() const
    {
        return m_pageNumber;
    }

    void setPageNumber(int pageNumber)
    {
        m_pageNumber = pageNumber;
    }

    QPointF location() const
    {
        return m_location;
    }

    void setLocation(const QPointF &p)
    {
        m_location = p;
    }

    qreal zoom() const
    {
        return m_zoom;
    }

    void setZoom(qreal zoom)
    {
        m_zoom = zoom;
    }

private:
    QList<BookmarkNode *> m_childNodes;
    BookmarkNode *m_parentNode;

    QString m_title;
    int m_level = 0;
    int m_pageNumber = 0;
    QPointF m_location;
    qreal m_zoom = 0.0;
};

// #################################################################################################
// ###################      class BookmarkModel         ############################################
// #################################################################################################
BookmarkModel::BookmarkModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new BookmarkNode(nullptr))
{
    m_roleNames = QAbstractItemModel::roleNames();
    QMetaEnum rolesMetaEnum = metaObject()->enumerator(metaObject()->indexOfEnumerator("Role"));
    for (int r = Qt::UserRole; r < static_cast<int>(Role::RoleCount); ++r)
        m_roleNames.insert(r, QByteArray(rolesMetaEnum.valueToKey(r)).toLower());
}

BookmarkModel::~BookmarkModel() = default;

void BookmarkModel::appendPdf(QPdfDocument *pdf)
{
    if (m_pagesCount == 0)
        m_status = Status::Unchanged;
    else
        m_status = Status::Modified;
    QPdfBookmarkModel model;
    model.setDocument(pdf);
    m_pageOffset = m_pagesCount;
    beginResetModel();
    findBookmark(QModelIndex(), &model, m_rootNode.data());
    endResetModel();
    m_pageOffset = 0;
    m_pagesCount += pdf->pageCount();
}

void BookmarkModel::clear()
{
    beginResetModel();
    m_rootNode->clear();
    m_pagesCount = 0;
    endResetModel();
    m_status = Status::NoBookmarks;
}

void BookmarkModel::saveBookmarks(QPDF &qpdf)
{
    if (m_status == Status::Removed) {
        auto qpdfRoot = qpdf.getRoot();
        if (qpdfRoot.hasKey("/Outlines"s)) {
            qpdfRoot.replaceKey("/Outlines"s, QPDFObjectHandle::newDictionary());
        }
    } else if (m_status == Status::Modified)
        qCDebug(KARP_LOG) << "{BookmarkModel}" << "Storing modified bookmarks is not implemented yet!";
}

int BookmarkModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    BookmarkNode *parentNode = nullptr;

    if (!parent.isValid())
        parentNode = m_rootNode.data();
    else
        parentNode = static_cast<BookmarkNode *>(parent.internalPointer());

    return parentNode->childCount();
}

int BookmarkModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QModelIndex BookmarkModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    BookmarkNode *parentNode;

    if (!parent.isValid())
        parentNode = m_rootNode.data();
    else
        parentNode = static_cast<BookmarkNode *>(parent.internalPointer());

    BookmarkNode *childNode = parentNode->child(row);
    if (childNode)
        return createIndex(row, column, childNode);
    else
        return QModelIndex();
}

QModelIndex BookmarkModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    const BookmarkNode *childNode = static_cast<BookmarkNode *>(index.internalPointer());
    BookmarkNode *parentNode = childNode->parentNode();

    if (parentNode == m_rootNode.data())
        return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

QVariant BookmarkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const BookmarkNode *node = static_cast<BookmarkNode *>(index.internalPointer());
    switch (static_cast<Role>(role)) {
    case Role::Title:
        return node->title();
    case Role::Level:
        return node->level();
    case Role::Page:
        return node->pageNumber();
    case Role::Location:
        return node->location();
    case Role::Zoom:
        return node->zoom();
    default:
        break;
    }
    if (role == Qt::DisplayRole)
        return node->title();
    return QVariant();
}

QHash<int, QByteArray> BookmarkModel::roleNames() const
{
    return m_roleNames;
}

void BookmarkModel::findBookmark(const QModelIndex &index, const QAbstractItemModel *model, BookmarkNode *parentBookmark)
{
    BookmarkNode *childBookmark = nullptr;
    if (index.isValid()) {
        childBookmark = new BookmarkNode(parentBookmark);
        if (parentBookmark)
            parentBookmark->appendChild(childBookmark);
        childBookmark->grabDataFromIndex(index, m_pageOffset);
    } else {
        childBookmark = parentBookmark;
    }
    if ((index.flags() & Qt::ItemNeverHasChildren) || !model->hasChildren(index))
        return;
    const auto rows = model->rowCount(index);
    const auto cols = model->columnCount(index);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            findBookmark(model->index(i, j, index), model, childBookmark);
        }
    }
}

#include "moc_bookmarkmodel.cpp"
