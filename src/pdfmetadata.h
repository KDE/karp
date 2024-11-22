// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QPdfDocument>

#define PDF_METADATA_TAGS_COUNT (8)

/**
 * @brief PdfMetaData class handles available info of PDF file.
 */
class PdfMetaData
{
public:
    PdfMetaData();

    bool modified() const;

    QString title() const;
    void setTitle(const QString &title);

    QString subject() const;
    void setSubject(const QString &subject);

    QString author() const;
    void setAuthor(const QString &author);

    QString creator() const;
    void setCreator(const QString &creator);

    QString producer() const;
    void setProducer(const QString &producer);

    QString keyword() const;
    void setKeyword(const QString &keyword);

    QDateTime creationDate() const;
    void setCreationDate(const QDateTime &creationDate);

    QDateTime modDate() const;
    void setModDate(const QDateTime &modDate);

    QStringList model() const;
    void setData(const QStringList &mData);

    /**
     * List of tags used in PDF metadata ordered according to
     * @p QPdfDocument::MetaDataField:
     * Title, Subject, Author, Keyword, Creator, Producer, CreationDate, ModDate
     */
    static QByteArrayList tags();

    /**
     * Returns JSON object with tags of all existing metadata
     */
    QJsonObject toJSON();

    static QByteArray tag(QPdfDocument::MetaDataField tagId);

    /**
     * Returns Metadata JSON tag, i.e. '/Author'.
     * It is @p tag() with prepend '/'
     * Use @p QPdfDocument::MetaDataField enumerator
     */
    static QLatin1String jsonTag(QPdfDocument::MetaDataField tagId);

    static QString jsonString(const QString &s);

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
