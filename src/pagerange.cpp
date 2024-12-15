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

void PageRange::setRange(int f, int t, int s)
{
    m_from = f;
    m_to = t;
    m_n = s;
}

void PageRange::setTo(int t)
{
    m_to = t;
}

void PageRange::reset()
{
    m_from = 0;
    m_to = 0;
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

void PageRange::setTypeInt(int rType)
{
    m_type = static_cast<RangeType>(rType);
}

bool PageRange::allInRange() const
{
    return m_type == AllInRange;
}

bool PageRange::everyN() const
{
    return m_type == EveryNPage;
}

bool PageRange::allOutOfRange() const
{
    return m_type == AllOutOfRange;
}

#include "moc_pagerange.cpp"
