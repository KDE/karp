// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2

Item {
    anchors.fill: parent
    visible: !deleted

    QQC2.Button {
        anchors { bottom: parent.bottom; left: parent.left }
        icon.name: "edit-delete"
        icon.color: "red"
        onClicked: {
            img.rotation = 0
            pdfModel.addDeletion(pageNr, true)
        }
    }
    QQC2.Button {
        anchors { top: parent.top; left: parent.left }
        icon.name: "object-rotate-left"
        onClicked: img.rotation = img.rotation > -270 ? img.rotation - 90 : 0
    }
    QQC2.Button {
        anchors { top: parent.top; right: parent.right }
        icon.name: "object-rotate-right"
        onClicked: img.rotation = img.rotation < 270 ? img.rotation + 90 : 0
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
