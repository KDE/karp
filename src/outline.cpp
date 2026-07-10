// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 by Tomasz Bojczuk <seelook@gmail.com>

#include "outline.h"
#include "bookmarkmodel.h"
#include "karp_debug.h"

Outline::Outline(int page, Outline *parentNode)
    : m_parentNode(parentNode)
    , m_pageNumber(page)
{
    // register all nodes except root one which is empty
    if (parentNode)
        BookmarkModel::self()->registerNode(this);
}

Outline::~Outline()
{
    if (m_parentNode && BookmarkModel::self())
        BookmarkModel::self()->unregisterNode(this);
    clear();
}

void Outline::grabDataPopplerOutline(const Poppler::OutlineItem outline, int level, int pageOffset)
{
    auto destination = outline.destination();
    m_title = outline.name();
    m_level = level;
    m_pageNumber = destination->pageNumber() + pageOffset;
    m_location = QPointF(destination->left(), destination->top());
    m_zoom = destination->zoom();
}

void Outline::clear()
{
    qDeleteAll(m_childNodes);
    m_childNodes.clear();
}

int Outline::row() const
{
    if (m_parentNode)
        return m_parentNode->m_childNodes.indexOf(const_cast<Outline *>(this));

    return 0;
}

void Outline::removeChild(int row)
{
    if (row < 0 || row >= childCount()) {
        qCDebug(KARP_LOG) << "[Outline]" << "Cannot remove row" << row << "which doesn't exist!";
        return;
    }
    auto nodeToRemove = m_childNodes.takeAt(row);
    nodeToRemove->clear();
    delete nodeToRemove;
}

void Outline::fixOutlinePage(int newPage)
{
    if (newPage == m_pageNumber)
        return;
    m_pageNumber = newPage;
    BookmarkModel::self()->updatePageNr(this);
}
