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
        DragHandler {
            id: dragHandler
            target: editDelg.parent
            cursorShape: Qt.DragMoveCursor
            onActiveChanged: {
                if (pdfView.currentItem)
                    pdfView.currentItem.pdfPage.dragActive = active
            }
        }
    }
    QQC2.Button {
        z: 1
        anchors { bottom: parent.bottom; left: parent.left }
        icon.name: "edit-delete"
        icon.color: "red"
        onClicked: pdfModel.deletePage(pageNr)
    }
    QQC2.Button {
        z: 1
        anchors { top: parent.top; left: parent.left }
        icon.name: "object-rotate-left"
        onClicked: pdfModel.rotatePage(pageNr, pdfView.currentItem.img.rotation > -270 ? pdfView.currentItem.img.rotation - 90 : 0)
    }
    QQC2.Button {
        z: 1
        anchors { top: parent.top; right: parent.right }
        icon.name: "object-rotate-right"
        onClicked: pdfModel.rotatePage(pageNr, pdfView.currentItem.img.rotation < 270 ? pdfView.currentItem.img.rotation + 90 : 0)
    }
    // move at upper row
    QQC2.Button {
        visible: pageNr >= pdfModel.columns
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
        icon.name: "arrow-up"
        onClicked: movePage(pageNr, pageNr - pdfModel.columns)
    }
    // move at lower row
    QQC2.Button {
        visible: pageNr < pdfModel.pageCount - pdfModel.columns
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        icon.name: "arrow-down"
        onClicked: movePage(pageNr, pageNr + pdfModel.columns)
    }
    // move at next column
    QQC2.Button {
        visible: pageNr < pdfModel.pageCount - 1
        z: 1
        anchors { verticalCenter: parent.verticalCenter; right: parent.right }
        icon.name: "arrow-right"
        onClicked: movePage(pageNr, pageNr + 1)
    }
    // move at previous column
    QQC2.Button {
        visible: pageNr > 0
        z: 1
        anchors { verticalCenter: parent.verticalCenter; left: parent.left }
        icon.name: "arrow-left"
        onClicked: movePage(pageNr, pageNr - 1)
    }
}
