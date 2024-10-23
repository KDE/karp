// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfpage.h"

PdfPage::PdfPage(const QImage &image, quint16 origPage)
    : m_image(image)
    , m_origPage(origPage)
{
}

void PdfPage::setImage(const QImage &img)
{
    m_image = img;
}

void PdfPage::setDeleted(bool isDel)
{
    if (isDel)
        m_flags |= PageDeleted;
    else
        m_flags &= ~PageDeleted;
}

void PdfPage::setRotated(int r)
{
    m_flags &= ~PageRotated270; // reset
    if (r == 90)
        m_flags |= PageRotated90;
    else if (r == 180)
        m_flags |= PageRotated180;
    else if (r == 270)
        m_flags |= PageRotated270;
    // in any other case it is 0
}

qreal PdfPage::ratio() const
{
    if (m_image.isNull())
        return 1.0;
    else
        return static_cast<qreal>(m_image.height()) / static_cast<qreal>(m_image.width());
}
