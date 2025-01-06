// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 by Tomasz Bojczuk <seelook@gmail.com>

#include "outline.h"
#include "bookmarkmodel.h"
#include <QPdfBookmarkModel>

Outline::Outline(int page, Outline *parentNode)
    : m_parentNode(parentNode)
    , m_pageNumber(page)
{
    // register all nodes except root one which is empty
    if (parentNode)
        BookmarkModel::self()->addNode(this);
}

Outline::~Outline()
{
    clear();
}

void Outline::grabDataFromIndex(const QModelIndex &index, int pageOffset)
{
    if (!index.isValid())
        return;
    m_title = index.data(static_cast<int>(QPdfBookmarkModel::Role::Title)).toString();
    m_level = index.data(static_cast<int>(QPdfBookmarkModel::Role::Level)).toInt();
    m_pageNumber = index.data(static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt() + pageOffset;
    m_location = index.data(static_cast<int>(QPdfBookmarkModel::Role::Location)).toPointF();
    m_zoom = index.data(static_cast<int>(QPdfBookmarkModel::Role::Zoom)).toReal();
}

void Outline::clear()
{
    qDeleteAll(m_childNodes);
    m_childNodes.clear();
}
