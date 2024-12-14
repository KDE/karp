// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQml.Models
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.karp
import org.kde.karp.config

GridView {
    id: pdfView

    clip: true
    leftMargin: Kirigami.Units.largeSpacing
    rightMargin: Kirigami.Units.largeSpacing
    topMargin: Kirigami.Units.largeSpacing
    bottomMargin: Kirigami.Units.largeSpacing + bottomBar.height

    displaced: Transition {
        NumberAnimation {
            properties: "x,y"
            easing.type: Easing.OutQuad
        }
    }
    moveDisplaced: Transition {
        NumberAnimation {
            properties: "x,y"
            easing.type: Easing.OutQuad
        }
    }

    currentIndex: -1

    delegate: DropArea {
        id: dropDelegate

        property int delegateIndex: index
        required property int index
        required property int pageNr
        required property int fileId
        required property int origPage
        required property real pageRatio
        required property var pageImg
        required property int rotated
        property alias pdfPage: pdfPage
        property alias img: img

        width: GridView.view.cellWidth
        height: GridView.view.cellHeight

        onEntered: drag => {
            let from = drag.source.pageIndex
            let to = dropDelegate.delegateIndex
            if (from === to)
                return
            pdfView.movePage(from, to)
        }

        QQC2.AbstractButton {
            id: pdfPage

            property int pageIndex: dropDelegate.delegateIndex
            property bool dragActive: false

            width: pdfModel.maxPageWidth
            height: pdfModel.maxPageWidth * pageRatio
            z: dragActive ? 5 : 1

            Drag.active: dragActive
            Drag.source: pdfPage
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2

            background: PdfPageItem {
                id: img
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                image: pageImg
                scale: parent.width / (rotated === 90 || rotated === 270 ? height : width)
                rotation: rotated
                onRotationChanged: console.log(rotation)
                opacity: pdfPage.dragActive ? 0.5 : 1
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

            onClicked: {
                pdfView.currentIndex = pdfView.currentIndex === index ? -1 : index
            }

            states: [
                State {
                    when: pdfPage.dragActive
                    ParentChange {
                        target: pdfPage
                        parent: pdfView.contentItem
                    }
                }
            ]
        }
    }

    highlight: EditDelegate {}

    function movePage(from: int, to: int) : void {
        const targetPage = pdfModel.movePage(from, to)
    }
}
