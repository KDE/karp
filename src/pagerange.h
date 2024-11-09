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
    enum RangeType { AllInRange = 0, EveryNPage, AllOutOfRange };
    Q_ENUM(RangeType)

    int from() const;
    void setFrom(int frm);

    int to() const;
    void setTo(int t);

    int n() const;
    void setN(int nn);

    PageRange::RangeType type() const;
    void setType(RangeType rType);
    void setTypeInt(int rType);

    bool allInRange() const;
    bool everyN() const;
    bool allOutOfRange() const;

private:
    int m_from = 1;
    int m_to = 1;
    int m_n = 1;
    RangeType m_type = AllInRange;
};
