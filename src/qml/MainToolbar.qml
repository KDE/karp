// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import QtQuick.Layouts
import org.kde.karp

Components.FloatingToolBar {
    id: root

    required property PdfEditModel pdfModel
    property alias labelsVisible: labelsAction.checked
    property alias multiSelect: selectAction.checked
    property alias showBookmarks: bookmarkAction.checked

    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.ToolButton {
            text: i18nc("@action:intoolbar", "Select pages to delete")
            display: QQC2.ToolButton.IconOnly
            icon {
                name: "edit-delete"
                color: "red"
            }

            onClicked: {
                let from = pdfModel.firstSelected ? pdfModel.firstSelected : 1
                let to = pdfModel.lastSelected ? pdfModel.lastSelected : 1
                delDlgComp.createObject(null, { range: APP.range(from, to) })
            }

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            icon.name: "object-rotate-right"
            text: i18nc("@action:intoolbar", "Select pages to rotate")
            display: QQC2.ToolButton.IconOnly
            onClicked: {
                let from = pdfModel.firstSelected ? pdfModel.firstSelected : 1
                let to = pdfModel.lastSelected ? pdfModel.lastSelected : 1
                rotDlgComp.createObject(null, { range: APP.range(from, to) })
            }

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            text: i18nc("@action:intoolbar", "Select pages to move")
            display: QQC2.ToolButton.IconOnly
            icon.name: "transform-move"
            onClicked: {
                let from = pdfModel.firstSelected ? pdfModel.firstSelected : 1
                let to = pdfModel.lastSelected ? pdfModel.lastSelected : 1
                mvDlgComp.createObject(null, { range: APP.range(from, to) })
            }

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            id: bookmarkAction
            text: i18nc("@action:intoolbar", "Table of Content (Bookmarks)")
            display: QQC2.ToolButton.IconOnly
            icon.name: "bookmark-toolbar"
            checkable: true
            onClicked: showBookmarks = checked // override default binding to prefer user want to see bookmark pane

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            id: selectAction

            text: i18nc("@action:intoolbar", "Multiple pages selection")
            display: QQC2.ToolButton.IconOnly
            icon.name: "view-pages-overview"
            icon.color: checked ? Kirigami.Theme.highlightColor : undefined
            checkable: true
            checked: APP.ctrlPressed

            onClicked: {
                if (checked) {
                    checked = true
                } else {
                    let currPage = pdfView.currentIndex > -1 ? pdfView.currentIndex : 0
                    pdfModel.selectPage(currPage, pdfView.currentIndex > -1, false)
                    checked = Qt.binding(() => APP.ctrlPressed)
                }
            }

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            id: labelsAction

            text: i18nc("@action:intoolbar", "Show page labels")
            display: QQC2.ToolButton.IconOnly
            icon.name: "label"
            checkable: true
            checked: true

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            text: i18nc("@action:intoolbar", "Zoom Out")
            icon.name: "zoom-out"
            display: QQC2.ToolButton.IconOnly
            onClicked: pdfModel.zoomOut()
            enabled: pdfModel.maxPageWidth > Kirigami.Units.gridUnit * 7

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            text: i18nc("@action:intoolbar", "Zoom In")
            icon.name: "zoom-in"
            onClicked: pdfModel.zoomIn()
            enabled: pdfModel.columns > 1
            display: QQC2.ToolButton.IconOnly

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.SpinBox {
            id: pageSpin
            property bool canIndexAtY: true
            from: 1; to: pdfModel.pageCount
            textFromValue: (value) => {
                return i18n("Page %1 of %2", value, to)
            }
            onValueModified: {
                canIndexAtY = false
                pdfView.positionViewAtIndex(value - 1, GridView.Center)
                canIndexAtY = true
            }
        }

        Component {
            id: delDlgComp
            SelectPagesDialog {
                visible: true
                title: i18n("Select pages to delete")
                acceptText: i18nc("@action:button", "Delete")
                pageCount: pdfModel.pageCount
                onAccepted: {
                    pdfModel.deletePages(range)
                    pdfView.currentIndex = -1
                }
            }
        }
        Component {
            id: rotDlgComp
            SelectPagesDialog {
                id: rotDlg
                visible: true
                property int angle: 90
                title: i18n("Select pages to rotate")
                acceptText: i18nc("@action:button", "Rotate")
                acceptIcon: "object-rotate-right"
                height: Kirigami.Units.gridUnit * 23
                pageCount: pdfModel.pageCount
                topItem: Kirigami.AbstractCard {
                    Layout.fillWidth: true
                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing
                        Item { height: 1; Layout.fillWidth: true }
                        QQC2.Label { text: i18n("Angle") }
                        QQC2.ComboBox {
                            id: angleCombo
                            model: [ "90°", "180°", "270°" ]
                            currentIndex: rotDlg.angle / 90 - 1
                            onActivated: (index) => { rotDlg.angle = (index + 1) * 90 }
                        }
                        Item { height: 1; Layout.fillWidth: true }
                    }
                }
                onAccepted: pdfModel.rotatePages(range, rotDlg.angle)
            }
        }
        Component {
            id: mvDlgComp
            MovePagesDialog {
                id: mvDlg
                visible: true
                title: i18n("Select pages to move")
                pageCount: pdfModel.pageCount
                onAccepted: pdfModel.movePages(range, targetPage)
            }
        }
    }

    Connections {
        target: pdfView
        function onContentYChanged() : void {
            if (pageSpin.canIndexAtY)
                pageSpin.value = pdfView.indexAt(10, pdfView.contentY + Math.min(pdfView.cellHeight, pdfView.height / 2)) + 1
        }
    }
}
