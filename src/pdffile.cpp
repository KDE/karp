// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "pdffile.h"

PdfFile::PdfFile(const QString &d, const QString &n, quint16 p, PdfFileFlags s)
    : m_dir(d)
    , m_name(n)
    , m_pages(p)
    , m_state(s)
{
}
