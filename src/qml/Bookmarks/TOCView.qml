// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import QtQuick.Layouts
import org.kde.karp

ColumnLayout {
    id: bookPane

    property alias model: tView.model
    property alias rows: tView.rows

    signal bookmarkSelected(var pageNr)

    Kirigami.SearchField {
        visible: false
        Layout.fillWidth: true
    }

    TreeView {
        id: tView

        Layout.fillWidth: true
        Layout.fillHeight: true

        clip: true

        // selectionModel: ItemSelectionModel {
        //     id: selMod
        // }

        delegate: QQC2.TreeViewDelegate {
            id: treeDelegate

            required property string title
            required property int page

            implicitWidth: TableView.view.width

            QQC2.ToolTip {
                text: title
            }

            contentItem: RowLayout {
                id: layout
                spacing: Kirigami.Units.smallSpacing

                KD.TitleSubtitle {
                    id: nameLabel
                    title: treeDelegate.title
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: treeDelegate.hovered ? "\u22EE" : treeDelegate.page + 1
                    Layout.alignment: Qt.AlignRight
                    rightPadding: Kirigami.Units.smallSpacing
                    background: Rectangle {
                        border {
                            width: ma.containsMouse ? 2 : 0
                            color: Kirigami.Theme.highlightColor
                        }
                        color: "transparent"
                        radius: 2
                    }
                    MouseArea {
                        id: ma
                        hoverEnabled: true
                        anchors.fill: parent
                        onClicked: {
                            outlineDlg.sender = treeDelegate
                            outlineDlg.modelIndex = treeView.index(row, column)
                            titleAction.text = title
                            addSubAction.enabled = !hasChildren
                            menu.popup()
                        }
                    }
                }
            }

            onClicked: bookPane.bookmarkSelected(page)
        }
    }

    QQC2.Button {
        visible: bookPane.rows < 1
        Layout.alignment: Qt.AlignCenter
        icon.source: "bookmark-new"
        text: i18n("Add Chapter")
        onClicked: {
            outlineDlg.modelIndex = tView.rootIndex // Requires Qt 6.6
            outlineDlg.whereToAdd = BookmarkModel.Insert.AtEnd
            outlineDlg.title = text
            outlineDlg.open()
        }
    }

    QQC2.Menu {
        id: menu

        Kirigami.Action {
            id: titleAction
            icon.name: "bookmark-edit"
            onTriggered: {
                outlineDlg.title = text
                outlineDlg.whereToAdd = BookmarkModel.Insert.Edit
                outlineDlg.bookmarkTitle = outlineDlg.sender.title
                outlineDlg.targetPage = outlineDlg.sender.page + 1
                outlineDlg.open()
            }
        }
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Insert Above")
            icon.name: "go-up"
            onTriggered: {
                outlineDlg.title = text
                outlineDlg.whereToAdd = BookmarkModel.Insert.Above
                outlineDlg.open()
            }
        }
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Insert Below")
            icon.name: "go-down"
            onTriggered: {
                outlineDlg.title = text
                outlineDlg.whereToAdd = BookmarkModel.Insert.Below
                outlineDlg.open()
            }
        }
        Kirigami.Action {
            id: addSubAction
            text: i18nc("@action:inmenu", "Add Subsection")
            icon.name: "bookmark-new"
            onTriggered: {
                outlineDlg.title = text
                outlineDlg.whereToAdd = BookmarkModel.Insert.Inside
                outlineDlg.open()
            }
        }
    }

    OutlineDialog {
        id: outlineDlg
        property var sender: null
        pageCount: bookPane.model.pageCount
        onAccepted: {
            bookPane.model.insertBookmark(modelIndex, whereToAdd, bookmarkTitle, targetPage - 1)
            if (whereToAdd === BookmarkModel.Insert.Inside && sender)
                tView.expand(sender.row)
            sender = null
            bookmarkTitle = ""
        }
    }
}
