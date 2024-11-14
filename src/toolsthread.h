// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QThread>

class PdfMetaData;

class ToolsThread : public QThread
{
    Q_OBJECT

public:
    explicit ToolsThread();
    ~ToolsThread();

    static ToolsThread *self()
    {
        return m_self;
    }

    void lookForTools();
    void lookForQPDF(const QString &outFile);
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

    void applyMetadata(const QString &pdfPath, PdfMetaData *metadata);

    QString qpdfVersion() const;
    QString gsVersion() const;

Q_SIGNALS:
    void lookingDone();
    void progressChanged(qreal);

protected:
    enum ToolsMode : quint8 {
        ToolsIdle = 0,
        ToolsFindAll,
        ToolsFindQPDF,
        ToolsFindGS,
        ToolsResizeByGs,
        ToolsMetadata,
    };

    void run() override;

    void findPdfTools();
    QString findQpdf(const QString &qpdfPath = QString());
    QString findGhostScript(const QString &gsfPath = QString());
    bool resizeByGsThread();
    void doMetadata();

private:
    static ToolsThread *m_self;
    ToolsMode m_mode = ToolsIdle;
    QString m_qpdfVersion;
    QString m_gsVersion;
    QString m_pathArg;
    int m_pageCountArg = 0;
    PdfMetaData *m_metadataArg = nullptr;
    bool m_doCancel = false;
};
