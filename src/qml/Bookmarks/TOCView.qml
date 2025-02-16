// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.karp
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

ColumnLayout {
    id: tocView

    function openOutlineDialog(whereToAdd: var, outlineTitle: string, pageNr: int, dialogTitle: string) : void {
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

    property alias model: tView.model
    property alias rows: tView.rows
    property var outlineDlg

    signal bookmarkSelected(var pageNr)
    signal tocAboutToClear()

    Kirigami.SearchField {
        visible: false // TODO
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
            onClicked: tocView.bookmarkSelected(page)

            QQC2.ToolTip {
                text: treeDelegate.title
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
                    text: treeDelegate.hovered ? " \u22EE " : treeDelegate.page + 1
                    Layout.alignment: Qt.AlignRight

                    MouseArea {
                        id: ma

                        hoverEnabled: true
                        anchors.fill: parent
                        onClicked: {
                            menu.sender = treeDelegate;
                            menu.modelIndex = tView.index(treeDelegate.row, 0);
                            titleAction.text = treeDelegate.title;
                            addSubAction.enabled = !treeDelegate.hasChildren;
                            menu.popup();
                        }
                    }

                    background: Rectangle {
                        color: "transparent"
                        radius: 2

                        border {
                            width: ma.containsMouse ? 2 : 0
                            color: Kirigami.Theme.highlightColor
                        }

                    }

                }

            }

        }

    }

    Kirigami.ActionToolBar {
        alignment: Qt.AlignCenter
        actions: [
            Kirigami.Action {
                icon.name: "bookmark-new"
                text: tocView.rows < 1 ? i18n("Add Chapter") : ""
                tooltip: i18n("Add Chapter")
                onTriggered: {
                    menu.modelIndex = tView.rootIndex;
                    tocView.openOutlineDialog(BookmarkModel.Insert.AtEnd, "", 0, text);
                }
            },
            Kirigami.Action {
                visible: tocView.rows
                icon.name: "expand-all"
                tooltip: i18nc("@action:intoolbar", "Expand All")
                onTriggered: tView.expandRecursively()
            },
            Kirigami.Action {
                visible: tocView.rows
                icon.name: "collapse-all"
                tooltip: i18nc("@action:intoolbar", "Collapse All")
                onTriggered: {
                    tView.collapseRecursively()
                    tView.returnToBounds()
                }
            },
            Kirigami.Action {
                id: clearAllAct
                visible: tocView.rows
                icon.name: "edit-delete"
                text: i18nc("@action:intoolbar", "Clear Table of Contents")
                onTriggered: clearTOCcomp.createObject(tocView)
            }
        ]
    }

    QQC2.Menu {
        id: menu

        property var sender: null
        property var modelIndex: null

        Kirigami.Action {
            id: titleAction

            icon.name: "bookmark-edit"
            onTriggered: {
                openOutlineDialog(BookmarkModel.Insert.Edit, menu.sender.title, menu.sender.page, i18n("Edit Chapter"));
            }
        }

        Kirigami.Action {
            text: i18nc("@action:inmenu", "Insert Above")
            icon.name: "go-up"
            onTriggered: {
                 tocView.openOutlineDialog(BookmarkModel.Insert.Above, "", menu.sender.page, text);
            }
        }

        Kirigami.Action {
            text: i18nc("@action:inmenu", "Insert Below")
            icon.name: "go-down"
            onTriggered: {
                 tocView.openOutlineDialog(BookmarkModel.Insert.Below, "", menu.sender.page, text);
            }
        }

        Kirigami.Action {
            id: addSubAction

            text: i18nc("@action:inmenu", "Add Subsection")
            icon.name: "bookmark-new"
            onTriggered: {
                 tocView.openOutlineDialog(BookmarkModel.Insert.Inside, "", menu.sender.page, text);
            }
        }

        Kirigami.Action {
            enabled: !addSubAction.enabled && menu.sender && !menu.sender.expanded
            text: i18nc("@action:inmenu", "Expand whole section")
            onTriggered: {
                tView.expandRecursively(menu.sender.row, -1)
            }
        }

        Kirigami.Action {
            enabled: !addSubAction.enabled && menu.sender && menu.sender.expanded
            text: i18nc("@action:inmenu", "Collapse whole section")
            onTriggered: {
                tView.collapseRecursively(menu.sender.row)
            }
        }
    }

    Component {
        id: clearTOCcomp
        Kirigami.Dialog {
            visible: true
            title: clearAllAct.text
            padding: Kirigami.Units.largeSpacing
            showCloseButton: false
            flatFooterButtons: false
            QQC2.Label {
                text: i18n("Are you sure you'd like to remove all chapters?")
            }

            standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
            onAccepted: {
                tocView.tocAboutToClear()
                tocView.model.clear()
            }
            onClosed: destroy()
        }
    }

}
