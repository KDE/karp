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
    }

    function generate() {
        Qt.createComponent("org.kde.karp", "ProgressDialog").createObject(page, { pdfModel: pdfModel })
        pdfModel.generate()
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
            text: pdfModel.pdfCount > 0 ? "1. " + pdfModel.getPdfName(0) : ""
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
                        versionAction.checked: pdfModel.pdfVersion === version
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
        visible: !pdfModel.pageCount
        onClicked: openPDFs(APP.getPdfFiles())
    }

    PdfEditModel {
        id: pdfModel
        viewWidth: pdfView.width
        spacing: pdfView.columnSpacing
    }

    contentItem: QQC2.ScrollView {

        TableView {
            id: pdfView

            // clip: true // clipped by ScrollView
            visible: pdfModel.pageCount
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.largeSpacing
            model: pdfModel
            editTriggers: TableView.SingleTapped | TableView.AnyKeyPressed

            leftMargin: Kirigami.Units.largeSpacing
            rightMargin: Kirigami.Units.largeSpacing
            topMargin: Kirigami.Units.largeSpacing
            bottomMargin: Kirigami.Units.largeSpacing + bottomBar.height

            property int dragTargetPage: -1

            selectionModel: ItemSelectionModel { id: selModel }

            delegate: Rectangle {
                id: delegateRect
                required property int pageNr
                required property int fileId
                required property int origPage
                required property real pageRatio
                required property var pageImg
                required property int rotated
                // required property bool selected // TODO - multiple selection
                required property bool current
                visible: pageNr < pdfModel.pageCount
                implicitWidth: pdfModel.maxPageWidth; implicitHeight: pdfModel.maxPageWidth * pageRatio
                color: "transparent"
                border {
                    width: current ? 5 : 0
                    color: Kirigami.Theme.highlightColor
                }
                PdfPageItem {
                    id: img
                    property bool enableAnimation: false
                    z: -1
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    image: pageImg
                    scale: parent.width / (rotated === 90 || rotated === 270 ? height : width)
                    Behavior on x { enabled: img.enableAnimation; NumberAnimation {} }
                    Behavior on y { enabled: img.enableAnimation; NumberAnimation {} }
                    rotation: rotated
                }
                Rectangle {
                    visible: bottomBar.labelsVisible
                    anchors { bottom: parent.bottom; right: parent.right; margins: 2 }
                    height: Kirigami.Units.gridUnit * 2
                    width: height * 3
                    color: pdfModel.labelColor(fileId)
                    Text {
                        x: Kirigami.Units.smallSpacing
                        width: parent.width - x * 2; height: parent.height
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        color: "#fff"
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 6
                        font { pixelSize: parent.height * 0.8; bold: true }
                        text: (pageNr + 1) + " <font size=\"1\">(" + (origPage + 1) + (pdfModel.pdfCount > 1 ? "/" + (fileId + 1) : "") + ")</font>"
                    }
                }
                TableView.editDelegate: EditDelegate {}
                Loader {
                    active: pdfView.dragTargetPage === pageNr
                    z: 5
                    sourceComponent: Rectangle {
                        x: delegateRect.width * 0.9 + Kirigami.Units.smallSpacing
                        y: delegateRect.height * 0.05
                        width: delegateRect.width * 0.1; height: delegateRect.height * 0.9
                        color: Kirigami.Theme.highlightColor
                    }
                }
            } // delegate
        } // ListView
    }

    MainToolbar {
        id: bottomBar

        pdfModel: pdfModel
        visible: pdfModel.pageCount
        z: 600000
        parent: page.overlay

        anchors {
            bottom: parent.bottom
            margins: Kirigami.Units.largeSpacing
            horizontalCenter: parent.horizontalCenter
        }
    }

    function movePage(from: int, to: int) : void {
        var pageNr = pdfModel.movePage(from, to)
        if (pageNr > -1) {
            selModel.setCurrentIndex(pdfView.index(pageNr / pdfView.columns, pageNr % pdfView.columns), ItemSelectionModel.Current)
            pdfView.edit(selModel.currentIndex)
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
            const component = Qt.createComponent("org.kde.karp", "PdfFilesDialog");
            if (component.status !== Component.Ready) {
                console.error(component.errorString());
                return;
            }
            component.createObject(page, { pdfEdit: pdfModel })
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
    }
}
