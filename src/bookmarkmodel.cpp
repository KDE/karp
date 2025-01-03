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
        // register all nodes except root one which is empty
        if (parentNode)
            BookmarkModel::self()->addNode(this);
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

    void prependChild(BookmarkNode *child)
    {
        m_childNodes.prepend(child);
    }

    void insertChild(int index, BookmarkNode *child)
    {
        m_childNodes.insert(index, child);
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
    QVector<BookmarkNode *> m_childNodes;
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

BookmarkModel *BookmarkModel::m_self = nullptr;

BookmarkModel::BookmarkModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new BookmarkNode(nullptr))
{
    m_self = this;
    m_roleNames = QAbstractItemModel::roleNames();
    QMetaEnum rolesMetaEnum = metaObject()->enumerator(metaObject()->indexOfEnumerator("Role"));
    for (int r = Qt::UserRole; r < static_cast<int>(Role::RoleCount); ++r)
        m_roleNames.insert(r, QByteArray(rolesMetaEnum.valueToKey(r)).toLower());
}

BookmarkModel::~BookmarkModel()
{
    m_self = nullptr;
}

void BookmarkModel::appendPdf(QPdfDocument *pdf)
{
    if (m_pagesCount == 0)
        m_status = Status::Unchanged;
    else
        m_status = Status::Modified;
    QPdfBookmarkModel model;
    model.setDocument(pdf);
    // if (!model.rowCount()) {
    //     auto r = m_rootNode.data();
    //     auto b1 = new BookmarkNode(r);
    //     b1->setTitle(u"One"_s);
    //     b1->setPageNumber(1);
    //     auto b11 = new BookmarkNode(b1);
    //     b11->setTitle(u"Inside One"_s);
    //     b11->setLevel(1);
    //     b11->setPageNumber(2);
    //     b1->appendChild(b11);
    //     r->appendChild(b1);
    //     auto b2 = new BookmarkNode(r);
    //     b2->setTitle(u"Two"_s);
    //     b2->setPageNumber(3);
    //     r->appendChild(b2);
    // }
    m_pageOffset = (m_pagesCount > 0 ? m_pagesCount - 1 : 0);
    beginResetModel();
    addBookmarksFromModel(QModelIndex(), &model, m_rootNode.data());
    endResetModel();
    m_pageOffset = 0;
    m_pagesCount += pdf->pageCount();
}

void BookmarkModel::prependPdf(QPdfDocument *pdf)
{
    m_status = Status::Modified;
    QPdfBookmarkModel model;
    model.setDocument(pdf);
    m_pageOffset = 0;
    beginResetModel();
    if (rowCount(QModelIndex()) > 0) {
        // increase all page numbers by page number of adding PDF
        iterate(QModelIndex(), [&](const QModelIndex &idx) {
            const auto n = static_cast<BookmarkNode *>(idx.internalPointer());
            if (n)
                n->setPageNumber(n->pageNumber() + pdf->pageCount());
        });
    }
    addBookmarksFromModel(QModelIndex(), &model, m_rootNode.data(), true);
    endResetModel();
    m_pagesCount += pdf->pageCount();
}

void BookmarkModel::clear()
{
    beginResetModel();
    m_rootNode->clear();
    m_pagesCount = 0;
    endResetModel();
    m_nodes.clear();
    m_status = Status::NoBookmarks;
}

void BookmarkModel::saveBookmarks(QPDF &qpdf)
{
    if (m_status == Status::NoBookmarks)
        return;
    // TODO Unchanged

    auto outlines = QPDFObjectHandle::newDictionary();
    auto qpdfRoot = qpdf.getRoot();
    auto outStream = qpdf.newStream();
    qpdfRoot.replaceKey("/Outlines"s, outStream);
    auto outKey = qpdfRoot.getKey("/Outlines"s);
    qpdf.replaceObject(outKey.getObjectID(), outKey.getGeneration(), outlines);

    outlines.replaceKey("/Count"s, QPDFObjectHandle::newInteger(m_nodes.count()));
    outlines.replaceKey("/Type"s, QPDFObjectHandle::newName("/Outlines"s));

    QVector<QPoint> pageRefs;
    for (auto &page : qpdf.getAllPages()) {
        pageRefs << QPoint(page.getObjectID(), page.getGeneration());
    }

    QHash<const BookmarkNode *, QPoint> nodeIds;
    iterate(QModelIndex(), [&](const QModelIndex &idx) {
        const BookmarkNode *n = static_cast<BookmarkNode *>(idx.internalPointer());
        if (!n->parentNode())
            return;

        auto nodeStream = qpdf.newStream();
        auto nodeDict = QPDFObjectHandle::newDictionary();
        nodeDict.replaceKey("/Count"s, QPDFObjectHandle::newInteger(n->childCount()));
        nodeDict.replaceKey("/Title"s, QPDFObjectHandle::newUnicodeString(n->title().toStdString()));
        auto destArr = QPDFObjectHandle::newArray();
        if (n->pageNumber() <= pageRefs.count()) {
            auto &p = pageRefs[n->pageNumber()];
            destArr.appendItem(qpdf.getObject(p.x(), p.y()));
        } else {
            qCDebug(KARP_LOG) << "[BookmarkModel]" << "Bookmark refers to page which doesn't exist in QPDF!";
        }
        destArr.appendItem(QPDFObjectHandle::newName("/XYZ"s));
        destArr.appendItem(QPDFObjectHandle::newReal(n->location().x()));
        destArr.appendItem(QPDFObjectHandle::newReal(n->location().y()));
        destArr.appendItem(QPDFObjectHandle::newReal(n->zoom()));
        nodeDict.replaceKey("/Dest"s, destArr);
        qpdf.replaceObject(nodeStream.getObjectID(), nodeStream.getGeneration(), nodeDict);
        nodeIds.insert(n, QPoint(nodeStream.getObjectID(), nodeStream.getGeneration()));
    });

    auto rNode = m_rootNode.data();
    const auto posOfFirst = nodeIds.value(rNode->child(0));
    outlines.replaceKey("/First", qpdf.getObject(posOfFirst.x(), posOfFirst.y()));
    const auto posOfLast = nodeIds.value(rNode->child(rNode->childCount() - 1));
    outlines.replaceKey("/Last", qpdf.getObject(posOfLast.x(), posOfLast.y()));
    QPoint posOfPrev;

    iterate(QModelIndex(), [&](const QModelIndex &idx) {
        const BookmarkNode *n = static_cast<BookmarkNode *>(idx.internalPointer());
        if (!n->parentNode())
            return;
        auto nodePos = nodeIds.value(n);
        auto nodeObj = qpdf.getObject(nodePos.x(), nodePos.y());
        auto parentNode = n->parentNode();
        auto parentPos = nodeIds.value(parentNode);
        if (parentPos.isNull() || n->level() == 0) {
            nodeObj.replaceKey("/Parent"s, outlines);
        } else {
            auto parentObj = qpdf.getObject(parentPos.x(), parentPos.y());
            nodeObj.replaceKey("/Parent"s, parentObj);
            if (parentNode->childCount()) {
                if (n == parentNode->child(0))
                    parentObj.replaceKey("/First", nodeObj);
                if (n == parentNode->child(parentNode->childCount() - 1))
                    parentObj.replaceKey("/Last", nodeObj);
            }
        }
        if (n->level() == 0) {
            if (!posOfPrev.isNull()) {
                auto prevObj = qpdf.getObject(posOfPrev.x(), posOfPrev.y());
                nodeObj.replaceKey("/Prev", prevObj);
                prevObj.replaceKey("/Next", qpdf.getObject(nodePos.x(), nodePos.y()));
            }
            posOfPrev = nodePos;
        }
        if (n->childCount() > 1) {
            QPoint prevChildPos;
            for (int b = 0; b < n->childCount(); ++b) {
                auto bPos = nodeIds.value(n->child(b));
                auto bObj = qpdf.getObject(bPos.x(), bPos.y());
                if (!prevChildPos.isNull()) {
                    auto prevObj = qpdf.getObject(prevChildPos.x(), prevChildPos.y());
                    bObj.replaceKey("/Prev", prevObj);
                    prevObj.replaceKey("/Next", qpdf.getObject(bPos.x(), bPos.y()));
                }
                prevChildPos = bPos;
            }
        }
    });

    if (m_status == Status::Removed) {
        if (qpdfRoot.hasKey("/Outlines"s)) {
            qpdfRoot.removeKey("/Outlines"s);
        }
    }
}

int BookmarkModel::bokmarksCount() const
{
    return m_nodes.count();
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

void BookmarkModel::addBookmarksFromModel(const QModelIndex &index, const QAbstractItemModel *model, BookmarkNode *parentBookmark, bool doPrepend)
{
    BookmarkNode *childBookmark = nullptr;
    if (index.isValid()) {
        childBookmark = new BookmarkNode(parentBookmark);
        childBookmark->grabDataFromIndex(index, m_pageOffset);
        if (parentBookmark) {
            if (doPrepend) {
                if (childBookmark->level() == 0) {
                    qDebug() << "Prepend" << index.row() << childBookmark->title();
                    parentBookmark->insertChild(index.row(), childBookmark);
                } else
                    parentBookmark->appendChild(childBookmark);
            } else
                parentBookmark->appendChild(childBookmark);
        }
    } else {
        childBookmark = parentBookmark;
    }
    if ((index.flags() & Qt::ItemNeverHasChildren) || !model->hasChildren(index))
        return;
    const auto rows = model->rowCount(index);
    const auto cols = model->columnCount(index);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            addBookmarksFromModel(model->index(i, j, index), model, childBookmark, doPrepend);
        }
    }
}

void BookmarkModel::addNode(BookmarkNode *node)
{
    m_nodes << node;
}

void BookmarkModel::iterate(const QModelIndex &parentIndex, const std::function<void(const QModelIndex &)> &funct)
{
    if (parentIndex.isValid())
        funct(parentIndex);
    if ((parentIndex.flags() & Qt::ItemNeverHasChildren) || !hasChildren(parentIndex))
        return;
    const auto rows = rowCount(parentIndex);
    const auto cols = columnCount(parentIndex);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            iterate(index(i, j, parentIndex), funct);
        }
    }
}

#include "moc_bookmarkmodel.cpp"
