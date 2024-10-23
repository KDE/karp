// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include "pdffile.h"
#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class PdfEditModel;

/**
 * @brief PdfListModel exposes selected PDF files to ListView
 */
class PdfListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PdfListModel(QObject *parent = nullptr);

    enum PdfListRoles {
        RoleDirName = Qt::UserRole,
        RoleFileName,
        RolePageCount,
    };

    /**
     * Addends @p pdfFile to the model.
     * Returns number of pages the file has or 0.
     */
    int appendFile(const QString &pdfFile);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVector<PdfFile> &pdfInfos();

private:
    int m_rows = 0;

    QVector<PdfFile> m_pdfInfos;
};

/**
 * @brief PdfsOrganizer class organizes PDF files list.
 */
class PdfsOrganizer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant editModel READ editModel WRITE setEditModel NOTIFY editModelChanged)
    Q_PROPERTY(QVariant fileModel READ fileModel NOTIFY fileModelChanged)
    Q_PROPERTY(int totalPages READ totalPages NOTIFY totalPagesChanged)

public:
    explicit PdfsOrganizer(QObject *parent = nullptr);

    QVariant editModel();
    void setEditModel(const QVariant &edMod);

    QVariant fileModel() const;

    int totalPages() const;

    /**
     * Opens File Dialog to select one or more PDF files.
     * If something was selected - adds files to @p PdfListModel
     * and returns TRUE.
     */
    Q_INVOKABLE bool addMorePDFs();

Q_SIGNALS:
    void editModelChanged();
    void fileModelChanged();
    void totalPagesChanged();

private:
    PdfEditModel *m_editModel = nullptr;
    PdfListModel *m_fileModel;
    int m_totalPages = 0;
};
