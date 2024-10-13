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
    // TODO: improve scaling
    beginRemoveRows(QModelIndex(), 0, m_rows - 1);
    m_rows = 0;
    endRemoveRows();
    int tmpRowCount = m_pdfDoc->pageCount();
    beginInsertRows(QModelIndex(), 0, tmpRowCount - 1);
    m_rows = tmpRowCount;
    endInsertRows();
}

bool PdfEditModel::edited() const
{
    return true && m_deletedCount > 0; // TODO
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
    m_rotated[pageId] = angle;
    Q_EMIT editedChanged(); /// TODO
}

void PdfEditModel::addDeletion(int pageId, bool doDel)
{
    if (pageId < 0 || pageId >= m_rows)
        return;
    m_deleted[pageId] = doDel;
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

void PdfEditModel::generate()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProgram(u"qpdf"_s);
    // p.setArguments(QStringList() << u"--version"_s);
    // p.start();
    // p.waitForFinished();
    // qDebug().noquote() << p.readLine();
    // p.close();

    QStringList args;
    // Find page ranges according to deleted ones
    args << m_pdfFile;
    if (m_deletedCount > 0) {
        QString delArgs;
        args << u"--pages"_s << u"."_s;
        QVector<QPair<int, int>> pageRanges;
        pageRanges << QPair<int, int>(-1, -1);
        for (int d = 0; d < m_rows; ++d) {
            if (m_deleted[d]) {
                if (pageRanges.last().first != -1)
                    pageRanges << QPair<int, int>(-1, -1);
            } else {
                if (pageRanges.last().first == -1)
                    pageRanges.last().first = d;
                pageRanges.last().second = d;
            }
        }
        if (pageRanges.last().first == -1)
            pageRanges.removeLast();
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
    qDebug().noquote() << args;
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
    switch (role) {
    case RoleImage: {
        QSizeF pSize = m_pdfDoc->pagePointSize(index.row());
        qreal pageRatio = pSize.height() / pSize.width();
        return QVariant::fromValue(m_pdfDoc->render(index.row(), QSize(m_maxPageWidth, qFloor(m_maxPageWidth * pageRatio))));
    }
    case RoleRotated:
        return m_rotated[index.row()];
    case RoleDeleted:
        return m_deleted[index.row()];
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PdfEditModel::roleNames() const
{
    return {{RoleImage, "pageImg"}, {RoleRotated, "rotated"}, {RoleDeleted, "deleted"}};
}

QString PdfEditModel::getPagesForRotation(int angle, const QVector<quint16> &pageList)
{
    QString pRange;
    if (!pageList.isEmpty()) {
        pRange.append(QString(u"--rotate=+%1:"_s).arg(angle));
        pRange.append(QString::number(pageList[0] + 1));
    }
    int p = 1;
    while (p < pageList.count()) {
        if (!pRange.isEmpty())
            pRange.append(u","_s);
        pRange.append(QString::number(pageList[p] + 1));
        p++;
    }
    return pRange;
}

#include "moc_pdfeditmodel.cpp"
