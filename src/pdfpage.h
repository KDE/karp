// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#pragma once

#include <QImage>

/**
 * @brief PdfPage is container class with informations about PDF page
 */
class PdfPage
{
public:
    PdfPage(const QImage &image, quint16 origPage);
    PdfPage(quint16 origPage)
        : m_origPage(origPage)
    {
    }

    QImage image() const
    {
        return m_image;
    }

    void setImage(const QImage &img);

    bool nullImage() const
    {
        return m_image.isNull();
    }

    void setOrigPage(quint16 origP)
    {
        m_origPage = origP;
    }

    quint16 origPage() const
    {
        return m_origPage;
    }

    int rotated() const
    {
        return (m_flags & PageRotated270) * 90;
    }

    void setRotated(int r);

    bool deleted() const
    {
        return m_flags & PageDeleted;
    }

    void setDeleted(bool isDel)
    {
        if (isDel)
            m_flags |= PageDeleted;
        else
            m_flags &= ~PageDeleted;
    }

    qreal ratio() const;

    enum PageFlags : quint8 {
        PageUnchanged = 0,
        PageRotated90 = 1,
        PageRotated180 = 2,
        PageRotated270 = 3,
        PageDeleted = 4,
    };

private:
    QImage m_image;
    quint16 m_origPage = 0;
    quint8 m_flags = 0;
};
