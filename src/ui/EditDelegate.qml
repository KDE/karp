// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2

Item {
    id: editDelg
    anchors.fill: parent
    visible: !deleted

    /**
     * Calculates page number from given @p mouse position
     */
    function pageAtMouse(mouse) {
        var pos = dragButt.mapToItem(pdfView.contentItem, mouse.x, mouse.y)
        var cellPos = pdfView.cellAtPosition(pos.x, pos.y, true)
        return cellPos.y * pdfView.columns + cellPos.x
    }

    QQC2.Button {
        id: dragButt
        anchors.centerIn: parent
        icon.name: "transform-move"
        MouseArea {
            anchors.fill: parent
            drag.target: img
            drag.axis: Drag.XAndYAxis
            onPressed: img.opacity = 0.3
            onPositionChanged: (mouse) => {
                var targetPage = pageAtMouse(mouse)
                pdfView.dragTargetPage = targetPage === pageNr ? -1 : targetPage
            }
            onReleased: (mouse) => {
                img.enableAnimation = true
                img.x = (editDelg.width - img.width) / 2
                img.y = (editDelg.height - img.height) / 2
                img.opacity = 1
                pdfView.dragTargetPage = -1
                var targetPage = pageAtMouse(mouse)
                if (targetPage !== pageNr)
                    movePage(pageNr, targetPage)
            }
        }
    }
    QQC2.Button {
        anchors { bottom: parent.bottom; left: parent.left }
        icon.name: "edit-delete"
        icon.color: "red"
        onClicked: {
            pdfModel.addRotation(pageNr, 0)
            pdfModel.addDeletion(pageNr, true)
        }
    }
    QQC2.Button {
        anchors { top: parent.top; left: parent.left }
        icon.name: "object-rotate-left"
        onClicked: pdfModel.addRotation(pageNr, img.rotation > -270 ? img.rotation - 90 : 0)
    }
    QQC2.Button {
        anchors { top: parent.top; right: parent.right }
        icon.name: "object-rotate-right"
        onClicked: pdfModel.addRotation(pageNr, img.rotation < 270 ? img.rotation + 90 : 0)
    }
    // move at upper row
    QQC2.Button {
        visible: pageNr > pdfView.columns
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
        icon.name: "go-up"
        onClicked: movePage(pageNr, pageNr - pdfView.columns)
    }
    // move at lower row
    QQC2.Button {
        visible: pageNr < pdfModel.pageCount - pdfView.columns
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        icon.name: "go-down"
        onClicked: movePage(pageNr, pageNr + pdfView.columns)
    }
    // move at next column
    QQC2.Button {
        visible: pageNr < pdfModel.pageCount - 1
        anchors { verticalCenter: parent.verticalCenter; right: parent.right }
        icon.name: "go-right"
        onClicked: movePage(pageNr, pageNr + 1)
    }
    // move at previous column
    QQC2.Button {
        visible: pageNr > 0
        anchors { verticalCenter: parent.verticalCenter; left: parent.left }
        icon.name: "go-left"
        onClicked: movePage(pageNr, pageNr - 1)
    }
}
