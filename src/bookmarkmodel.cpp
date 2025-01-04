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

// #################################################################################################
// ###################         class Outline            ############################################
// #################################################################################################

/**
 * @brief Outline class describes PDF outline/bookmark/chapter.
 * It is also node for Tree View model displaying bookmarks.
 *
 * This class is based from @p QPdfBookmarkModel implementation.
 */
class Outline
{
public:
    explicit Outline(Outline *parentNode = nullptr)
        : m_parentNode(parentNode)
    {
        // register all nodes except root one which is empty
        if (parentNode)
            BookmarkModel::self()->addNode(this);
    }

    ~Outline()
    {
        clear();
    }

    void clear()
    {
        qDeleteAll(m_childNodes);
        m_childNodes.clear();
    }

    void appendChild(Outline *child)
    {
        m_childNodes.append(child);
    }

    void prependChild(Outline *child)
    {
        m_childNodes.prepend(child);
    }

    void insertChild(int index, Outline *child)
    {
        m_childNodes.insert(index, child);
    }

    Outline *child(int row) const
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
            return m_parentNode->m_childNodes.indexOf(const_cast<Outline *>(this));

        return 0;
    }

    Outline *parentNode() const
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

    /**
     * NOTICE
     * Page numbering in PDF starts from 0 in contrary to QPDF where it starts from 1
     */
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
    QVector<Outline *> m_childNodes;
    Outline *m_parentNode;

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
    , m_rootNode(new Outline(nullptr))
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

int BookmarkModel::pageCount() const
{
    return m_pageCount;
}

void BookmarkModel::setPageCount(int pgCnt)
{
    if (m_pageCount == pgCnt)
        return;
    m_pageCount = pgCnt;
    Q_EMIT pageCountChanged();
}

void BookmarkModel::appendPdf(QPdfDocument *pdf)
{
    if (m_pageCount == 0)
        m_status = Status::Unchanged;
    else
        m_status = Status::Modified;
    QPdfBookmarkModel model;
    model.setDocument(pdf);
    m_pageOffset = (m_pageCount > 0 ? m_pageCount - 1 : 0);
    beginResetModel();
    addBookmarksFromModel(QModelIndex(), &model, m_rootNode.data());
    endResetModel();
    m_pageOffset = 0;
    setPageCount(m_pageCount + pdf->pageCount());
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
            const auto n = static_cast<Outline *>(idx.internalPointer());
            if (n)
                n->setPageNumber(n->pageNumber() + pdf->pageCount());
        });
    }
    addBookmarksFromModel(QModelIndex(), &model, m_rootNode.data(), true);
    endResetModel();
    setPageCount(m_pageCount + pdf->pageCount());
}

void BookmarkModel::clear()
{
    beginResetModel();
    m_rootNode->clear();
    m_pageCount = 0;
    endResetModel();
    m_counter = 0;
    m_status = Status::NoBookmarks;
}

/**
 * As for now QPDF doesn't provide methods to manipulate and store bookmarks.
 * We have to manipulate QPDF streams and dictionaries to achieve that.
 */
void BookmarkModel::saveBookmarks(QPDF &qpdf)
{
    if (m_status == Status::NoBookmarks || m_status == Status::Unchanged)
        return;

    auto qpdfRoot = qpdf.getRoot();
    if (m_status == Status::Removed) {
        if (qpdfRoot.hasKey("/Outlines"s)) {
            qpdfRoot.removeKey("/Outlines"s);
        }
        return;
    }

    // Replace /Outlines key with new stream with our new outlines/bookmarks
    auto outlines = QPDFObjectHandle::newDictionary();
    auto outStream = qpdf.newStream();
    qpdfRoot.replaceKey("/Outlines"s, outStream);
    auto outKey = qpdfRoot.getKey("/Outlines"s);
    qpdf.replaceObject(outKey.getObjectID(), outKey.getGeneration(), outlines);

    outlines.replaceKey("/Count"s, QPDFObjectHandle::newInteger(bookmarksCount()));
    outlines.replaceKey("/Type"s, QPDFObjectHandle::newName("/Outlines"s));

    // Store references to all PDF pages in QPDF
    QVector<QPoint> pageRefs; /**< x is object ID y is generation  */
    for (auto &page : qpdf.getAllPages()) {
        pageRefs << QPoint(page.getObjectID(), page.getGeneration());
    }

    QHash<const Outline *, QPoint> nodeIds;
    // Create objects for every outline and set part of its data:
    // the part which doesn't depend on other outlines.
    // Store references to every outline object connected with Outline node classes in QHash list.
    iterate(QModelIndex(), [&](const QModelIndex &idx) {
        const Outline *n = static_cast<Outline *>(idx.internalPointer());
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

    // Set /Firs and /Last references for main outlines (in root node)
    auto rNode = m_rootNode.data();
    const auto posOfFirst = nodeIds.value(rNode->child(0));
    outlines.replaceKey("/First", qpdf.getObject(posOfFirst.x(), posOfFirst.y()));
    const auto posOfLast = nodeIds.value(rNode->child(rNode->childCount() - 1));
    outlines.replaceKey("/Last", qpdf.getObject(posOfLast.x(), posOfLast.y()));
    QPoint posOfPrev;

    // Set keys depend on sibling or parenting outlines: /Prev, /Next, /First, /Last
    // and /Parent for nested bookmarks
    iterate(QModelIndex(), [&](const QModelIndex &idx) {
        const Outline *n = static_cast<Outline *>(idx.internalPointer());
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
}

int BookmarkModel::bookmarksCount() const
{
    return m_counter;
}

void BookmarkModel::insertBookmark(const QModelIndex &idx, int where, const QString &title, int page)
{
    auto w = static_cast<Insert>(where);
    if (w == Insert::AtEnd) {
        const int rowNr = rowCount(idx);
        beginInsertRows(idx, rowNr, rowNr);
        auto r = m_rootNode.data();
        auto b = new Outline(r);
        r->appendChild(b);
        b->setTitle(title);
        b->setPageNumber(page);
        // level of top nodes is set to 0 by default
        endInsertRows();
        return;
    }
    auto b = static_cast<Outline *>(idx.internalPointer());
    if (!b) {
        qCDebug(KARP_LOG) << "[BookmarkModel]" << "Cannot get node from index!";
        return;
    }
    if (w == Insert::Below || w == Insert::Above) {
        auto newB = new Outline(b->parentNode());
        newB->setTitle(title);
        newB->setPageNumber(page);
        newB->setLevel(b->level());
        const int rowNr = b->row() + (w == Insert::Below ? 1 : 0);
        beginInsertRows(idx.parent(), rowNr, rowNr);
        b->parentNode()->insertChild(rowNr, newB);
        endInsertRows();
    } else if (w == Insert::Inside) {
        if (b->childCount())
            qCDebug(KARP_LOG) << "[BookmarkModel]" << "There is sub-chapter already!";
        auto newB = new Outline(b);
        newB->setTitle(title);
        newB->setPageNumber(page);
        newB->setLevel(b->level() + 1);
        beginInsertRows(idx, 0, 0);
        b->appendChild(newB);
        endInsertRows();
    } else if (w == Insert::Edit) {
        b->setTitle(title);
        b->setPageNumber(page);
        Q_EMIT dataChanged(idx, idx);
    }
    m_status = Status::Modified;
}

int BookmarkModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    Outline *parentNode = nullptr;

    if (!parent.isValid())
        parentNode = m_rootNode.data();
    else
        parentNode = static_cast<Outline *>(parent.internalPointer());

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

    Outline *parentNode;

    if (!parent.isValid())
        parentNode = m_rootNode.data();
    else
        parentNode = static_cast<Outline *>(parent.internalPointer());

    auto childNode = parentNode->child(row);
    if (childNode)
        return createIndex(row, column, childNode);
    else
        return QModelIndex();
}

QModelIndex BookmarkModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    const Outline *childNode = static_cast<Outline *>(index.internalPointer());
    Outline *parentNode = childNode->parentNode();

    if (parentNode == m_rootNode.data())
        return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

QVariant BookmarkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Outline *node = static_cast<Outline *>(index.internalPointer());
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

void BookmarkModel::addBookmarksFromModel(const QModelIndex &index, const QAbstractItemModel *model, Outline *parentBookmark, bool doPrepend)
{
    Outline *childBookmark = nullptr;
    if (index.isValid()) {
        childBookmark = new Outline(parentBookmark);
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

void BookmarkModel::addNode(Outline *node)
{
    Q_UNUSED(node)
    m_counter++;
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
