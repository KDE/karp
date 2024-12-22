// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Rectangle {
    id: editDelg
    anchors.fill: parent

    property int pageNr: pdfView.currentItem ? pdfView.currentIndex : 0
    property bool selected: pdfView.currentItem ? pdfView.currentItem.selected : false

    visible: pdfView.currentItem !== null
    parent: pdfView.currentItem ? pdfView.currentItem.pdfPage : null
    color: APP.alpha(Kirigami.Theme.highlightColor, dragHandler.active ? 32 : 0)
    border {
        width: pdfView.currentIndex === pageNr ? 5 : 0
        color: Kirigami.Theme.highlightColor
    }
    z: 2

    QQC2.Button {
        id: dragButt
        z: 1
        anchors.centerIn: parent
        icon.name: "transform-move"
        scale: dragHandler.active ? 1.5 : 1
        DragHandler {
            id: dragHandler
            target: editDelg.parent
            cursorShape: Qt.DragMoveCursor
            onActiveChanged: {
                pdfView.pageIsDragged = active
                if (pdfView.currentItem)
                    pdfView.currentItem.pdfPage.dragActive = active
            }
            onCentroidChanged: {
                pdfView.dragPos = Qt.point(centroid.scenePosition.x - centroid.scenePressPosition.x, centroid.scenePosition.y - centroid.scenePressPosition.y)
            }
        }
    }
    QQC2.Button {
        visible: !dragHandler.active
        z: 1
        anchors { bottom: parent.bottom; left: parent.left; bottomMargin: Kirigami.Units.gridUnit * 2 }
        icon.name: "edit-delete"
        icon.color: "red"
        onClicked: pdfModel.deletePage(pageNr)
    }
    QQC2.Button {
        visible: !dragHandler.active
        z: 1
        anchors { top: parent.top; left: parent.left }
        icon.name: "object-rotate-left"
        onClicked: pdfModel.rotatePage(pageNr, pdfView.currentItem.img.rotation > -270 ? pdfView.currentItem.img.rotation - 90 : 0)
    }
    QQC2.Button {
        id: rotLeftButt
        visible: !dragHandler.active
        z: 1
        anchors { top: parent.top; right: parent.right }
        icon.name: "object-rotate-right"
        onClicked: pdfModel.rotatePage(pageNr, pdfView.currentItem.img.rotation < 270 ? pdfView.currentItem.img.rotation + 90 : 0)
    }
    // move at upper row
    QQC2.Button {
        visible: !dragHandler.active && pageNr >= pdfModel.columns && pageNr - pdfModel.columns < pdfModel.firstSelected - 1
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
        icon.name: "arrow-up"
        onClicked: pdfModel.moveSelected(pageNr - pdfModel.columns)
    }
    // move at lower row
    QQC2.Button {
        visible: !dragHandler.active && pageNr < pdfModel.pageCount - pdfModel.columns && pageNr + pdfModel.columns > pdfModel.lastSelected + 1
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: Kirigami.Units.gridUnit * 2 }
        icon.name: "arrow-down"
        onClicked: pdfModel.moveSelected(pageNr + pdfModel.columns)
    }
    // move at next column
    QQC2.Button {
        visible: !dragHandler.active && pageNr < pdfModel.pageCount - 1 && pageNr + 1 > pdfModel.lastSelected - 1
        z: 1
        anchors { verticalCenter: parent.verticalCenter; right: parent.right }
        icon.name: "arrow-right"
        onClicked: pdfModel.moveSelected(pageNr + 2)
    }
    // move at previous column
    QQC2.Button {
        visible: !dragHandler.active && pageNr > 0 && pageNr - 1 < pdfModel.firstSelected - 1
        z: 1
        anchors { verticalCenter: parent.verticalCenter; left: parent.left }
        icon.name: "arrow-left"
        onClicked: pdfModel.moveSelected(pageNr - 1)
    }
}
