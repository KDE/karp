// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "bookmarkmodel.h"
#include "karp_debug.h"
#include "outline.h"
#include <QDebug>
#include <QMetaEnum>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFUsage.hh>

#include <QElapsedTimer>

using namespace Qt::Literals::StringLiterals;
using namespace std::string_literals;

BookmarkModel *BookmarkModel::m_self = nullptr;

BookmarkModel::BookmarkModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new Outline(-1, nullptr))
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

BookmarkModel::Status BookmarkModel::status() const
{
    return m_status;
}

void BookmarkModel::setStatus(Status st)
{
    if (m_status == st)
        return;
    m_status = st;
    Q_EMIT statusChanged();
}

void BookmarkModel::appendPdf(QPdfDocument *pdf)
{
    if (m_pageCount == 0)
        setStatus(Status::Unchanged);
    else
        setStatus(Status::Modified);
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
    setStatus(Status::Modified);
    QPdfBookmarkModel model;
    model.setDocument(pdf);
    m_pageOffset = 0;
    beginResetModel();
    if (rowCount(QModelIndex()) > 0) {
        // increase all page numbers by page number of adding PDF
        iterate(QModelIndex(), [&](const QModelIndex &idx) -> bool {
            const auto n = static_cast<Outline *>(idx.internalPointer());
            if (n)
                n->setPageNumber(n->pageNumber() + pdf->pageCount());
            return false;
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
    setStatus(Status::NoBookmarks);
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
    iterate(QModelIndex(), [&qpdf, &pageRefs, &nodeIds](const QModelIndex &idx) -> bool {
        const Outline *node = static_cast<Outline *>(idx.internalPointer());
        if (!node->parentNode())
            return true;

        auto nodeStream = qpdf.newStream();
        auto nodeDict = QPDFObjectHandle::newDictionary();
        nodeDict.replaceKey("/Count"s, QPDFObjectHandle::newInteger(node->childCount()));
        nodeDict.replaceKey("/Title"s, QPDFObjectHandle::newUnicodeString(node->title().toStdString()));
        auto destArr = QPDFObjectHandle::newArray();
        if (node->pageNumber() < pageRefs.size()) {
            const auto &pos = pageRefs[node->pageNumber()];
            destArr.appendItem(qpdf.getObject(pos.x(), pos.y()));
        } else {
            qCDebug(KARP_LOG) << "[BookmarkModel]" << "Bookmark refers to page which doesn't exist in QPDF!";
        }
        destArr.appendItem(QPDFObjectHandle::newName("/XYZ"s));
        destArr.appendItem(QPDFObjectHandle::newReal(node->location().x()));
        destArr.appendItem(QPDFObjectHandle::newReal(node->location().y()));
        destArr.appendItem(QPDFObjectHandle::newReal(node->zoom()));
        nodeDict.replaceKey("/Dest"s, destArr);
        qpdf.replaceObject(nodeStream.getObjectID(), nodeStream.getGeneration(), nodeDict);
        nodeIds.insert(node, QPoint(nodeStream.getObjectID(), nodeStream.getGeneration()));
        return false;
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
    iterate(QModelIndex(), [&nodeIds, &qpdf, &outlines, &posOfPrev](const QModelIndex &idx) -> bool {
        const Outline *node = static_cast<Outline *>(idx.internalPointer());
        if (!node->parentNode())
            return true;
        auto nodePos = nodeIds.value(node);
        auto nodeObj = qpdf.getObject(nodePos.x(), nodePos.y());
        const auto *const parentNode = node->parentNode();
        auto parentPos = nodeIds.value(parentNode);
        if (parentPos.isNull() || node->level() == 0) {
            nodeObj.replaceKey("/Parent"s, outlines);
        } else {
            auto parentObj = qpdf.getObject(parentPos.x(), parentPos.y());
            nodeObj.replaceKey("/Parent"s, parentObj);
            if (parentNode->childCount()) {
                if (node == parentNode->child(0))
                    parentObj.replaceKey("/First", nodeObj);
                if (node == parentNode->child(parentNode->childCount() - 1))
                    parentObj.replaceKey("/Last", nodeObj);
            }
        }
        if (node->level() == 0) {
            if (!posOfPrev.isNull()) {
                auto prevObj = qpdf.getObject(posOfPrev.x(), posOfPrev.y());
                nodeObj.replaceKey("/Prev", prevObj);
                prevObj.replaceKey("/Next", qpdf.getObject(nodePos.x(), nodePos.y()));
            }
            posOfPrev = nodePos;
        }
        if (node->childCount() > 1) {
            QPoint prevChildPos;
            for (int b = 0; b < node->childCount(); ++b) {
                auto bPos = nodeIds.value(node->child(b));
                auto bObj = qpdf.getObject(bPos.x(), bPos.y());
                if (!prevChildPos.isNull()) {
                    auto prevObj = qpdf.getObject(prevChildPos.x(), prevChildPos.y());
                    bObj.replaceKey("/Prev", prevObj);
                    prevObj.replaceKey("/Next", qpdf.getObject(bPos.x(), bPos.y()));
                }
                prevChildPos = bPos;
            }
        }
        return false;
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
        auto b = new Outline(page, r);
        r->appendChild(b);
        b->setTitle(title);
        // level of top nodes is set to 0 by default
        endInsertRows();
        return;
    }
    auto node = static_cast<Outline *>(idx.internalPointer());
    if (!node) {
        qCDebug(KARP_LOG) << "[BookmarkModel]" << "Cannot get node from index!" << idx;
        return;
    }
    if (w == Insert::Below || w == Insert::Above) {
        auto newB = new Outline(page, node->parentNode());
        newB->setTitle(title);
        newB->setLevel(node->level());
        const int rowNr = node->row() + (w == Insert::Below ? 1 : 0);
        beginInsertRows(idx.parent(), rowNr, rowNr);
        node->parentNode()->insertChild(rowNr, newB);
        endInsertRows();
    } else if (w == Insert::Inside) {
        if (node->childCount())
            qCDebug(KARP_LOG) << "[BookmarkModel]" << "There is sub-chapter already!";
        auto newB = new Outline(page, node);
        newB->setTitle(title);
        newB->setLevel(node->level() + 1);
        beginInsertRows(idx, 0, 0);
        node->appendChild(newB);
        endInsertRows();
    } else if (w == Insert::Edit) {
        Q_EMIT aboutToChange(node, title, page);
        node->setTitle(title);
        node->setPageNumber(page);
        Q_EMIT dataChanged(idx, idx);
    }
    setStatus(Status::Modified);
}

void BookmarkModel::removeOutline(const QModelIndex &idx)
{
    auto node = static_cast<Outline *>(idx.internalPointer());
    if (!node)
        return;
    aboutToRemove(node);
    const int rowToRemove = node->row();
    beginRemoveRows(idx.parent(), rowToRemove, rowToRemove);
    node->parentNode()->removeChild(rowToRemove);
    endRemoveRows();
    setStatus(Status::Modified);
}

QModelIndex BookmarkModel::indexFromOutline(Outline *o)
{
    QModelIndex outlineIndex;
    iterate(QModelIndex(), [&](const QModelIndex &idx) -> bool {
        const Outline *const node = static_cast<Outline *>(idx.internalPointer());
        if (node == o) {
            outlineIndex = idx;
            return true;
        }
        return false;
    });
    return outlineIndex;
}

void BookmarkModel::removePage(int pageNr)
{
    QVector<Outline *> toRemoveList;
    bool wasModified = false;
    iterate(QModelIndex(), [&](const QModelIndex &idx) -> bool {
        Outline *const node = static_cast<Outline *>(idx.internalPointer());
        if (node->pageNumber() == pageNr) {
            toRemoveList << node;
        } else if (node->pageNumber() > pageNr) {
            // decrease every bookmark page number after the number of deleted page.
            // But do not emit aboutToChange - pages with their bookmarks are shifted automatically
            node->setPageNumber(node->pageNumber() - 1);
            Q_EMIT dataChanged(idx, idx, QList<int>() << static_cast<int>(Role::Page));
            wasModified = true;
        }
        return false; // walk through all outlines
    });
    if (!toRemoveList.isEmpty()) {
        for (const auto &outlineToRm : toRemoveList) {
            removeOutline(indexFromOutline(outlineToRm));
        }
    }
    if (wasModified && toRemoveList.isEmpty())
        setStatus(Status::Modified);
}

int BookmarkModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    const Outline *parentNode = nullptr;

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

    const Outline *parentNode;

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

void BookmarkModel::walkThrough(Outline *parentNode, const std::function<void(Outline *)> &funct)
{
    if (parentNode)
        funct(parentNode);

    const auto rows = parentNode->childCount();
    for (int i = 0; i < rows; ++i) {
        walkThrough(parentNode->child(i), funct);
    }
}

void BookmarkModel::addBookmarksFromModel(const QModelIndex &index, const QAbstractItemModel *model, Outline *parentBookmark, bool doPrepend)
{
    Outline *childBookmark = nullptr;
    if (index.isValid()) {
        childBookmark = new Outline(index.data(static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt() + m_pageOffset, parentBookmark);
        childBookmark->grabDataFromIndex(index, m_pageOffset);
        if (parentBookmark) {
            if (doPrepend) {
                if (childBookmark->level() == 0) {
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
    m_counter++;
    Q_EMIT outlineAdded(node);
}

void BookmarkModel::iterate(const QModelIndex &parentIndex, const std::function<bool(const QModelIndex &)> &funct)
{
    if (parentIndex.isValid()) {
        bool doTerminate = funct(parentIndex);
        if (doTerminate)
            return;
    }
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
