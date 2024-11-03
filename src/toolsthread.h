// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QThread>

class ToolsThread : public QThread
{
    Q_OBJECT

public:
    explicit ToolsThread();
    ~ToolsThread();

    void lookForTools();
    void lookForQPDF(const QString &qpdfPath);
    void lookForGS(const QString &gsPath);

    QString qpdfVersion() const;
    QString gsVersion() const;

Q_SIGNALS:
    void lookingDone();

protected:
    enum ToolsMode : quint8 {
        ToolsIdle = 0,
        ToolsFindAll,
        ToolsFindQPDF,
        ToolsFindGS,
    };

    void run() override;

    void findPdfTools();
    QString findQpdf(const QString &qpdfPath = QString());
    QString findGhostScript(const QString &gsfPath = QString());

private:
    ToolsMode m_mode = ToolsIdle;
    QString m_qpdfVersion;
    QString m_gsVersion;
    QString m_arg1;
};
