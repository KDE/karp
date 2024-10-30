// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import QtQuick.Layouts
import org.kde.deafed

FormCard.FormCardDialog {
    id: pdfsDialog

    property alias pdfEdit: pdfOrg.editModel
    property alias initFiles: pdfOrg.initFiles

    title: i18n("Add and arrange PDF files")
    visible: true
    width: mainWin.width - Kirigami.Units.gridUnit * 2
    height: mainWin.height - Kirigami.Units.gridUnit * 2

    standardButtons: QQC2.DialogButtonBox.Cancel | QQC2.DialogButtonBox.Apply

    PdfsOrganizer {
        id: pdfOrg
    }

    // place this information atop of footer - on the left. There is plenty of space
    QQC2.Label {
        visible: pdfOrg.totalPages
        parent: footer
        x: Kirigami.Units.gridUnit * 2
        anchors.verticalCenter: parent.verticalCenter
        font.bold: true
        text: i18n("Total pages") + ": " + pdfOrg.totalPages
    }

    ColumnLayout {
        Layout.margins: Kirigami.Units.gridUnit

        Kirigami.ActionToolBar {
            id: toolBar
            actions: [
                Kirigami.Action {
                    icon.name: "application-pdf"
                    text: i18n("add PDF files")
                    onTriggered: pdfOrg.addMorePDFs()
                },
                Kirigami.Action {
                    id: showPath
                    text: i18n("show PDF path")
                    checkable: true
                    checked: true
                }
            ]
        }

        Kirigami.CardsListView {
            id: fileView
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            spacing: Kirigami.Units.smallSpacing
            model: pdfOrg.fileModel

            property int dragTargetIndex: -1

            delegate: Kirigami.AbstractCard {
                required property int index
                required property string path
                required property string fileName
                required property int pageCount
                z: dragArea.pressed ? 5 : 1
                contentItem: Item {
                    implicitWidth: rowLay.implicitWidth
                    implicitHeight: rowLay.implicitHeight
                    Rectangle {
                        id: drawRect
                        width: parent.width; height: parent.height
                        color: dragArea.pressed ? PDFED.alpha(Kirigami.Theme.textColor, 50) : "transparent"
                        MouseArea {
                            id: dragArea
                            anchors.fill: parent
                            drag.target: drawRect
                            drag.axis: Drag.YAxis
                            // onPressed: {
                            // }
                            onPositionChanged: (mouse) => {
                                let pos = mapToItem(fileView.contentItem, mouse.x, mouse.y + height / 2)
                                let targetId = fileView.indexAt(pos.x, pos.y)
                                if (targetId !== index)
                                    fileView.dragTargetIndex = targetId
                            }
                            onReleased: (mouse) => {
                                let pos = mapToItem(fileView.contentItem, mouse.x, mouse.y + height / 2)
                                let targetId = fileView.indexAt(pos.x, pos.y)
                                if (targetId !== index)
                                    pdfOrg.fileModel.move(index, targetId)
                                drawRect.x = 0
                                drawRect.y = 0
                                fileView.dragTargetIndex = -1
                            }
                        }
                        RowLayout {
                            id: rowLay
                            anchors { left: parent.left; top: parent.top; right: parent.right }
                            QQC2.Label {
                                text: (index + 1)
                            }
                            Kirigami.Icon {
                                source: "application-pdf"
                                Layout.fillHeight: true
                                Layout.maximumHeight: Kirigami.Units.iconSizes.large
                                Layout.preferredWidth: height
                            }
                            QQC2.Label {
                                visible: showPath.checked
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
                                        onTriggered: pdfOrg.fileModel.remove(index)
                                    }
                                ]
                            }
                        } // RowLayout
                    } // Rectangle
                    Loader {
                        active: fileView.dragTargetIndex === index
                        sourceComponent: Rectangle {
                            y: -fileView.spacing * 2
                            width: drawRect.width; height: fileView.spacing * 3
                            color: Kirigami.Theme.highlightColor
                        }
                    }
                } // Item (contentItem)
            } // delegate
        }
    }

    onApplied: {
        pdfOrg.aplyNewFiles()
        close()
    }
    onClosed: destroy()
}
