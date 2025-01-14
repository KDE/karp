// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include "pagerange.h"
#include <QPdfDocument>

class QPdfPageRenderer;
class QThread;
class PdfPage;

#define NO_PAGE_ID (65535)

/**
 * @brief PdfFile extends @p QPdfDocument class.
 *
 * It render pages in separate thread with @p QPdfPageRenderer
 * and collects queries for rendering pages.
 */
class PdfFile : public QPdfDocument
{
    Q_OBJECT

public:
    enum PdfFileFlags : quint8 {
        PdfNotAdded = 0, /**< Initial state when file was not yet loaded to the model */
        PdfLoaded,
        PdfAllPages,
        PdfEveryNPage,
    };

    PdfFile(const QString &pdfFileName, quint16 refFileId, PdfFileFlags s = PdfNotAdded);
    ~PdfFile() override;

    void setFile(const QString &fileName);

    quint16 referenceFileId() const
    {
        return m_refFileId;
    }

    void setReferenceId(quint16 refId)
    {
        m_refFileId = refId;
    }

    PdfFileFlags state()
    {
        return m_state;
    }

    void setState(PdfFileFlags s)
    {
        m_state = s;
    }

    /**
     * PDF file path (without name)
     */
    const QString &dir() const
    {
        return m_dir;
    }

    /**
     * PDF File name with .pdf extension
     */
    const QString &name() const
    {
        return m_name;
    }

    QString filePath() const
    {
        return m_dir + m_name;
    }

    PageRange range() const
    {
        return m_range;
    }

    /**
     * Request rendering image for @p pdfPage of @p pageId in pages list.
     * Saves rendered @p QImage and emits @p pageRendered() when ready.
     */
    void requestPage(PdfPage *pdfPage, const QSize &pageSize, quint16 pageId);

Q_SIGNALS:
    void pageRendered(quint16, PdfPage *);

protected:
    void requestPageSlot(int pageNumber, QSize imageSize, const QImage &img);

    struct PageToRender {
        PdfPage *pdfPage = nullptr;
        quint16 pageId = NO_PAGE_ID;
        QSize size;
        PageToRender(PdfPage *p, quint16 id, const QSize &s)
            : pdfPage(p)
            , pageId(id)
            , size(s)
        {
        }
    };

    void threadSlot();

private:
    QThread *m_thread;
    quint16 m_refFileId = 0;
    PdfFileFlags m_state = PdfNotAdded;
    QPdfPageRenderer *m_renderer = nullptr;
    QString m_dir;
    QString m_name;
    PageRange m_range;
    QVector<PageToRender> m_pagesToRender;
};
