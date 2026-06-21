// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.controls as Kirigami
import org.kde.kirigami.actioncollection as Kirigami

import org.kde.karp
import org.kde.karp.config

Kirigami.Page {
    id: page

    property alias pdfModel: pdfModel
    property bool showBookmarks: false
    property bool showLabels: false
    property bool multiSelect: APP.ctrlPressed

    topPadding: 0
    rightPadding: 0
    leftPadding: 0
    bottomPadding: 0

    FontMetrics {
        id: nameElided
    }
    titleDelegate: Kirigami.ActionToolBar {
        id: leftActionBar
        Layout.fillWidth: true
        Layout.fillHeight: true

        alignment: Qt.AlignLeft
        actions: [
            Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "open_pdf"
                onTriggered: page.openOrganizerDialog()
            },
            Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "clear_all"
                enabled: pdfModel.pageCount
                onTriggered: page.clearAll()
            }
        ]
    }

    actions: [
        Kirigami.Action {
            visible: pdfModel.pdfCount > 0
            displayComponent: QQC2.Label {
                text: i18np("file", "files", pdfModel.pdfCount) + ":"
            }
        },
        Kirigami.Action {
            id: fileActions
            visible: pdfModel.pdfCount > 0
            text: nameElided.elidedText(tooltip, Qt.ElideMiddle, page.width * 0.4, 0)
            tooltip: pdfModel.pdfCount > 0 ? "1. " + pdfModel.getPdfName(0) : ""
            icon.name: "snap-page"
            icon.color: pdfModel.labelColor(0)
        },
        Kirigami.Action {
            Kirigami.ActionCollection.collection: "org.kde.karp.actions"
            Kirigami.ActionCollection.action: "export"
            visible: pdfModel.pageCount
            enabled: pdfModel.edited
            //TODO: what to do with KarpConf.askForOutFile
            onTriggered: {
                Qt.createComponent("org.kde.karp", "ExportDialog").createObject(page, {
                    pdfModel: pdfModel
                }).open();
            }
        },
        Kirigami.Action {
            Kirigami.ActionCollection.collection: "org.kde.globalactions"
            Kirigami.ActionCollection.action: "Preferences"
            onTriggered: page.openSettings()
        }
    ]

    InitialInfo {
        z: 600000
        parent: page.overlay
        visible: !pdfModel.pageCount
        onClicked: page.openPDFs(APP.getPdfFiles())
    }

    PdfEditModel {
        id: pdfModel
        viewWidth: pdfView.width
        spacing: Kirigami.Units.largeSpacing
    }

    contentItem: QQC2.SplitView {
        id: splitView

        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Item {
            implicitWidth: Kirigami.Units.smallSpacing
            Kirigami.Separator {
                // HACK: only way to add space between bookmark list and splitter handle
                anchors.right: parent.right
                height: parent.height
            }
        }

        BookmarksView {
            id: bookmarksView

            visible: page.showBookmarks

            QQC2.SplitView.fillHeight: true
            QQC2.SplitView.preferredWidth: Kirigami.Units.gridUnit * 15
            QQC2.SplitView.minimumWidth: Kirigami.Units.gridUnit * 5
            QQC2.SplitView.maximumWidth: page.width / 3

            onBookmarkSelected: pageNr => {
                pdfView.positionViewAtIndex(pageNr, GridView.Center);
            }
        }

        PdfView {
            id: pdfView
            visible: pdfModel.pageCount
            
            QQC2.SplitView.fillWidth: true
            QQC2.SplitView.minimumWidth: splitView.width / 2
            QQC2.SplitView.fillHeight: true

            model: pdfModel
            showLabels: page.showLabels
            multiSelect: page.multiSelect
            
            cellWidth: pdfModel.maxPageWidth + pdfModel.spacing
            cellHeight: pdfModel.maxPageHeight + pdfModel.spacing

            bottomMargin: Kirigami.Units.largeSpacing + bookmarksView.height

            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        }
    }

    MainToolbar {
        id: bottomBar

        pdfModel: pdfModel
        visible: pdfModel.pageCount
        x: pdfView.x + (pdfView.width - width) / 2
        z: 600000
        parent: page.overlay

        showBookmarks: page.showBookmarks
        showLabels: page.showLabels
        multiSelect: page.multiSelect

        onBooksmarksToggled: checked => page.showBookmarks = checked
        onLabelsToggled: checked => page.showLabels = checked

        anchors {
            bottom: parent.bottom
            margins: Kirigami.Units.largeSpacing
        }
    }

    Connections {
        target: pdfModel
        function onPdfCountChanged(): void {
            if (pdfModel.pdfCount > 1) {
                let action = actionComp.createObject(fileActions);
                action.text = pdfModel.pdfCount + ". " + pdfModel.getPdfName(pdfModel.pdfCount - 1);
                action.icon.color = pdfModel.labelColor(pdfModel.pdfCount - 1);
                fileActions.children.push(action);
            }
        }

        function onPasswordRequired(fName: string, fId: int): void {
            let passDlg = Qt.createComponent("org.kde.karp", "PdfPassDialog").createObject(page, {
                fileName: fName,
                fileId: fId
            });
            passDlg.accepted.connect(function () {
                pdfModel.setPdfPassword(passDlg.fileId, passDlg.passKey);
            });
            passDlg.rejected.connect(function () {
                pdfModel.setPdfPassword(passDlg.fileId, "");
            });
        }
    }

    Connections {
        target: APP
        function onToolIsMissing(warn: string): void {
            Qt.createComponent("org.kde.karp", "MissingPdfTool").createObject(page, {
                text: warn
            });
        }
    }

    Component {
        id: actionComp
        Kirigami.Action {
            icon.name: "snap-page"
        }
    }

    /**
     * Common function to handle @p pdfFiles argument.
     * When it is just single file - load it immediately or open OrganizerDialog
     */
    function openPDFs(pdfFiles: var) {
        if (pdfFiles.length === 1)
            pdfModel.loadPdfFile(pdfFiles[0]);
        else if (pdfFiles.length > 1)
            Qt.createComponent("org.kde.karp", "OrganizerDialog").createObject(page, {
                pdfEdit: pdfModel,
                initFiles: pdfFiles
            });
        bookmarksView.model = pdfModel.getBookmarkModel();
    }

    function openOrganizerDialog(): void {
        const fileDlgComp = Qt.createComponent("org.kde.karp", "OrganizerDialog");
        if (fileDlgComp.status !== Component.Ready) {
            console.error(fileDlgComp.errorString());
            return;
        }
        // pdfView.selectionModel.clearCurrentIndex()
        // Workaround to avoid stilling drag by pdfView during PDF reorder
        contentItem.enabled = false;
        let fileDlgObj = fileDlgComp.createObject(page, {
            pdfEdit: pdfModel
        });
        fileDlgObj.closed.connect(() => contentItem.enabled = true);
    }
    function clearAll() {
        pdfModel.clearAll();
    }
    function openSettings(): void {
        const settings = Qt.createComponent("org.kde.karp", "SettingsPage").createObject(page);
        settings.open();
    }
}
