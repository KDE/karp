// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

#include "pdfeditmodel.h"
#include <QDebug>
#include <QPdfDocument>
#include <QProcess>

using namespace Qt::Literals::StringLiterals;

PdfEditModel::PdfEditModel(const QString &pdfFile, QObject *parent)
    : QAbstractListModel(parent)
{
    m_pdfDoc = new QPdfDocument(this);
    auto res = m_pdfDoc->load(pdfFile);
    if (res != QPdfDocument::Error::None) {
        qDebug() << "[PdfEditModel]" << "Cannot load PDF document" << pdfFile;
        return;
    }
    m_pdfFile = pdfFile;

    m_rows = m_pdfDoc->pageCount();
    m_rotated = new quint16[m_rows]{0};
    m_deleted = new bool[m_rows]{false};
    m_deletedCount = 0;
    m_pageMap.clear();
    m_wasMoved = false;
    for (int i = 0; i < m_rows; ++i) {
        m_pageMap << i;
    }
}

PdfEditModel::~PdfEditModel()
{
    if (m_rotated)
        delete[] m_rotated;
    if (m_deleted)
        delete[] m_deleted;
}

qreal PdfEditModel::maxPageWidth() const
{
    return m_maxPageWidth;
}

void PdfEditModel::setMaxPageWidth(qreal maxPW)
{
    if (maxPW == m_maxPageWidth)
        return;
    m_maxPageWidth = maxPW;
    Q_EMIT maxPageWidthChanged();
    Q_EMIT dataChanged(index(0), index(m_rows - 1), QList<int>() << RoleImage);
}

bool PdfEditModel::edited() const
{
    return true && (m_deletedCount > 0 || m_wasMoved); // TODO
}

void PdfEditModel::addRotation(int pageId, int angle)
{
    if (pageId < 0 || pageId >= m_rows)
        return;
    if (angle < 0) {
        switch (angle) {
        case -90:
            angle = 270;
            break;
        case -180:
            angle = 180;
            break;
        case -270:
            angle = 90;
            break;
        }
    }
    m_rotated[map(pageId)] = angle;
    Q_EMIT editedChanged(); /// TODO
}

void PdfEditModel::addDeletion(int pageId, bool doDel)
{
    if (pageId < 0 || pageId >= m_rows)
        return;
    m_deleted[map(pageId)] = doDel;
    if (doDel) {
        m_deletedCount++;
    } else {
        if (m_deletedCount == 0) {
            qDebug() << "[PdfEditModel]" << "No page was deleted yet!";
            return;
        } else {
            m_deletedCount--;
        }
    }
    Q_EMIT editedChanged(); /// TODO
    Q_EMIT dataChanged(index(pageId), index(pageId), QList<int>() << RoleDeleted);
}

/**
 * Moves given @p pageNr to @p toPage.
 * If next page after @p toPage is deleted, moves @p pageNr after deleted page.
 * Returns -1 if target page can be determined or target page number.
 */
int PdfEditModel::addMove(int pageNr, int toPage)
{
    if (pageNr < 0 || pageNr >= m_rows || toPage < 0 || toPage >= m_rows)
        return -1;

    m_wasMoved = true;
    int cnt = toPage + 1;
    while (cnt < m_rows) {
        if (!m_deleted[map(cnt)]) {
            toPage = cnt - 1;
            break;
        }
        cnt++;
    }
    if (cnt >= m_rows)
        return -1;
    m_pageMap.move(pageNr, toPage);
    Q_EMIT editedChanged();
    Q_EMIT dataChanged(index(0), index(m_rows - 1));
    return toPage;
}

void PdfEditModel::generate()
{
    if (m_deletedCount >= m_rows)
        return;
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProgram(u"qpdf"_s);
    // p.setArguments(QStringList() << u"--version"_s);
    // p.start();
    // p.waitForFinished();
    // qDebug().noquote() << p.readLine();
    // p.close();

    QStringList args;
    args << m_pdfFile;
    if (m_deletedCount > 0 || m_wasMoved) {
        QString delArgs;
        args << u"--pages"_s << u"."_s;
        QVector<QPair<int, int>> pageRanges;
        bool lastRangeClosed = false;
        int cnt = 0;
        while (cnt < m_rows && m_deleted[map(cnt)]) {
            cnt++;
        }
        // cnt++;
        if (cnt >= m_rows)
            return;
        int fromPage = map(cnt);
        int toPage = fromPage + 1;
        cnt++;
        for (int d = cnt; d < m_rows; ++d) {
            int nr = map(d);
            if (m_deleted[nr]) {
                if (d == m_rows - 1)
                    lastRangeClosed = false;
                continue;
            }
            if (nr == toPage) {
                toPage++;
                lastRangeClosed = false;
                continue;
            } else {
                toPage--;
                pageRanges << QPair<int, int>(fromPage, toPage);
                lastRangeClosed = d < m_rows - 1;
                fromPage = nr;
                toPage = fromPage + 1;
            }
        }
        if (!pageRanges.isEmpty() && !lastRangeClosed)
            pageRanges << QPair<int, int>(fromPage, toPage - 1);
        for (auto &r : pageRanges) {
            if (!delArgs.isEmpty())
                delArgs.append(u","_s);
            if (r.first == r.second) {
                delArgs.append(QString::number(r.first + 1));
            } else {
                delArgs.append(QString(u"%1-%2"_s).arg(r.first + 1).arg(r.second + 1));
            }
        }
        args << delArgs << u"--"_s;
    }
    // Rotation of pages - aggregate angles
    QVector<quint16> r90, r180, r270;
    for (int r = 0; r < m_rows; ++r) {
        if (m_rotated[r] == 90)
            r90 << r;
        else if (m_rotated[r] == 180)
            r180 << r;
        else if (m_rotated[r] == 270)
            r270 << r;
    }
    if (!r90.isEmpty())
        args << getPagesForRotation(90, r90);
    if (!r180.isEmpty())
        args << getPagesForRotation(180, r180);
    if (!r270.isEmpty())
        args << getPagesForRotation(270, r270);
    auto out = m_pdfFile;
    out.insert(m_pdfFile.length() - 4, u"-out"_s);
    args << out;
    qDebug().noquote() << args.join(u" "_s);
    p.setArguments(args);
    p.start();
    p.waitForFinished();
    qDebug().noquote().nospace() << p.readAll();
    p.close();
}

QPdfDocument *PdfEditModel::pdfDocument() const
{
    return m_pdfDoc;
}

int PdfEditModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_rows;
}

QVariant PdfEditModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows)
        return QVariant();
    int row = map(index.row());
    switch (role) {
    case RoleImage: {
        QSizeF pSize = m_pdfDoc->pagePointSize(row);
        qreal pageRatio = pSize.height() / pSize.width();
        return QVariant::fromValue(m_pdfDoc->render(row, QSize(m_maxPageWidth, qFloor(m_maxPageWidth * pageRatio))));
    }
    case RoleRotated:
        return m_rotated[row];
    case RoleDeleted:
        return m_deleted[row];
    case RoleOrigNr:
        return row;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{RoleImage, "pageImg"}, {RoleRotated, "rotated"}, {RoleDeleted, "deleted"}, {RoleOrigNr, "origPage"}};
}

QString PdfEditModel::getPagesForRotation(int angle, const QVector<quint16> &pageList)
{
    QString pRange;
    if (!pageList.isEmpty()) {
        pRange.append(QString(u"--rotate=+%1:"_s).arg(angle));
        pRange.append(QString::number(map(pageList[0]) + 1));
    }
    int p = 1;
    while (p < pageList.count()) {
        if (!pRange.isEmpty())
            pRange.append(u","_s);
        pRange.append(QString::number(map(pageList[p]) + 1));
        p++;
    }
    return pRange;
}

#include "moc_pdfeditmodel.cpp"
