// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.Page {
    id: page

    title: i18n("Arrange") + ": " + PDFED.name

    property alias pdfModel: pdfModel

    actions: [
        Kirigami.Action {
            id: labelsAction
            visible: pdfModel.pageCount
            icon.name: "label"
            text: i18n("Labels")
            checkable: true
            checked: true
        },
        Kirigami.Action {
            visible: pdfModel.pageCount
            icon.name: "zoom-out"
            text: i18n("Zoom Out")
            // shortcutsName: "ZoomOut"
            onTriggered: pdfModel.zoomOut()
            enabled: pdfModel.maxPageWidth > Kirigami.Units.gridUnit * 7
        },
        Kirigami.Action {
            visible: pdfModel.pageCount
            icon.name: "zoom-in"
            text: i18n("Zoom In")
            // shortcutsName: "ZoomIn"
            onTriggered: pdfModel.zoomIn()
            enabled: pdfView.columns > 1
        },
        Kirigami.Action {
            visible: pdfModel.pageCount
            enabled: pdfModel.edited
            icon.name: "application-pdf"
            text: i18n("Generate")
            onTriggered: pdfModel.generate()
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
                onTriggered: Qt.createComponent("qrc:/qt/qml/org/kde/deafed/ui/PdfMetadataDialog.qml")
                                                .createObject(page, { metaDataModel: pdfModel.metaDataModel() })
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
        height: page.height - Kirigami.Units.largeSpacing * 2
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
            implicitWidth: img.width; implicitHeight: img.height
            color: "transparent"
            border {
                width: current ? 5 : 0
                color: Kirigami.Theme.highlightColor
            }
            PdfPageItem {
                id: img
                z: -1
                image: pageImg
                Behavior on x { NumberAnimation {} }
                Behavior on y { NumberAnimation {} }
                opacity: x !== 0 || y !== 0 ? 0.3 : 1
                rotation: rotated
                onRotationChanged: pdfModel.addRotation(pageNr, rotation)
            }
            Loader {
                active: labelsAction.checked
                anchors { bottom: parent.bottom; right: parent.right; margins: 2 }
                height: Kirigami.Units.gridUnit * 2
                width: height * 3
                sourceComponent: Rectangle {
                    anchors.fill: parent
                    // anchors { bottom: parent.bottom; right: parent.right; margins: 2 }
                    // height: Kirigami.Units.gridUnit * 2
                    // width: height * 3
                    color: "#80000000"
                    Text {
                        width: parent.width * 0.9; height: parent.height
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        color: "#fff"
                        fontSizeMode: Text.HorizontalFit
                        minimumPixelSize: 6
                        font { pixelSize: parent.height * 0.8; bold: true }
                        text: (pageNr + 1) + " <font size=\"1\">(" + (origPage + 1) + ")</font>"
                    }
                }
            }
            TableView.editDelegate: EditDelegate {}
            Loader {
                active: deleted
                anchors.fill: parent
                sourceComponent: DeletedDelegate {
                    buttonVisible: current
                    onWantRevert: pdfModel.addDeletion(pageNr, false)
                }
            }
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

    // footer: Rectangle {
    //     id: bottomRect
    //     width: page.width; height: Kirigami.Units.gridUnit * 3
    //     color: Kirigami.Theme.alternateBackgroundColor
    //     Text {
    //         anchors { fill: parent; margins: Kirigami.Units.smallSpacing }
    //         text: pdfModel.command
    //         wrapMode: Text.WordWrap
    //         color: Kirigami.Theme.textColor
    //     }
    // }

    function movePage(from, to) {
        var pageNr = pdfModel.addMove(from, to)
        if (pageNr > -1) {
            selModel.setCurrentIndex(pdfView.index(pageNr / pdfView.columns, pageNr % pdfView.columns), ItemSelectionModel.Current)
            pdfView.edit(selModel.currentIndex)
        }
    }

    Component.onCompleted: {
        if (PDFED.path !== "")
            pdfModel.loadPdfFile(PDFED.path)
    }
}
