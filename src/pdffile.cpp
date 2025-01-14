// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdffile.h"
#include "karp_debug.h"
#include "pdfpage.h"
#include <QDir>
#include <QFileInfo>
#include <QPdfPageRenderer>
#include <QThread>
#include <algorithm>

PdfFile::PdfFile(const QString &pdfFileName, quint16 refFileId, PdfFileFlags s)
    : QPdfDocument()
    , m_thread(new QThread)
    , m_refFileId(refFileId)
    , m_state(s)
{
    moveToThread(m_thread);
    connect(m_thread, &QThread::started, this, &PdfFile::threadSlot);
    setFile(pdfFileName);
}

PdfFile::~PdfFile()
{
    m_thread->wait();
    m_thread->deleteLater();
    if (m_renderer)
        m_renderer->deleteLater();
}

void PdfFile::setFile(const QString &fileName)
{
    if (!m_renderer) {
        m_renderer = new QPdfPageRenderer();
        m_renderer->setDocument(this);
        // m_renderer->setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);
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
    if (std::any_of(m_pagesToRender.cbegin(), m_pagesToRender.cend(), [&](const PdfFile::PageToRender &PageToRend) {
            return pdfPage == PageToRend.pdfPage;
        }))
        return;
    m_pagesToRender << PageToRender(pdfPage, pageId, pageSize);
    m_thread->start();
}

void PdfFile::requestPageSlot(int pageNumber, QSize imageSize, const QImage &img)
{
    Q_UNUSED(imageSize);
    auto renderedPage = m_pagesToRender.takeFirst();
    if (pageNumber != renderedPage.pdfPage->origPage())
        qCDebug(KARP_LOG) << "[PdfFile]" << "Wrong page rendered! FIXME!" << pageNumber << renderedPage.pdfPage->origPage();
    renderedPage.pdfPage->setImage(img);
    if (m_pagesToRender.isEmpty()) {
        m_thread->quit();
    } else {
        auto &pageToRender = m_pagesToRender.first();
        m_renderer->requestPage(pageToRender.pdfPage->origPage(), pageToRender.size);
    }
    Q_EMIT pageRendered(renderedPage.pageId, renderedPage.pdfPage);
}

void PdfFile::threadSlot()
{
    if (!m_pagesToRender.isEmpty()) {
        auto &pageToRender = m_pagesToRender.first();
        m_renderer->requestPage(pageToRender.pdfPage->origPage(), pageToRender.size);
    }
}

#include "moc_pdffile.cpp"
