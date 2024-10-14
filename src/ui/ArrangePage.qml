// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Pdf
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.Page {
    id: page

    title: i18n("Arrange PDF Pages") + ": " + PDFED.name

    actions: [
        Kirigami.Action {
            visible: PDFED.pdfLoaded
            enabled: PDFED.pdfModel && PDFED.pdfModel.edited
            icon.name: "application-pdf"
            text: i18n("Generate")
            onTriggered: PDFED.pdfModel.generate()
        }
    ]

    QQC2.Button {
        visible: !PDFED.pdfLoaded
        anchors.centerIn: parent
        text: i18n("Select PDF file")
        icon.name: "application-pdf"
        onClicked: {
            PDFED.getPdfFile()
            PDFED.pdfModel.maxPageWidth = Qt.binding(function() { return page.width / 3 })
        }
    }

    Rectangle {
        visible: PDFED.pdfLoaded
        anchors.fill: pdfView
        color: Kirigami.Theme.alternateBackgroundColor
    }

    ListView {
        id: pdfView
        visible: PDFED.pdfLoaded
        width: page.width - Kirigami.Units.largeSpacing * 4
        height: page.height - bottomRect.height - Kirigami.Units.largeSpacing
        clip: true
        spacing: Kirigami.Units.smallSpacing
        model: PDFED.pdfModel
        currentIndex: -1

        delegate: Rectangle {
            id: delegRect
            property bool checked: pdfView.currentIndex === index
            x: Kirigami.Units.smallSpacing
            width: img.width + 4; height: img.height + 4
            color: "transparent"
            border {
                width: checked ? 3 : 0
                color: Kirigami.Theme.highlightColor
            }
            PdfPageItem {
                id: img
                x: 2; y: 2; z: -1
                image: pageImg
                rotation: rotated
                onRotationChanged: PDFED.pdfModel.addRotation(index, rotation)
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
                    text: (index + 1) + " <font size=\"1\">(" + (origPage + 1) + ")</font>"
                }
            }
            MouseArea {
                id: ma
                anchors.fill: parent
                onClicked: pdfView.currentIndex = index
            }
            Item {
                anchors.fill: parent
                visible: delegRect.checked && !deleted
                QQC2.Button {
                    anchors { top: parent.top; left: parent.left }
                    icon.name: "edit-delete"
                    icon.color: "red"
                    onClicked: {
                        img.rotation = 0
                        PDFED.pdfModel.addDeletion(index, true)
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
                    visible: pdfView.currentIndex !== 0
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
                    icon.name: "go-up"
                    onClicked: movePage(index, index - 1)
                }
                QQC2.Button {
                    visible: pdfView.currentIndex !== pdfView.count - 1
                    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
                    icon.name: "go-down"
                    onClicked: movePage(index, index + 1)
                }
            }
            Loader {
                active: deleted
                z: 5 // atop of mouse area
                anchors.fill: parent
                sourceComponent: DeletedDelegate {
                    buttonVisible: delegRect.checked
                    onWantRevert: PDFED.pdfModel.addDeletion(index, false)
                }
            }
        }
        QQC2.ScrollBar.vertical: QQC2.ScrollBar { visible: true }
    } // ListView

    footer: Rectangle {
        id: bottomRect
        width: page.width; height: Kirigami.Units.gridUnit * 2
        color: Kirigami.Theme.alternateBackgroundColor
    }

    function movePage(from, to) {
        var pageNr = PDFED.pdfModel.addMove(from, to)
        if (pageNr > -1)
            pdfView.currentIndex = pageNr
    }
}
