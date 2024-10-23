// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pagesrenderer.h"
#include "pdfeditmodel.h"
#include "pdfpage.h"
#include <QDebug>
#include <QPdfDocument>

// #################################################################################################
// ###################        class RenderThread       ############################################
// #################################################################################################
PdfEditModel *RenderThread::m_model = nullptr;
int RenderThread::m_pageWidth = 100;

RenderThread::RenderThread()
    : QThread()
{
}

void RenderThread::setRenderParams(PdfEditModel *m, int pageWidth)
{
    m_model = m;
    m_pageWidth = pageWidth;
}

void RenderThread::render(int nr, PdfPage *p)
{
    if (m_busy) {
        qDebug() << "[RenderThread] Rendering in progress!";
        return;
    }
    m_busy = true;
    m_nr = nr;
    m_page = p;
    start();
}

void RenderThread::run()
{
    QElapsedTimer e;
    e.start();
    QSizeF pSize = m_model->pdfDocument()->pagePointSize(m_nr);
    qreal pageRatio = pSize.height() / pSize.width();
    m_page->setImage(m_model->pdfDocument()->render(m_nr, QSize(m_pageWidth, qFloor(m_pageWidth * pageRatio))));
    qDebug() << "Rendering" << m_nr << e.nsecsElapsed() / 1000000.0;
    Q_EMIT pageReady(m_nr);
    m_busy = false;
    // exec();
}

// #################################################################################################
// ###################        class PagesRenderer       ############################################
// #################################################################################################
PagesRenderer::PagesRenderer(PdfEditModel *m, int pageWidth)
    : QObject()
{
    RenderThread::setRenderParams(m, pageWidth);
    m_maxThreads = qMax(1, QThread::idealThreadCount() - 2);
    m_thread = new QThread();
    connect(m_thread, &QThread::started, this, &PagesRenderer::threadSlot);
    moveToThread(m_thread);
    m_thread->start();
}

PagesRenderer::~PagesRenderer()
{
}

void PagesRenderer::finishExistence()
{
    for (auto &w : m_workers) {
        connect(w, &QThread::finished, w, &RenderThread::deleteLater);
        w->quit();
        w->wait();
    }
    connect(m_thread, &QThread::finished, this, &PagesRenderer::deleteLater);
    m_thread->quit();
    m_thread->wait();
}

/**
 * Find free thread to render given page or put the page to @p m_queue
 */
void PagesRenderer::renderPage(int pageNr, PdfPage *p)
{
    bool notStarted = true;
    int cnt = 0;
    for (auto &w : m_workers) {
        if (w->isBusy()) {
            cnt++;
            continue;
        }
        qDebug() << "Render" << cnt << pageNr;
        w->render(pageNr, p);
        notStarted = false;
        break;
    }
    if (notStarted)
        m_queue << QPair<quint16, PdfPage *>(pageNr, p);
}

/**
 * Prepare @p RenderThread instances to utilize available CPU threads.
 */
void PagesRenderer::threadSlot()
{
    for (int r = 0; r < m_maxThreads; ++r) {
        auto w = new RenderThread();
        connect(w, &RenderThread::pageReady, this, &PagesRenderer::pageReadySlot);
        m_workers << w;
    }
}

/**
 * Page was rendered by some @p RenderThread - send a signal.
 * If there are more pages to render in the @p m_queue
 * use @p sender() thread - it is free now - to render next page
 */
void PagesRenderer::pageReadySlot(int pageNr)
{
    Q_EMIT pageRendered(pageNr);
    if (m_queue.isEmpty())
        return;
    auto renderer = qobject_cast<RenderThread *>(sender());
    if (renderer) {
        qDebug() << "renderer" << renderer;
        renderer->quit();
        auto nextPage = m_queue.takeFirst();
        renderer->render(nextPage.first, nextPage.second);
    }
}
