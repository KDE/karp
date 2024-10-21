// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#pragma once

#include <QThread>

class PdfPage;
class PdfEditModel;

/**
 * @brief RenderThread render PDF page image in separate thread
 */
class RenderThread : public QThread
{
    Q_OBJECT

public:
    explicit RenderThread();

    static void setRenderParams(PdfEditModel *m, int pageWidth);

    void render(int nr, PdfPage *p);

    bool isBusy() const
    {
        return m_busy;
    }

Q_SIGNALS:
    void pageReady(int);

protected:
    void run() override;

private:
    static PdfEditModel *m_model;
    static int m_pageWidth;
    PdfPage *m_page = nullptr;
    quint16 m_nr = 0;
    bool m_busy = false;
};

/**
 * @brief PagesRenderer class maintains rendering PDF pages
 * utilizing available CPU threads
 */
class PagesRenderer : public QObject
{
    Q_OBJECT

public:
    explicit PagesRenderer(PdfEditModel *m, int pageWidth);
    ~PagesRenderer() override;

    /**
     * Terminates all @p RenderThread and calls @p deleteLater()
     */
    void finishExistence();

    void renderPage(int pageNr, PdfPage *p);

Q_SIGNALS:
    void pageRendered(int);

protected:
    void threadSlot();
    void pageReadySlot(int pageNr);

private:
    QThread *m_thread;
    int m_maxThreads = 1;

    /**
     * List of @p QPair where @p first is page number and @p second is @p PdfPage
     */
    QVector<QPair<quint16, PdfPage *>> m_queue;
    QVector<RenderThread *> m_workers;
};
