// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfdocument.h"
#include "pdfpage.h"
#include <QDir>
#include <QFileInfo>

PdfDocument::PdfDocument(const QString &pdfFileName, quint16 refFileId, PdfFileFlags s)
    : m_refFileId(refFileId)
    , m_state(s)
{
    setFile(pdfFileName);
}

PdfDocument::~PdfDocument()
{
}

void PdfDocument::setFile(const QString &fileName, const QByteArray &ownerPassword, const QByteArray &userPassword)
{
    QFileInfo pdfInfo(fileName);
    m_dir = pdfInfo.canonicalPath() + QDir::separator();
    m_name = pdfInfo.fileName();
    m_document = Poppler::Document::load(fileName, ownerPassword, userPassword);
    // TODO: Handle errors
    if (m_document) {
        m_range.setTo(m_document->numPages());
    }
}

void PdfDocument::requestPage(PdfPage *pdfPage, const QSize &pageSize, quint16 pageId)
{
    Q_UNUSED(pageId);
    Q_UNUSED(pageSize)

    if (!m_document) {
        return;
    }
    std::unique_ptr<Poppler::Page> page = m_document->page(pdfPage->origPage());
    if (!page) {
        return;
    }
    QImage image = page->renderToImage();
    pdfPage->setImage(image);
    Q_EMIT pageRendered(pageId, pdfPage);
}

#include "moc_pdfdocument.cpp"
