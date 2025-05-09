// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QObject>
#include <QThread>
#include <qpdf/QPDFJob.hh>

class PdfEditModel;
class PdfMetaData;

/**
 * @brief QPDFwrapper handler libqpdf in separate thread
 */
class QpdfProxy : public QObject
{
    Q_OBJECT

public:
    explicit QpdfProxy(PdfEditModel *pdfModel, QObject *parent = nullptr);
    ~QpdfProxy() override;

    /**
     * Invokes @p QPDFJob with transformations from @p PdfEditModel
     */
    void doJob();

    static void forcePdfVersion(QPDFJob::Config *jobConf, qreal ver);
    static void setPdfPassword(QPDFJob::Config *jobConf, const std::string &pass);
    static void addMetaToJob(QPDF &qpdf, PdfMetaData *metaData);

Q_SIGNALS:
    void finished();

protected:
    void threadSlot();

    /**
     * Converts list of pages @p QVector<quint16> to page range
     * in format respected by qpdf: 1-5 or 1,2,4-7 and appends it to @p QPDFJob.
     *
     * NOTICE: first number in the list represents file reference ID in PDF lists!
     * So pages of this file starts from @p range[1]
     */
    void appendRangeToJob(const QVector<quint16> &range, QPDFJob::PagesConfig *qpdfPages, bool isFirst = false);

    std::string getPagesForRotation(int angle, const QVector<quint16> &pageList);

private:
    PdfEditModel *const m_pdfModel;
    QThread *const m_thread;
};
