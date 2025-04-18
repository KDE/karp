// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

#pragma once

#include "pdffile.h"
#include <QAbstractListModel>
#include <QObject>

class PdfEditModel;
class PdfFile;

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
        RoleLocked,
        RoleAllPages,
    };

    /**
     * Addends @p pdfFile to the model.
     * Returns number of pages the file has or 0.
     */
    int appendFile(const QString &pdfFile);
    int appendPdfFile(PdfFile *pdf);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rows() const
    {
        return m_rows;
    }

    PdfFile *getPdfFile(int id)
    {
        return m_pdfFiles[id];
    }

    PdfFile *lastPdf()
    {
        return m_pdfFiles.last();
    }

    Q_INVOKABLE void move(int fromId, int toId);
    Q_INVOKABLE void remove(int fileId);

    int setPdfPassword(int fileId, const QString &pass);

private:
    int m_rows = 0;

    QVector<PdfFile *> m_pdfFiles;
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
    Q_PROPERTY(QVariant initFiles READ initFiles WRITE setInitFiles NOTIFY initFilesChanged)

public:
    explicit PdfsOrganizer(QObject *parent = nullptr);

    QVariant editModel();
    void setEditModel(const QVariant &edMod);

    QVariant fileModel() const;

    int totalPages() const;

    QVariant initFiles() const;
    void setInitFiles(const QVariant &filesVar);

    /**
     * Opens File Dialog to select one or more PDF files.
     * If something was selected - adds files to @p PdfListModel
     * and returns TRUE.
     */
    Q_INVOKABLE bool addMorePDFs();

    Q_INVOKABLE void applyNewFiles();

    Q_INVOKABLE void setPdfPassword(int fileId, const QString &pass);

Q_SIGNALS:
    void editModelChanged();
    void fileModelChanged();
    void totalPagesChanged();
    void initFilesChanged(); // dummy

    /**
     * Emitted when PDF file requires password to open.
     * 1st argument is file name and 2nd is file id in the list
     */
    void passwordRequired(const QString &, int);

protected:
    void addPdfList(const QStringList &pdfList);

private:
    PdfEditModel *m_editModel = nullptr;
    PdfListModel *const m_fileModel;
    int m_totalPages = 0;
    QList<int> m_passFiles;
};
