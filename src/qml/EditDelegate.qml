// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Rectangle {
    id: editDelg
    anchors.fill: parent

    required property var pdfView
    required property var pdfModel
    property int pageNr: pdfView.currentItem ? pdfView.currentIndex : 0
    property bool selected: pdfView.currentItem ? pdfView.currentItem.selected : false
    property bool hasOutline: pdfView.currentItem ? pdfView.currentItem.hasOutline : false

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
                editDelg.pdfView.pageIsDragged = active
                if (pdfView.currentItem)
                    pdfView.currentItem.pdfPage.dragActive = active
            }
            onActiveTranslationChanged: {
                let posY = dragButt.mapToItem(editDelg.pdfView, dragButt.x, dragButt.y).y
                if (posY < Kirigami.Units.gridUnit * 2 && !editDelg.pdfView.atYBeginning)
                    pdfView.dragOverlay = posY - Kirigami.Units.gridUnit * 2
                else if (posY > pdfView.height - Kirigami.Units.gridUnit * 4 && !pdfView.atYEnd)
                    pdfView.dragOverlay = posY - (pdfView.height - Kirigami.Units.gridUnit * 4)
                else
                    pdfView.dragOverlay = 0
            }
        }
    }
    QQC2.Button {
        visible: !dragHandler.active
        z: 1
        anchors { bottom: parent.bottom; left: parent.left; bottomMargin: Kirigami.Units.gridUnit * 2 }
        icon.name: "edit-delete"
        icon.color: "red"
        onClicked: {
            editDelg.pdfModel.deletePage(editDelg.pageNr)
            editDelg.pdfView.currentIndex = -1
        }
    }
    QQC2.Button {
        visible: !dragHandler.active
        z: 1
        anchors { top: parent.top; left: parent.left }
        icon.name: "object-rotate-left"
        onClicked: editDelg.pdfModel.rotatePage(editDelg.pageNr, editDelg.pdfView.currentItem.img.rotation > -270 ? editDelg.pdfView.currentItem.img.rotation - 90 : 0)
    }
    QQC2.Button {
        id: rotLeftButt
        visible: !dragHandler.active
        z: 1
        anchors { top: parent.top; right: parent.right }
        icon.name: "object-rotate-right"
        onClicked: editDelg.pdfModel.rotatePage(editDelg.pageNr, editDelg.pdfView.currentItem.img.rotation < 270 ? editDelg.pdfView.currentItem.img.rotation + 90 : 0)
    }
    QQC2.Button {
        id: outlineButt
        visible: editDelg.hasOutline && !dragHandler.active
        z: 1
        anchors { top: rotLeftButt.bottom; left: parent.left }
        icon.name: editDelg.hasOutline ? "bookmarks-bookmarked" : "bookmarks"
        onClicked: {
            let menu = outlineComp.createObject(outlineButt, { model: editDelg.pdfModel.getPageOutlines(editDelg.pageNr) })
            menu.selected.connect((index) => {
                var idx = pdfModel.indexFromOutline(pageNr, index)
                let outlineDlg = Qt.createComponent("org.kde.karp", "OutlineDialog").createObject(editDelg,
                                                                                                  {
                                                                                                      whereToAdd: BookmarkModel.Insert.Edit,
                                                                                                      bookmarkTitle: pdfModel.outlineTitle(idx),
                                                                                                      targetPage: pdfModel.outlinePage(idx) + 1,
                                                                                                      pageCount: pdfModel.pageCount
                                                                                                })
                outlineDlg.accepted.connect(() => pdfModel.insertBookmark(idx, BookmarkModel.Insert.Edit, outlineDlg.bookmarkTitle, outlineDlg.targetPage - 1))
                outlineDlg.removed.connect(() => pdfModel.removeOutline(idx))
            })
            menu.popup()
        }
    }
    // move at upper row
    QQC2.Button {
        visible: !dragHandler.active && editDelg.pageNr >= editDelg.pdfModel.columns && editDelg.pageNr - pdfModel.columns < pdfModel.firstSelected - 1
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }
        icon.name: "arrow-up"
        onClicked: editDelg.pdfModel.moveSelected(editDelg.pageNr - editDelg.pdfModel.columns)
    }
    // move at lower row
    QQC2.Button {
        visible: !dragHandler.active && editDelg.pageNr < editDelg.pdfModel.pageCount - pdfModel.columns && pageNr + pdfModel.columns > pdfModel.lastSelected + 1
        z: 1
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: Kirigami.Units.gridUnit * 2 }
        icon.name: "arrow-down"
        onClicked: editDelg.pdfModel.moveSelected(editDelg.pageNr + editDelg.pdfModel.columns)
    }
    // move at next column
    QQC2.Button {
        visible: !dragHandler.active && editDelg.pageNr < editDelg.pdfModel.pageCount - 1 && pageNr + 1 > pdfModel.lastSelected - 1
        z: 1
        anchors { verticalCenter: parent.verticalCenter; right: parent.right }
        icon.name: "arrow-right"
        onClicked: editDelg.pdfModel.moveSelected(editDelg.pageNr + 2)
    }
    // move at previous column
    QQC2.Button {
        visible: !dragHandler.active && editDelg.pageNr > 0 && editDelg.pageNr - 1 < editDelg.pdfModel.firstSelected - 1
        z: 1
        anchors { verticalCenter: parent.verticalCenter; left: parent.left }
        icon.name: "arrow-left"
        onClicked: editDelg.pdfModel.moveSelected(editDelg.pageNr - 1)
    }

    Component {
        id: outlineComp
        // TODO: adjust for mobile
        QQC2.Menu {
            id: outlineMenu
            property var model: null
            signal selected(var index)
            width: Math.min(Kirigami.Units.gridUnit * 20, implicitWidth)
            Component.onCompleted: {
                for (let o = 0; o < model.length; ++o) {
                    let newAct = actionComp.createObject(outlineMenu)
                    newAct.text = model[o]
                    newAct.index = o
                    newAct.triggered.connect(() => outlineMenu.selected(newAct.index))
                    outlineMenu.addAction(newAct)
                }
            }
            onClosed: destroy()
        }
    }
    Component {
        id: actionComp
        Kirigami.Action {
            property int index: -1
        }
    }
}
