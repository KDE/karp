// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfpage.h"
#include "outline.h"

PdfPage::PdfPage(const QImage &image, quint16 origPage, quint16 refFile)
    : m_image(image)
    , m_origPage(origPage)
    , m_refFile(refFile)
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

void PdfPage::setSelected(bool isSel)
{
    if (isSel)
        m_flags |= PageSelected;
    else
        m_flags &= ~PageSelected;
}

qreal PdfPage::ratio() const
{
    if (m_image.isNull())
        return 1.0;
    else
        return static_cast<qreal>(m_image.height()) / static_cast<qreal>(m_image.width());
}

void PdfPage::addOutline(Outline *o)
{
    m_outlines << o;
}

void PdfPage::removeOutline(Outline *o)
{
    m_outlines.removeOne(o);
}

Outline *PdfPage::getOutline(int id)
{
    if (id < 0 || id >= m_outlines.size())
        return nullptr;
    return m_outlines[id];
}

QStringList PdfPage::outlineModel() const
{
    if (m_outlines.isEmpty())
        return QStringList();
    QStringList oList;
    for (const auto &o : m_outlines)
        oList << o->title();
    return oList;
}
