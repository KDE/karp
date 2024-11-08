// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pagerange.h"

int PageRange::from() const
{
    return m_from;
}

void PageRange::setFrom(int frm)
{
    m_from = frm;
}

int PageRange::to() const
{
    return m_to;
}

void PageRange::setTo(int t)
{
    m_to = t;
}

int PageRange::n() const
{
    return m_n;
}

void PageRange::setN(int nn)
{
    m_n = nn;
}

PageRange::RangeType PageRange::type() const
{
    return m_type;
}

void PageRange::setType(RangeType rType)
{
    m_type = rType;
}

int PageRange::typeInt() const
{
    return static_cast<int>(m_type);
}

void PageRange::setTypeInt(int rType)
{
    m_type = static_cast<RangeType>(rType);
}
