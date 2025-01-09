// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QImage>

class Outline;

/**
 * @brief PdfPage is container class with information about PDF page.
 */
class PdfPage
{
public:
    explicit PdfPage(const QImage &image, quint16 origPage, quint16 refFile = 0);

    explicit PdfPage(quint16 origPage, quint16 refFile = 0)
        : m_origPage(origPage)
        , m_refFile(refFile)
    {
    }

    QImage image() const
    {
        return m_image;
    }
    void setImage(const QImage &img);

    /**
     * True when image is not set - has null dimension.
     */
    bool nullImage() const
    {
        return m_image.isNull();
    }

    /**
     * Page number in reference file. It doesn't change when pages are moved.
     */
    quint16 origPage() const
    {
        return m_origPage;
    }

    /**
     * Number of the file in the list of PDF files which contains this page
     */
    quint16 referenceFile() const
    {
        return m_refFile;
    }

    void setReferenceFile(quint16 refFile)
    {
        m_refFile = refFile;
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
    void setDeleted(bool isDel);

    bool selected() const
    {
        return m_flags & PageSelected;
    }
    void setSelected(bool isSel);

    qreal ratio() const;

    bool hasOutline() const
    {
        return !m_outlines.isEmpty();
    }

    void addOutline(Outline *o);
    bool removeOutline(Outline *o);

    Outline *getOutline(int id);

    QStringList outlineModel() const;

    /**
     * Page state kept into @p m_flags
     */
    enum PageFlags : quint8 {
        PageUnchanged = 0,
        PageRotated90 = 1,
        PageRotated180 = 2,
        PageRotated270 = 3,
        PageDeleted = 4,
        PageSelected = 16,
    };

private:
    QImage m_image;
    quint16 m_origPage = 0;
    quint16 m_refFile = 0;
    quint8 m_flags = 0;
    QVector<Outline *> m_outlines;
};
