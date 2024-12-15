// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QObject>
#include <QQmlEngine>

/**
 * @brief Describes page range in a PDF document.
 * Starts from 1 - first page. NOTICE: 0 is invalid
 */
class PageRange
{
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(pageRange)

    Q_PROPERTY(int from READ from WRITE setFrom)
    Q_PROPERTY(int to READ to WRITE setTo)
    Q_PROPERTY(int n READ n WRITE setN)
    Q_PROPERTY(int type READ type WRITE setTypeInt)
    Q_PROPERTY(bool allInRange READ allInRange)
    Q_PROPERTY(bool everyN READ everyN)
    Q_PROPERTY(bool allOutOfRange READ allOutOfRange)

public:
    PageRange()
    {
    }

    /**
     * @p RangeType describes which pages are selected in the range
     * @p from() @p to() pages.
     * When @p EveryNPage is set - @p step() is also used
     */
    enum RangeType : quint8 {
        AllInRange = 0,
        EveryNPage,
        AllOutOfRange
    };
    Q_ENUM(RangeType)

    int from() const;
    void setFrom(int frm);

    int to() const;
    void setTo(int t);

    void setRange(int f, int t, int s = 1);

    /**
     * Returns number of pages in this range.
     * NOTICE: Due to @p from() and @p to() are included
     * i.e: range of [1-5] contains 5 pages.
     */
    int pageCount() const
    {
        return m_to - m_from + 1;
    }

    bool isValid() const
    {
        return m_from > 0 && m_to > 0;
    }

    /**
     * Resets to 0 and makes page range invalid.
     */
    void reset();

    int n() const;
    void setN(int nn);

    PageRange::RangeType type() const;
    void setType(RangeType rType);
    void setTypeInt(int rType);

    /**
     * Shortcut for @p RangeType::AllInRange
     * All pages @p from() @p to() included from and to
     */
    bool allInRange() const;

    /**
     * Shortcut for @p RangeType::EveryNPage
     * Every @p n() page in range @p from @p to
     */
    bool everyN() const;

    /**
     * Shortcut for @p RangeType::AllOutOfRange
     * All pages before @p from and after @p to
     */
    bool allOutOfRange() const;

private:
    quint16 m_from = 1;
    quint16 m_to = 1;
    quint16 m_n = 1;
    RangeType m_type = AllInRange;
};
