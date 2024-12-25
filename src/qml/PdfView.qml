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

    property var dragPos: Qt.point(0, 0)
    property bool pageIsDragged: false
    property int dragOverlay: 0

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
        required property bool selected
        property alias pdfPage: pdfPage
        property alias img: img

        visible: index < pdfModel.pageCount
        width: GridView.view.cellWidth
        height: GridView.view.cellHeight

        onEntered: drag => {
            if (selected)
                return
            let from = drag.source.pageIndex
            let to = dropDelegate.delegateIndex
            if (from === to)
                return
            pdfModel.moveSelected(to)
        }

        QQC2.AbstractButton {
            id: pdfPage

            property int pageIndex: dropDelegate.delegateIndex
            property bool dragActive: false

            width: pdfModel.maxPageWidth
            height: pdfModel.maxPageWidth * pageRatio
            z: dragActive ? 5 : 1
            visible: !pdfView.pageIsDragged || !selected || index === pdfView.currentIndex

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
                opacity: pdfView.pageIsDragged && selected ? 0.5 : 1
                Behavior on rotation { NumberAnimation { easing.type: Easing.OutQuad } }
                Rectangle {
                    anchors.fill: parent
                    color: selected ? APP.alpha(Kirigami.Theme.highlightColor, 32) : "transparent"
                }
            }
            Rectangle {
                visible: bottomBar.labelsVisible
                anchors.bottom: parent.bottom
                height: Kirigami.Units.gridUnit * 2
                width: pdfPage.width
                color: pdfModel.labelColor(fileId)
                Column {
                    anchors { left: parent.left; leftMargin: Kirigami.Units.smallSpacing }
                    spacing: -Kirigami.Units.smallSpacing
                    Text {
                        height: Kirigami.Units.gridUnit
                        width: pdfPage.width - (Kirigami.Units.gridUnit + Kirigami.Units.smallSpacing) * 3
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        color: "#fff"
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 9
                        elide: Text.ElideRight
                        font { pixelSize: height }
                        text: pdfModel.getPdfName(fileId)
                    }
                    Text {
                        height: Kirigami.Units.gridUnit
                        anchors.left: parent.left
                        color: "#fff"
                        font { pixelSize: height * 0.9; bold: true }
                        text: origPage + 1
                    }
                }
                Text {
                    width: Kirigami.Units.gridUnit * 3; height: Kirigami.Units.gridUnit * 2
                    anchors { right: parent.right; rightMargin: Kirigami.Units.smallSpacing }
                    horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter
                    color: "#fff"
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 6
                    font { pixelSize: height * 0.9; bold: true }
                    text: (pageNr + 1)
                }
            }

            onClicked: {
                pdfModel.selectPage(index, pdfView.currentIndex !== index, bottomBar.multiSelect)
                pdfView.currentIndex = pdfView.currentIndex === index ? -1 : index
            }

            states: [
                State {
                    when: (!bottomBar.multiSelect && pdfPage.dragActive) || (pdfView.pageIsDragged && bottomBar.multiSelect && selected)
                    ParentChange {
                        target: pdfPage
                        parent: pdfView.contentItem
                    }
                }
            ]
        }
    }

    highlight: EditDelegate {}

    Timer {
        running: pdfView.pageIsDragged && dragOverlay !== 0
        interval: 50
        repeat: true
        onTriggered: pdfView.contentY += Math.ceil(pdfView.dragOverlay / 3)
    }
}
