// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdffile.h"

PdfFile::PdfFile(const QString &d, const QString &n, quint16 p, PdfFileFlags s)
    : m_dir(d)
    , m_name(n)
    , m_pages(p)
    , m_state(s)
{
}
