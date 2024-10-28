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

    QString qpdfVersion() const;
    QString gsVersion() const;

Q_SIGNALS:
    void lookingDone();

protected:
    enum ToolsMode : quint8 {
        ToolsIdle = 0,
        ToolsFind,
    };

    void run() override;

    void findPdfTools();
    QString findQpdf();
    QString findGhostScript();

private:
    ToolsMode m_mode = ToolsIdle;
    QString m_qpdfVersion;
    QString m_gsVersion;
};
