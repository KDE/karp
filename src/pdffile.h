// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczzuk <seelook@gmail.com>

#pragma once

#include <QString>

/**
 * @brief PdfFileInfo is container class to keep info about PDF file.
 */
class PdfFile
{
public:
    enum PdfFileFlags : quint8 {
        PdfNotAdded = 0, /**< Initial state when file was not yet loaded to the model */
        PdfLoaded,
        PdfEvenOnly,
        PdfOddOnly,
        PdfEveryNPage,
    };

    PdfFile(const QString &d, const QString &n, quint16 p, PdfFileFlags s = PdfNotAdded);

    /**
     * PDF file path (without name)
     */
    QString dir() const
    {
        return m_dir;
    }

    /**
     * PDF File name with .pdf extension
     */
    QString name() const
    {
        return m_name;
    }

    quint16 pages() const
    {
        return m_pages;
    }

private:
    QString m_dir;
    QString m_name;
    quint16 m_pages = 0;
    PdfFileFlags m_state = PdfNotAdded;
    quint16 m_n = 1;
};
