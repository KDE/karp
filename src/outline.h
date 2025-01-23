// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2025 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QModelIndex>
#include <QPointF>
#include <QVector>

/**
 * @brief Outline class describes PDF outline/bookmark/chapter.
 * It is also node for Tree View model displaying bookmarks.
 *
 * This class is based from @p QPdfBookmarkModel implementation.
 */
class Outline
{
public:
    explicit Outline(int page = 0, Outline *parentNode = nullptr);
    ~Outline();

    void clear();

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

    void removeChild(int row);

    Outline *child(int row) const
    {
        return m_childNodes.at(row);
    }

    int childCount() const
    {
        return m_childNodes.size();
    }

    int row() const;

    Outline *parentNode() const
    {
        return m_parentNode;
    }

    void grabDataFromIndex(const QModelIndex &index, int pageOffset);

    const QString &title() const
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

    /**
     * Sets new page number and informs model about it.
     */
    void fixOutlinePage(int newPage);

private:
    QVector<Outline *> m_childNodes;
    Outline *m_parentNode;

    QString m_title;
    int m_level = 0;
    int m_pageNumber = 0;
    QPointF m_location;
    qreal m_zoom = 0.0;
};
