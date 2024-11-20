// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfmetadata.h"
#include "version-karp.h"

using namespace Qt::Literals::StringLiterals;

QByteArrayList PdfMetaData::m_tags = QByteArrayList() << "Title"_ba << "Subject"_ba << "Author"_ba << "Keywords"_ba << "Creator"_ba << "Producer"_ba
                                                      << "CreationDate"_ba << "ModDate"_ba;

PdfMetaData::PdfMetaData()
{
    m_creator = "Karp "_L1 + "(https://karp.kde.org)"_L1;
    m_producer = "Karp "_L1 + QLatin1String(KARP_VERSION_STRING) + " - KDE arranger for PDFs "_L1;
    m_modDate = QDateTime::currentDateTime();
    m_creationDate = m_modDate;
}

bool PdfMetaData::modyfied() const
{
    return m_modified;
}

QString PdfMetaData::title() const
{
    return m_title;
}

void PdfMetaData::setTitle(const QString &title)
{
    m_modified = true;
    m_title = title;
}

QString PdfMetaData::subject() const
{
    return m_subject;
}

void PdfMetaData::setSubject(const QString &subject)
{
    m_modified = true;
    m_subject = subject;
}

QString PdfMetaData::author() const
{
    return m_author;
}

void PdfMetaData::setAuthor(const QString &author)
{
    m_modified = true;
    m_author = author;
}

QString PdfMetaData::creator() const
{
    return m_creator;
}

void PdfMetaData::setCreator(const QString &creator)
{
    m_modified = true;
    m_creator = creator;
}

QString PdfMetaData::producer() const
{
    return m_producer;
}

void PdfMetaData::setProducer(const QString &producer)
{
    m_modified = true;
    m_producer = producer;
}

QDateTime PdfMetaData::creationDate() const
{
    return m_creationDate;
}

void PdfMetaData::setCreationDate(const QDateTime &creationDate)
{
    m_modified = true;
    m_creationDate = creationDate;
}

QDateTime PdfMetaData::modDate() const
{
    return m_modDate;
}

void PdfMetaData::setModDate(const QDateTime &modDate)
{
    m_modified = true;
    m_modDate = modDate;
}

QString PdfMetaData::keyword() const
{
    return m_keyword;
}

void PdfMetaData::setKeyword(const QString &keyword)
{
    m_modified = true;
    m_keyword = keyword;
}

/**
 * kli18n("Title"), kli18n("Subject"), kli18n("Author"), kli18n("Keyword"), kli18n("Producer"), kli18n("Creator"),
 * kli18n("Creation Date"), kli18n("Modification Date")
 */
QStringList PdfMetaData::model() const
{
    QStringList m;
    m << m_title << m_subject << m_author << m_keyword << m_producer << m_creator << m_creationDate.toString(u"yyyy.MM.dd hh:mm:ss"_s)
      << m_modDate.toString(u"yyyy.MM.dd hh:mm:ss"_s);
    return m;
}

void PdfMetaData::setData(const QStringList &mData)
{
    int dataCount = qMin(PDF_METADATA_TAGS_COUNT, mData.size());
    if (dataCount > 0)
        m_title = mData[0];
    if (dataCount > 1)
        m_subject = mData[1];
    if (dataCount > 2)
        m_author = mData[2];
    if (dataCount > 3)
        m_keyword = mData[3];
    if (dataCount > 4)
        m_producer = mData[4];
    if (dataCount > 5)
        m_creator = mData[5];
    // TODO what about dates?
    // if (dataCount > 6)
    //     m_creationDate = mData[6];
    // if (dataCount > 7)
    //     m_modDate = mData[7];
    m_modified = true;
}

QByteArrayList PdfMetaData::tags()
{
    return m_tags;
}

QJsonObject PdfMetaData::toJSON()
{
    QJsonObject tagsJSON;
    if (!m_title.isEmpty())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::Title), QJsonValue::fromVariant(jsonString(m_title)));
    if (!m_subject.isEmpty())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::Subject), QJsonValue::fromVariant(jsonString(m_subject)));
    if (!m_author.isEmpty())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::Author), QJsonValue::fromVariant(jsonString(m_author)));
    if (!m_keyword.isEmpty())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::Keywords), QJsonValue::fromVariant(jsonString(m_keyword)));
    if (!m_producer.isEmpty())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::Producer), QJsonValue::fromVariant(jsonString(m_producer)));
    if (!m_creator.isEmpty())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::Creator), QJsonValue::fromVariant(jsonString(m_creator)));
    // TODO: deal with time zone (+00)
    if (m_creationDate.isValid())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::CreationDate), QJsonValue::fromVariant(m_creationDate.toString(u"u:D:yyyyMMddhhmmss+00''00''"_s)));
    if (m_modDate.isValid())
        tagsJSON.insert(jsonTag(QPdfDocument::MetaDataField::ModificationDate), QJsonValue::fromVariant(m_modDate.toString(u"u:D:yyyyMMddhhmmss+00''00''"_s)));
    QJsonObject valueJSON;
    valueJSON.insert(u"value"_s, tagsJSON);
    return valueJSON;
}

QByteArray PdfMetaData::tag(QPdfDocument::MetaDataField tagId)
{
    int id = static_cast<int>(tagId);
    if (id < 0 || id >= PDF_METADATA_TAGS_COUNT)
        return QByteArray();
    return m_tags[id];
}

QLatin1String PdfMetaData::jsonTag(QPdfDocument::MetaDataField tagId)
{
    return QLatin1String("/" + tag(tagId));
}

QString PdfMetaData::jsonString(const QString &s)
{
    return QLatin1String("u:") + s;
}
