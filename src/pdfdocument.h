// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include "pagerange.h"
#include <QDateTime>
#include <QObject>
#include <poppler/qt6/poppler-qt6.h>

class PdfPage;

#define NO_PAGE_ID (65535)

/**
 * @brief PdfFile
 *
 * Contianer for a PDF document. Has reference to underlying Poppler document.
 */
class PdfDocument : public QObject
{
    Q_OBJECT
public:
    enum PdfFileFlags : quint8 {
        PdfNotAdded = 0, /**< Initial state when file was not yet loaded to the model */
        PdfLoaded,
        PdfAllPages,
        PdfEveryNPage,
    };

    PdfDocument(const QString &pdfFileName, quint16 refFileId, PdfFileFlags s = PdfNotAdded);
    ~PdfDocument();

    void setFile(const QString &fileName, const QByteArray &ownerPassword = QByteArray(), const QByteArray &userPassword = QByteArray());

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

    int pageCount() const
    {
        return m_document->numPages();
    }

    bool isLocked() const
    {
        return m_document->isLocked();
    }

    bool isValid() const
    {
        return m_document != nullptr;
    }

    QSize pageSize(int index) const
    {
        return m_document->page(index)->pageSize();
    }

    QDateTime creationDate() const
    {
        return m_document->creationDate();
    }

    QStringList infoKeys() const
    {
        return m_document->infoKeys();
    }

    QString info(const QString &key) const
    {
        return m_document->info(key);
    }

    QVector<Poppler::OutlineItem> outlines() const
    {
        return m_document->outline();
    }

    /**
     * Request rendering image for @p pdfPage of @p pageId in pages list.
     * Saves rendered @p QImage and emits @p pageRendered() when ready.
     */
    void requestPage(PdfPage *pdfPage, const QSize &pageSize, quint16 pageId);

Q_SIGNALS:
    void pageRendered(quint16, PdfPage *);

private:
    quint16 m_refFileId = 0;
    PdfFileFlags m_state = PdfNotAdded;
    QString m_dir;
    QString m_name;
    PageRange m_range;
    std::unique_ptr<Poppler::Document> m_document;
};
