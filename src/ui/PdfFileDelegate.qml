// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import QtQuick.Layouts
import org.kde.deafed

Kirigami.AbstractCard {
    property ListView lv: ListView.view
    required property int index
    required property string path
    required property string fileName
    required property int pageCount
    required property bool locked

    z: dragArea.pressed ? 5 : 1

    contentItem: Item {
        implicitWidth: rowLay.implicitWidth
        implicitHeight: rowLay.implicitHeight
        Rectangle {
            id: drawRect
            width: parent.width; height: parent.height
            color: {
                if (dragArea.pressed)
                    return PDFED.alpha(Kirigami.Theme.textColor, 50)
                else if (lv.dragTargetIndex === index)
                    return PDFED.alpha(Kirigami.Theme.highlightColor, 150)
                else
                    return "transparent"
            }
            Behavior on y { NumberAnimation {} }
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
                    MouseArea {
                        id: dragArea
                        anchors.fill: parent
                        drag.target: drawRect
                        drag.axis: Drag.YAxis
                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                        onPositionChanged: (mouse) => {
                            let pos = mapToItem(lv.contentItem, mouse.x, mouse.y)
                            let targetId = lv.indexAt(pos.x, pos.y)
                            if (targetId !== index && !lv.itemAtIndex(targetId)?.locked)
                                lv.dragTargetIndex = targetId
                        }
                        onReleased: (mouse) => {
                            let pos = mapToItem(lv.contentItem, mouse.x, mouse.y)
                            let targetId = lv.indexAt(pos.x, pos.y)
                            if (targetId !== index)
                                lv.model.move(index, targetId)
                            drawRect.y = 0
                            lv.dragTargetIndex = -1
                        }
                    }
                }
                QQC2.Label {
                    visible: showPathAction.checked
                    text: path
                    elide: Text.ElideLeft
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: fileName
                    font.bold: true
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                    Layout.horizontalStretchFactor: 1
                }
                QQC2.Label {
                    text: i18n("%1 pages", pageCount)
                    Layout.alignment: Qt.AlignRight
                }
                // Action bar is shrunk by Layout so it shows only '⋮' which is exactly what we want
                Kirigami.ActionToolBar {
                    actions: [
                        Kirigami.Action {
                            enabled: false
                            text: i18n("Add only odd pages")
                            icon.name: "page-simple"
                        },
                        Kirigami.Action {
                            enabled: false
                            text: i18n("Add only even pages")
                            icon.name: "page-simple"
                        },
                        Kirigami.Action {
                            enabled: false
                            text: i18n("Add every N page")
                            icon.name: "page-simple"
                        },
                        Kirigami.Action {
                            text: i18n("Remove from list")
                            icon.name: "user-trash"
                            onTriggered: lv.model.remove(index)
                        }
                    ]
                }
            } // RowLayout
        } // Rectangle
    } // Item (contentItem)
}