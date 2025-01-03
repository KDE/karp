// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdffile.h"
#include "karp_debug.h"
#include "pdfpage.h"
#include <QDir>
#include <QFileInfo>
#include <QPdfPageRenderer>

PdfFile::PdfFile(const QString &pdfFileName, quint16 refFileId, PdfFileFlags s)
    : QPdfDocument()
    , m_refFileId(refFileId)
    , m_state(s)
{
    setFile(pdfFileName);
}

PdfFile::~PdfFile()
{
    if (m_renderer)
        m_renderer->deleteLater();
}

void PdfFile::setFile(const QString &fileName)
{
    if (!m_renderer) {
        m_renderer = new QPdfPageRenderer();
        m_renderer->setDocument(this);
        m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
        connect(m_renderer, &QPdfPageRenderer::pageRendered, this, &PdfFile::requestPageSlot);
    }
    QFileInfo pdfInfo(fileName);
    m_dir = pdfInfo.canonicalPath() + QDir::separator();
    m_name = pdfInfo.fileName();
    load(fileName);
    m_range.setTo(pageCount());
}

void PdfFile::requestPage(PdfPage *pdfPage, const QSize &pageSize, quint16 pageId)
{
    // if a page is in the queue already - skip render request
    for (auto &p : m_pagesToRender) {
        if (pdfPage == p.pdfPage)
            return;
    }
    m_pagesToRender << PageToRender(pdfPage, pageId, pageSize);
    m_renderer->requestPage(pdfPage->origPage(), pageSize);
}

void PdfFile::requestPageSlot(int pageNumber, QSize imageSize, const QImage &img)
{
    Q_UNUSED(imageSize);
    auto renderedPage = m_pagesToRender.takeFirst();
    if (pageNumber != renderedPage.pdfPage->origPage())
        qCDebug(KARP_LOG) << "[PdfFile]" << "Wrong page rendered! FIXME!" << pageNumber << renderedPage.pdfPage->origPage();
    renderedPage.pdfPage->setImage(img);
    if (!m_pagesToRender.isEmpty()) {
        auto &pageToRender = m_pagesToRender.first();
        m_renderer->requestPage(pageToRender.pdfPage->origPage(), pageToRender.size);
    }
    Q_EMIT pageRendered(renderedPage.pageId, renderedPage.pdfPage);
}

#include "moc_pdffile.cpp"
