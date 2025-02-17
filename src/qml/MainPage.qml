// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.karp
import org.kde.karp.config

Kirigami.Page {
    id: page

    property alias pdfModel: pdfModel
    readonly property alias saveAction: saveAction

    function clearAll() {
        pdfModel.clearAll()
        bottomBar.showBookmarks = false
    }

    function generate() {
        Qt.createComponent("org.kde.karp", "ProgressDialog").createObject(page, { pdfModel: pdfModel })
        pdfModel.generate()
    }

    FontMetrics {
        id: nameElided
    }

    actions: [
        Kirigami.Action {
            id: saveAction
            visible: pdfModel.pageCount
            enabled: pdfModel.edited
            fromQAction: APP.action("save_pdf")
            text: KarpConf.askForOutFile ? i18nc("@action:inmenu", "Save As...") : i18nc("@action:inmenu", "Save")
        },
        Kirigami.Action {
            visible: pdfModel.pdfCount > 0
            displayComponent: QQC2.Label {
                text: i18np("file", "files", pdfModel.pdfCount) + ":"
            }
        },
        Kirigami.Action {
            id: nameAct
            visible: pdfModel.pdfCount > 0
            text: nameElided.elidedText(tooltip, Qt.ElideMiddle, page.width * 0.4, 0)
            tooltip: pdfModel.pdfCount > 0 ? "1. " + pdfModel.getPdfName(0) : ""
            icon.name: "snap-page"
            icon.color: pdfModel.labelColor(0)
        },
        Kirigami.Action {
            fromQAction: APP.action("open_pdf")
            text: ""
        },
        Kirigami.Action {
            enabled: pdfModel.pageCount
            icon.name: "settings-configure"
            text: i18n("PDF Options")
            Kirigami.Action {
                id: optimizeAct
                visible: false // TODO: it doesn't work properly with qpdf
                fromQAction: APP.action("optimize")
                checked: pdfModel.optimizeImages
            }
            Kirigami.Action {
                id: redSizeAct
                enabled: APP.gsVersion !== ""
                fromQAction: APP.action("reduce_size")
                checked: pdfModel.reduceSize
            }
            Kirigami.Action {
                id: pdfVerAct

                text: i18nc("@action:inmenu", "PDF Version")
                icon.name: 'application-pdf-symbolic'

                component VersionAction : Kirigami.Action {
                    id: versionAction

                    required property real version

                    text: version === 0 ? i18nc("like default PDF version", "Default") : i18nc("PDF version", "Version %1", version)
                    onTriggered: pdfModel.pdfVersion = version;
                    checkable: true

                    readonly property Binding binding: Binding {
                        versionAction.checked: pdfModel.pdfVersion === versionAction.version
                    }
                }

                VersionAction {
                    version: 0.0
                }

                VersionAction {
                    version: 1.4
                }

                VersionAction {
                    version: 1.5
                }

                VersionAction {
                    version: 1.6
                }

                VersionAction {
                    version: 1.7
                }
            }

            Kirigami.Action {
                fromQAction: APP.action("set_password")
                checked: pdfModel.passKey !== ""
            }
            Kirigami.Action {
                fromQAction: APP.action("pdf_meta")
            }
        }
    ]

    topPadding: 0
    rightPadding: 0
    leftPadding: 0
    bottomPadding: 0

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

        TOCView {
            id: outlines

            visible: bottomBar.showBookmarks
            QQC2.SplitView.fillHeight: true
            QQC2.SplitView.preferredWidth: Kirigami.Units.gridUnit * 15
            QQC2.SplitView.minimumWidth: Kirigami.Units.gridUnit * 5
            QQC2.SplitView.maximumWidth: page.width / 3

            onBookmarkSelected: (pageNr) => {
                pdfView.positionViewAtIndex(pageNr, GridView.Center)
            }
            onTocAboutToClear: {
                bottomBar.showBookmarks = true // override bindings to keep pane visible
            }
        }

        PdfView {
            id: pdfView

            visible: pdfModel.pageCount
            QQC2.SplitView.fillWidth: true
            QQC2.SplitView.minimumWidth: splitView.width / 2
            QQC2.SplitView.fillHeight: true

            cellWidth: pdfModel.maxPageWidth + pdfModel.spacing
            cellHeight: pdfModel.maxPageHeight + pdfModel.spacing

            bottomBar: bottomBar
            model: pdfModel

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
        showBookmarks: outlines.rows > 0

        anchors {
            bottom: parent.bottom
            margins: Kirigami.Units.largeSpacing
        }
    }

    Connections {
        target: pdfModel
        function onPdfCountChanged() : void {
            if (pdfModel.pdfCount > 1) {
                    let newAct = actionComp.createObject(nameAct)
                    newAct.text = pdfModel.pdfCount + ". " + pdfModel.getPdfName(pdfModel.pdfCount - 1)
                    newAct.icon.color = pdfModel.labelColor(pdfModel.pdfCount - 1)
                    nameAct.children.push(newAct)
            }
        }
        function onPasswordRequired(fName: string , fId: int) : void {
            let passDlg = Qt.createComponent("org.kde.karp", "PdfPassDialog").createObject(page, { fileName: fName, fileId: fId })
            passDlg.accepted.connect(function(){ pdfModel.setPdfPassword(passDlg.fileId, passDlg.passKey) })
            passDlg.rejected.connect(function(){ pdfModel.setPdfPassword(passDlg.fileId, "") })
        }
    }

    Connections {
        target: APP
        function onToolIsMissing(warn: string) : void {
            Qt.createComponent("org.kde.karp", "MissingPdfTool").createObject(page, { text: warn })
        }
        // Actions
        function onWantSavePdf() : void {
            page.generate()
        }
        function onWantOpenPdf() : void {
            const fileDlgComp = Qt.createComponent("org.kde.karp", "PdfFilesDialog");
            if (fileDlgComp.status !== Component.Ready) {
                console.error(fileDlgComp.errorString());
                return;
            }
            // pdfView.selectionModel.clearCurrentIndex()
            // Workaround to avoid stilling drag by pdfView during PDF reorder
            contentItem.enabled = false
            let fileDlgObj = fileDlgComp.createObject(page, { pdfEdit: pdfModel })
            fileDlgObj.closed.connect(() => contentItem.enabled = true)
        }
        function onWantClearAll() : void {
            page.clearAll()
        }
        function onWantOptimize() : void {
            pdfModel.optimizeImages = optimizeAct.checked
        }
        function onWantReduceSize() : void {
            pdfModel.reduceSize = redSizeAct.checked
        }
        function onWantSetPassword() : void {
            let passDlg = Qt.createComponent("org.kde.karp", "PdfPassDialog").createObject(page,
                { fileName: "", fileId: 0, title: i18n("Set password"), passLabel: i18n("Protect PDF file with password."), passKey: pdfModel.passKey })
            passDlg.accepted.connect(function(){ pdfModel.passKey = passDlg.passKey })
            passDlg.rejected.connect(function(){ pdfModel.passKey = "" })
        }
        function onWantPdfMeta() {
            Qt.createComponent("org.kde.karp", "PdfMetadataDialog").createObject(page, { pdfModel: pdfModel })
        }
    }

    Component {
        id: actionComp
        Kirigami.Action { icon.name: "snap-page" }
    }

    /**
     * Common function to handle @p pdfFiles argument.
     * When it is just single file - load it immediately or open PdfFilesDialog
     */
    function openPDFs(pdfFiles: var) {
        if (pdfFiles.length === 1)
            pdfModel.loadPdfFile(pdfFiles[0])
        else if (pdfFiles.length > 1)
            Qt.createComponent("org.kde.karp", "PdfFilesDialog").createObject(page, { pdfEdit: pdfModel, initFiles: pdfFiles })
        outlines.model = pdfModel.getBookmarkModel()
    }
}
