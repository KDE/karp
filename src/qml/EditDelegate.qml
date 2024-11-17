// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2

Item {
    id: editDelg
    anchors.fill: parent

    visible: current

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
        z: 1
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
        onClicked: pdfModel.rotatePage(pageNr, img.rotation > -270 ? img.rotation - 90 : 0)
    }
    QQC2.Button {
        z: 1
        anchors { top: parent.top; right: parent.right }
        icon.name: "object-rotate-right"
        onClicked: pdfModel.rotatePage(pageNr, img.rotation < 270 ? img.rotation + 90 : 0)
    }
    // move at upper row
    QQC2.Button {
        visible: pageNr > pdfView.columns
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
        icon.name: "arrow-up"
        onClicked: movePage(pageNr, pageNr - pdfView.columns - 1)
    }
    // move at lower row
    QQC2.Button {
        visible: pageNr < pdfModel.pageCount - pdfView.columns
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        icon.name: "arrow-down"
        onClicked: movePage(pageNr, pageNr + pdfView.columns)
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

    /**
     * This Mouse gives possibility to clear current index and hide EditDelegate
     * When current index is cleared, it changes parent to delegate item directly
     * to be clickable and allow revert current index on this cell
     */
    MouseArea {
        id: ma
        anchors.fill: parent
        onClicked: {
            if (current)
                pdfView.selectionModel.clearCurrentIndex()
            else
                pdfView.selectionModel.setCurrentIndex(pdfView.index(pageNr / pdfView.columns, pageNr % pdfView.columns), ItemSelectionModel.Current)
            ma.parent = current ? editDelg : editDelg.parent
        }
    }
}
