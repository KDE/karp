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

    ListView {
        id: pdfView
        visible: PDFED.pdfLoaded
        width: page.width / 2; height: page.height * 0.8
        clip: true
        spacing: height * 0.1
        model: PDFED.pdfModel
        currentIndex: -1

        delegate: Rectangle {
            id: delegRect
            property bool checked: pdfView.currentIndex === index
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
                    text: (index + 1) + " <font size=\"1\">(" + (index + 1) + ")</font>"
                }
            }
            MouseArea {
                id: ma
                anchors.fill: parent
                onClicked: pdfView.currentIndex = index
            }
            QQC2.Button {
                visible: delegRect.checked
                anchors { top: parent.top; left: parent.left }
                icon.name: "edit-delete"
                icon.color: "red"
                onClicked: console.log("delete Page", index)
            }
            QQC2.Button {
                visible: delegRect.checked
                anchors { verticalCenter: parent.verticalCenter; left: parent.left }
                icon.name: "object-rotate-left"
                onClicked: img.rotation = img.rotation > -270 ? img.rotation - 90 : 0
            }
            QQC2.Button {
                visible: delegRect.checked
                anchors { verticalCenter: parent.verticalCenter; right: parent.right }
                icon.name: "object-rotate-right"
                onClicked: img.rotation = img.rotation < 270 ? img.rotation + 90 : 0
            }
        }
    } // ListView

    QQC2.Button {
        visible: PDFED.pdfLoaded
        enabled: PDFED.pdfModel.edited
        anchors { bottom: parent.bottom; right: parent.right }
        icon.name: "application-pdf"
        text: i18n("Generate")
    }
}
