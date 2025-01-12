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
    id: tocView

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
                            menu.sender = treeDelegate
                            menu.modelIndex = treeView.index(row, column)
                            titleAction.text = title
                            addSubAction.enabled = !hasChildren
                            menu.popup()
                        }
                    }
                }
            }

            onClicked: tocView.bookmarkSelected(page)
        }
    }

    QQC2.Button {
        visible: tocView.rows < 1
        Layout.alignment: Qt.AlignCenter
        icon.source: "bookmark-new"
        text: i18n("Add Chapter")
        onClicked: {
            menu.modelIndex = tView.rootIndex
            openOutlineDialog(BookmarkModel.Insert.AtEnd, "", 0, text)
        }
    }

    QQC2.Menu {
        id: menu

        property var sender: null
        property var modelIndex: null

        Kirigami.Action {
            id: titleAction
            icon.name: "bookmark-edit"
            onTriggered: {
                openOutlineDialog(BookmarkModel.Insert.Edit, menu.sender.title, menu.sender.page, text)

            }
        }
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Insert Above")
            icon.name: "go-up"
            onTriggered: {
                openOutlineDialog(BookmarkModel.Insert.Above, "", menu.sender.page, text)

            }
        }
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Insert Below")
            icon.name: "go-down"
            onTriggered: {
                openOutlineDialog(BookmarkModel.Insert.Below, "", menu.sender.page, text)
            }
        }
        Kirigami.Action {
            id: addSubAction
            text: i18nc("@action:inmenu", "Add Subsection")
            icon.name: "bookmark-new"
            onTriggered: {
                openOutlineDialog(BookmarkModel.Insert.Inside, "", menu.sender.page, text)
            }
        }
    }

    function openOutlineDialog(whereToAdd: var, outlineTitle: string, pageNr: int, dialogTitle: string): void {
        outlineDlg = Qt.createComponent("org.kde.karp", "OutlineDialog").createObject(tocView,
                                                                                      {
                                                                                          pageCount: tocView.model.pageCount,
                                                                                          whereToAdd: whereToAdd,
                                                                                          bookmarkTitle: outlineTitle,
                                                                                          targetPage: pageNr + 1,
                                                                                          title: dialogTitle
                                                                                    })
        outlineDlg.accepted.connect(() => {
            tocView.model.insertBookmark(menu.modelIndex, outlineDlg.whereToAdd, outlineDlg.bookmarkTitle, outlineDlg.targetPage - 1)
            if (outlineDlg.whereToAdd === BookmarkModel.Insert.Inside && menu.sender)
                tView.expand(menu.sender.row)
                menu.sender = null
        })
        outlineDlg.removed.connect(() => tocView.model.removeOutline(menu.modelIndex))
    }

    property var outlineDlg
}
