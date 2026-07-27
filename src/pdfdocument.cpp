// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#include "pdfdocument.h"
#include "pdfpage.h"
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>

PdfDocument::PdfDocument(const QString &pdfFileName, quint16 refFileId, PdfFileFlags s)
    : m_refFileId(refFileId)
    , m_state(s)
{
    setFile(pdfFileName);
}

PdfDocument::~PdfDocument()
{
}

void PdfDocument::update()
{
    if (!m_document) {
        return;
    }

    m_locked = m_document->isLocked();

    if (m_locked) {
        return;
    }

    m_range.setTo(m_document->numPages());
    m_document->setRenderHint(Poppler::Document::Antialiasing);
    m_document->setRenderHint(Poppler::Document::TextAntialiasing);
    m_document->setRenderHint(Poppler::Document::TextHinting);
    m_document->setRenderHint(Poppler::Document::TextSlightHinting);
}

void PdfDocument::setFile(const QString &fileName)
{
    QFileInfo pdfInfo(fileName);
    m_dir = pdfInfo.canonicalPath() + QDir::separator();
    m_name = pdfInfo.fileName();
    m_document = Poppler::Document::load(fileName);

    update();
}

void PdfDocument::setPassword(const QByteArray password)
{
    if (m_document) {
        m_document->unlock(QByteArray(), password);
        update();
    }
}

void PdfDocument::requestPage(PdfPage *pdfPage, const QSize &requestedSize, quint16 pageId)
{
    if (!m_document || m_locked) {
        return;
    }
    std::unique_ptr<Poppler::Page> page = m_document->page(pdfPage->origPage());
    if (!page) {
        return;
    }

    qreal dpr = QGuiApplication::primaryScreen()->devicePixelRatio();
    auto pageSize = page->pageSizeF();

    auto xRes = (requestedSize.width() * dpr) / pageSize.width() * 72.0;
    auto yRes = (requestedSize.height() * dpr) / pageSize.height() * 72.0;

    QImage image = page->renderToImage(xRes, yRes);
    image.setDevicePixelRatio(dpr);

    pdfPage->setImage(image);
    Q_EMIT pageRendered(pageId, pdfPage);
}

#include "moc_pdfdocument.cpp"
