// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QDateTime>
#include <QPdfDocument>

#define PDF_METADATA_TAGS_COUNT (8)

class QPDFObjectHandle;

/**
 * @brief PdfMetaData class handles available info of PDF file.
 */
class PdfMetaData
{
public:
    PdfMetaData();

    bool modified() const;

    const QString &title() const;
    void setTitle(const QString &title);

    const QString &subject() const;
    void setSubject(const QString &subject);

    const QString &author() const;
    void setAuthor(const QString &author);

    const QString &creator() const;
    void setCreator(const QString &creator);

    const QString &producer() const;
    void setProducer(const QString &producer);

    const QString &keyword() const;
    void setKeyword(const QString &keyword);

    QDateTime creationDate() const;
    void setCreationDate(const QDateTime &creationDate);

    QDateTime modDate() const;
    void setModDate(const QDateTime &modDate);

    QVariantList model() const;
    void setData(const QVariantList &mData);

    void setAllInfoKeys(QPDFObjectHandle &qpdfHandle);

    /**
     * List of tags used in PDF metadata ordered according to
     * @p QPdfDocument::MetaDataField:
     * Title, Subject, Author, Keywords, Creator, Producer, CreationDate, ModDate
     */
    static QByteArrayList tags();

    static QByteArray tag(QPdfDocument::MetaDataField tagId);

    std::string infoTag(QPdfDocument::MetaDataField tagId);

private:
    static QByteArrayList m_tags;

    QString m_title;
    QString m_subject;
    QString m_author;
    QString m_creator;
    QString m_producer;
    QString m_keyword;
    QDateTime m_creationDate;
    QDateTime m_modDate;
    bool m_modified = false;
};

// Implementation of getters and setters
