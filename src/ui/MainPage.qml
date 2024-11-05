// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.Page {
    id: page

    property alias pdfModel: pdfModel

    function clearAll() {
        pdfModel.clearAll()
    }

    function generate() {
        Qt.createComponent("org.kde.deafed", "ProgressDialog").createObject(page, { pdfModel: pdfModel })
        pdfModel.generate()
    }

    actions: [
        Kirigami.Action {
            visible: pdfModel.pageCount
            enabled: pdfModel.edited
            icon.name: "document-save"
            text: i18n("Save")
            onTriggered: page.generate()
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
            icon.name: "list-add"
            tooltip: i18n("Add PDF files")
            onTriggered: Qt.createComponent("org.kde.deafed", "PdfFilesDialog").createObject(page, { pdfEdit: pdfModel })
        },
        Kirigami.Action {
            // visible: pdfModel.pageCount
            icon.name: "settings-configure"
            text: i18n("PDF Options")
            Kirigami.Action {
                icon.name: "image-x-generic"
                text: i18n("Optimize images")
                checkable: true
                checked: pdfModel.optimizeImages
                onTriggered: pdfModel.optimizeImages = checked
            }
            Kirigami.Action {
                icon.name: "application-x-compressed-tar"
                text: i18n("Reduce size")
                checkable: true
                checked: pdfModel.reduceSize
                onTriggered: pdfModel.reduceSize = checked
            }
            Kirigami.Action {
                icon.name: "viewpdf"
                text: i18n("PDF properties")
                onTriggered: Qt.createComponent("org.kde.deafed", "PdfMetadataDialog").createObject(page, { pdfModel: pdfModel })
            }
        }
    ]

    PdfEditModel {
        id: pdfModel
        viewWidth: pdfView.width
        spacing: pdfView.columnSpacing
    }

    QQC2.Button {
        visible: !pdfModel.pageCount
        anchors.centerIn: parent
        text: i18n("Select PDF file")
        icon.name: "application-pdf"
        onClicked: pdfModel.loadPdfFile(PDFED.getPdfFile())
    }

    Rectangle {
        visible: pdfModel.pageCount
        anchors.fill: pdfView
        color: Kirigami.Theme.alternateBackgroundColor
    }

    TableView {
        id: pdfView
        visible: pdfModel.pageCount
        width: page.width - Kirigami.Units.largeSpacing * 4
        height: page.height - Kirigami.Units.largeSpacing * 3 - bottomBar.height
        clip: true
        columnSpacing: Kirigami.Units.smallSpacing
        rowSpacing: Kirigami.Units.smallSpacing
        model: pdfModel
        editTriggers: TableView.SingleTapped | TableView.AnyKeyPressed

        property int dragTargetPage: -1

        selectionModel: ItemSelectionModel { id: selModel }

        delegate: Rectangle {
            id: delegRect
            visible: pageNr < pdfModel.pageCount
            // required property bool selected // TODO - multiple selection
            required property bool current
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
            Loader {
                active: bottomBar.labelsVisible
                anchors { bottom: parent.bottom; right: parent.right; margins: 2 }
                height: Kirigami.Units.gridUnit * 2
                width: height * 3
                sourceComponent: Rectangle {
                    anchors.fill: parent
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
            }
            TableView.editDelegate: EditDelegate {}
            Loader {
                active: pdfView.dragTargetPage === pageNr
                z: 5
                sourceComponent: Rectangle {
                    x: delegRect.width * 0.9 + Kirigami.Units.smallSpacing
                    y: delegRect.height * 0.05
                    width: delegRect.width * 0.1; height: delegRect.height * 0.9
                    color: Kirigami.Theme.highlightColor
                }
            }
        } // delegate
        QQC2.ScrollBar.vertical: QQC2.ScrollBar { visible: true }
    } // ListView

    footer: MainToolbar {
        id: bottomBar
        pdfModel: pdfModel
    }

    function movePage(from, to) {
        var pageNr = pdfModel.addMove(from, to)
        if (pageNr > -1) {
            selModel.setCurrentIndex(pdfView.index(pageNr / pdfView.columns, pageNr % pdfView.columns), ItemSelectionModel.Current)
            pdfView.edit(selModel.currentIndex)
        }
    }

    Connections {
        target: pdfModel
        function onPdfCountChanged() {
            if (pdfModel.pdfCount > 1) {
                    let newAct = actionComp.createObject(nameAct)
                    newAct.text = pdfModel.pdfCount + ". " + pdfModel.getPdfName(pdfModel.pdfCount - 1)
                    newAct.icon.color = pdfModel.labelColor(pdfModel.pdfCount - 1)
                    nameAct.children.push(newAct)
            }
        }
    }

    Connections {
        target: PDFED
        function onToolIsMissing(warn) {
            Qt.createComponent("org.kde.deafed", "MissingPdfTool").createObject(page, { text: warn })
        }
    }

    Component {
        id: actionComp
        Kirigami.Action { icon.name: "snap-page" }
    }

    Component.onCompleted: {
        var initFiles = PDFED.getInitFileList()
        if (initFiles.length === 1)
            pdfModel.loadPdfFile(initFiles[0])
        else if (initFiles.length > 1)
            Qt.createComponent("qrc:/qt/qml/org/kde/deafed/ui/PdfFilesDialog.qml").createObject(page, { pdfEdit: pdfModel, initFiles: initFiles })
    }
}
