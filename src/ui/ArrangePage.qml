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
                icon.name: "viewpdf"
                text: i18n("PDF properties")
                onTriggered: Qt.createComponent("qrc:/qt/qml/org/kde/deafed/ui/PdfMetadataDialog.qml").createObject(page)
            }
        }
    ]

    PdfEditModel {
        id: pdfModel
        maxPageWidth: (pdfView.width - (9 * pdfView.columnSpacing)) / 3
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

        delegate: Rectangle {
            id: delegRect
            visible: pageNr < pdfModel.pageCount
            required property bool selected
            required property bool current
            x: Kirigami.Units.smallSpacing
            implicitWidth: img.width; implicitHeight: img.height
            color: "transparent"
            border {
                width: current ? 3 : 0
                color: Kirigami.Theme.highlightColor
            }
            PdfPageItem {
                id: img
                x: 2; y: 2; z: -1
                image: pageImg
                rotation: rotated
                onRotationChanged: pdfModel.addRotation(pageNr, rotation)
            }
            Rectangle {
                anchors { bottom: parent.bottom; right: parent.right; margins: 2 }
                height: Math.max(parent.height, parent.width) * 0.05
                width: height * 3
                color: "#80000000"
                Text {
                    width: parent.width * 0.9; height: parent.height
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    color: "#fff"
                    font { pixelSize: parent.height * 0.8; bold: true }
                    text: (pageNr + 1) + " <font size=\"1\">(" + (origPage + 1) + ")</font>"
                }
            }
            MouseArea {
                id: ma
                anchors.fill: parent
                onClicked: current = true
            }
            Item {
                anchors.fill: parent
                visible: current && !deleted
                QQC2.Button {
                    anchors { top: parent.top; left: parent.left }
                    icon.name: "edit-delete"
                    icon.color: "red"
                    onClicked: {
                        img.rotation = 0
                        pdfModel.addDeletion(pageNr, true)
                    }
                }
                QQC2.Button {
                    anchors { verticalCenter: parent.verticalCenter; left: parent.left }
                    icon.name: "object-rotate-left"
                    onClicked: img.rotation = img.rotation > -270 ? img.rotation - 90 : 0
                }
                QQC2.Button {
                    anchors { verticalCenter: parent.verticalCenter; right: parent.right }
                    icon.name: "object-rotate-right"
                    onClicked: img.rotation = img.rotation < 270 ? img.rotation + 90 : 0
                }
                QQC2.Button {
                    visible: pageNr > 0
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
                    icon.name: "go-up"
                    onClicked: movePage(pageNr, pageNr - 1)
                }
                QQC2.Button {
                    visible: pageNr !== pdfModel.pageCount - 1
                    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
                    icon.name: "go-down"
                    onClicked: movePage(pageNr, pageNr + 1)
                }
            }
            Loader {
                active: deleted
                z: 5 // atop of mouse area
                anchors.fill: parent
                sourceComponent: DeletedDelegate {
                    buttonVisible: delegRect.checked
                    onWantRevert: pdfModel.addDeletion(pageNr, false)
                }
            }
        }
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
        // if (pageNr > -1) // TODO
        //     pdfView.currentIndex = pageNr
    }

    Component.onCompleted: {
        if (PDFED.path !== "")
            pdfModel.loadPdfFile(PDFED.path)
    }
}
