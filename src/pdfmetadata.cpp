// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfmetadata.h"
#include "version-karp.h"
#include <qpdf/QPDFObjectHandle.hh>

using namespace Qt::Literals::StringLiterals;

QByteArrayList PdfMetaData::m_tags = QByteArrayList() << "Title"_ba << "Subject"_ba << "Author"_ba << "Keywords"_ba << "Creator"_ba << "Producer"_ba
                                                      << "CreationDate"_ba << "ModDate"_ba;

PdfMetaData::PdfMetaData()
    : m_creator("Karp "_L1 + "(https://apps.kde.org/karp)"_L1)
    , m_producer("Karp "_L1 + QLatin1String(KARP_VERSION_STRING) + " - KDE arranger for PDFs "_L1)
    , m_modDate(QDateTime::currentDateTime())
{
    m_creationDate = m_modDate;
}

bool PdfMetaData::modified() const
{
    return m_modified;
}

const QString &PdfMetaData::title() const
{
    return m_title;
}

void PdfMetaData::setTitle(const QString &title)
{
    m_modified = true;
    m_title = title;
}

const QString &PdfMetaData::subject() const
{
    return m_subject;
}

void PdfMetaData::setSubject(const QString &subject)
{
    m_modified = true;
    m_subject = subject;
}

const QString &PdfMetaData::author() const
{
    return m_author;
}

void PdfMetaData::setAuthor(const QString &author)
{
    m_modified = true;
    m_author = author;
}

const QString &PdfMetaData::creator() const
{
    return m_creator;
}

void PdfMetaData::setCreator(const QString &creator)
{
    m_modified = true;
    m_creator = creator;
}

const QString &PdfMetaData::producer() const
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

void PdfMetaData::setCreationDate(const QDateTime &creationDate, bool makeModified)
{
    if (makeModified)
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

const QString &PdfMetaData::keyword() const
{
    return m_keyword;
}

void PdfMetaData::setKeyword(const QString &keyword)
{
    m_modified = true;
    m_keyword = keyword;
}

QVariantList PdfMetaData::model() const
{
    QVariantList m;
    m << m_title << m_subject << m_author << m_keyword << m_producer << m_creator << m_creationDate << m_modDate;
    return m;
}

void PdfMetaData::setData(const QVariantList &mData)
{
    const int dataCount = qMin(PDF_METADATA_TAGS_COUNT, mData.size());
    if (dataCount > 0)
        m_title = mData[0].toString();
    if (dataCount > 1)
        m_subject = mData[1].toString();
    if (dataCount > 2)
        m_author = mData[2].toString();
    if (dataCount > 3)
        m_keyword = mData[3].toString();
    if (dataCount > 4)
        m_producer = mData[4].toString();
    if (dataCount > 5)
        m_creator = mData[5].toString();
    if (dataCount > 6)
        m_creationDate = mData[6].toDateTime();
    if (dataCount > 7)
        m_modDate = mData[7].toDateTime();
    m_modified = true;
}

void PdfMetaData::setAllInfoKeys(QPDFObjectHandle &qpdfHandle)
{
    if (!m_title.isEmpty()) {
        auto titleObj = QPDFObjectHandle::newString(m_title.toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::Title), titleObj);
    }
    if (!m_subject.isEmpty()) {
        auto subjectObj = QPDFObjectHandle::newString(m_subject.toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::Subject), subjectObj);
    }
    if (!m_author.isEmpty()) {
        auto authorObj = QPDFObjectHandle::newString(m_author.toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::Author), authorObj);
    }
    if (!m_keyword.isEmpty()) {
        auto keywordsObj = QPDFObjectHandle::newString(m_keyword.toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::Keywords), keywordsObj);
    }
    if (!m_producer.isEmpty()) {
        auto producerObj = QPDFObjectHandle::newString(m_producer.toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::Producer), producerObj);
    }
    if (!m_creator.isEmpty()) {
        auto creatorObj = QPDFObjectHandle::newString(m_creator.toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::Creator), creatorObj);
    }
    // TODO: deal with time zone (+00), common format string
    if (m_creationDate.isValid()) {
        auto dateObj = QPDFObjectHandle::newString(m_creationDate.toString(u"D:yyyyMMddhhmmss+00''00''"_s).toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::CreationDate), dateObj);
    }
    if (m_modDate.isValid()) {
        auto dateObj = QPDFObjectHandle::newString(m_modDate.toString(u"D:yyyyMMddhhmmss+00''00''"_s).toStdString());
        qpdfHandle.replaceKey(infoTag(QPdfDocument::MetaDataField::ModificationDate), dateObj);
    }
}

QByteArrayList PdfMetaData::tags()
{
    return m_tags;
}

QByteArray PdfMetaData::tag(QPdfDocument::MetaDataField tagId)
{
    const int id = static_cast<int>(tagId);
    if (id < 0 || id >= PDF_METADATA_TAGS_COUNT)
        return QByteArray();
    return m_tags[id];
}

std::string PdfMetaData::infoTag(QPdfDocument::MetaDataField tagId)
{
    return "/" + tag(tagId).toStdString();
}
