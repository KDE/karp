// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QProcess>
#include <QThread>

/**
 * @brief GsThread combines @p QProcess and @p QThread
 * to perform gs operations on pdf pages using
 * all available cores/threads
 */
class GsThread : public QProcess
{
    Q_OBJECT
public:
    explicit GsThread(int page);
    ~GsThread() override;

    int pageNr() const
    {
        return m_pageNr;
    }

    void setPageNr(int page)
    {
        m_pageNr = page;
    }

    void doGS();

    bool isProcessing() const
    {
        return m_thread->isRunning();
    }

    void waitForFinish()
    {
        m_thread->wait();
    }

Q_SIGNALS:
    void gsFinished(GsThread *);

private:
    int m_pageNr;
    QThread *m_thread;
};

class ToolsThread : public QThread
{
    Q_OBJECT

public:
    explicit ToolsThread();
    ~ToolsThread() override;

    static ToolsThread *self()
    {
        return m_self;
    }

    void lookForTools();
    void lookForGS(const QString &gsPath);

    /**
     * This method of resizing PDF uses trick to convert PDF to PS and back.
     * It is done page by page, gs generates PS file with every page of PDF
     * and next gs command converts this *.ps page into PDF.
     * Because all that is done in $TEMP directory and *.ps file is quite big
     * converted *.ps is removed immediately.
     * This is also the reason why entire PDF is not turned into PS at once.
     * The huge PS may not fit into $TEMP and process fails.
     * Also this chunked way we have progress and easy way to terminate.
     */
    void resizeByGs(const QString &filePath, int pages);

    void cancel();

    QString qpdfVersion() const;
    const QString &gsVersion() const;

Q_SIGNALS:
    void lookingDone();
    void progressChanged(qreal);

protected:
    enum ToolsMode : quint8 {
        ToolsIdle = 0,
        ToolsFindAll,
        ToolsFindGS,
        ToolsResizeByGs,
    };

    void run() override;

    void findPdfTools();
    QString findGhostScript(const QString &gsfPath = QString());
    bool afterGSprocess();
    bool resizeByGsThread();

    /**
     * Returns list of 14 arguments to convert given @p pageNr
     * into *.ps file using gs command.
     * It uses @p m_pathArg to obtain pdf file path
     */
    QStringList toPsArgs(int pageNr);

    /**
     * Returns list of 8 arguments to convert *.ps file into *.pdf.
     * @p pagePath is path name but without extension
     */
    QStringList toPdfArgs(const QString &pagePath);

    void gsFinishedSlot(GsThread *gs);

Q_SIGNALS:
    void gsProcessed();

private:
    static ToolsThread *m_self;
    ToolsMode m_mode = ToolsIdle;
    QString m_gsVersion;
    QString m_pathArg;
    int m_pageCountArg = 0;
    bool m_doCancel = false;
    QVector<GsThread *> m_gsWorkers;
    int m_pagesDone = 0;
};
