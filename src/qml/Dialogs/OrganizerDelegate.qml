// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import QtQuick.Layouts
import org.kde.karp

Kirigami.AbstractCard {
    id: fileDelegate

    property alias dragActive: dragHandler.active

    contentItem: Item {
        implicitWidth: rowLay.implicitWidth
        implicitHeight: rowLay.implicitHeight

        RowLayout {
            id: rowLay
            anchors { left: parent.left; top: parent.top; right: parent.right }
            QQC2.Label {
                text: (index + 1)
            }
            Kirigami.Icon {
                source: locked ? "lock" : "handle-sort"
                Layout.fillHeight: true
                Layout.maximumHeight: Kirigami.Units.iconSizes.large
                Layout.preferredWidth: height
                DragHandler {
                    id: dragHandler
                    target: fileDelegate
                    xAxis.enabled: false
                    yAxis.enabled: !locked
                    cursorShape: Qt.DragMoveCursor
                }
            }
            ColumnLayout {
                spacing: 0

                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1

                QQC2.Label {
                    text: fileName
                    font.bold: true
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                QQC2.Label {
                    text: path
                    elide: Text.ElideLeft
                    Layout.fillWidth: true
                }
            }

            QQC2.Label {
                text: i18np("%1 page", "%1 pages", pageCount)
                Layout.alignment: Qt.AlignRight
            }
            // Action bar is shrunk by Layout so it shows only '⋮' which is exactly what we want
            Kirigami.ActionToolBar {
                // TODO: Are we going to allow removing previously added file(s)
                // Their pages can be intertwined already.
                // Still it is possible and not that much difficult, bu is this really necessary?
                // For now it is disabled
                enabled: !locked
                actions: [
                    Kirigami.Action {
                        enabled: !locked
                        visible: false
                        text: i18n("Add all pages")
                        icon.name: "page-simple"
                        checkable: true
                        checked: selectAll
                    },
                    Kirigami.Action {
                        enabled: !locked
                        visible: false
                        text: i18n("Add selected pages")
                        icon.name: "page-simple"
                        checkable: true
                        checked: !selectAll
                        onTriggered: {
                            selectComp.createObject()
                        }
                    },
                    Kirigami.Action {
                        text: i18n("Remove from list")
                        icon.name: "user-trash"
                        onTriggered: lv.model.remove(index)
                    }
                ]
            }
        } // RowLayout
    } // Item (contentItem)

    Component {
        id: selectComp
        SelectPagesDialog {
            visible: true
            title: i18n("Select pages to add")
            pageCount: fileDelegate.pageCount
            onAccepted: console.log("Accepted")
        }
    }
}
