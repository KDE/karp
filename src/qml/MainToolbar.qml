// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.actioncollection as Kirigami
import org.kde.kirigamiaddons.components as Components
import QtQuick.Layouts
import org.kde.karp

Components.FloatingToolBar {
    id: root

    required property PdfEditModel pdfModel
    required property bool showBookmarks
    required property bool multiSelect
    required property bool showLabels

    signal booksmarksToggled(bool checked)
    signal labelsToggled(bool checked)

    component ActionToolButton: QQC2.ToolButton {
        display: QQC2.ToolButton.IconOnly

        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        ActionToolButton {
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "multi-delete"

                onTriggered: {
                    let from = root.pdfModel.firstSelected ? root.pdfModel.firstSelected : 1;
                    let to = root.pdfModel.lastSelected ? root.pdfModel.lastSelected : 1;
                    delDlgComp.createObject(null, {
                        range: APP.range(from, to)
                    });
                }
            }
        }

        ActionToolButton {
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "multi-rotate"

                onTriggered: {
                    let from = root.pdfModel.firstSelected ? root.pdfModel.firstSelected : 1;
                    let to = root.pdfModel.lastSelected ? root.pdfModel.lastSelected : 1;
                    rotDlgComp.createObject(null, {
                        range: APP.range(from, to)
                    });
                }
            }
        }

        ActionToolButton {
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "multi-move"

                onTriggered: {
                    let from = root.pdfModel.firstSelected ? root.pdfModel.firstSelected : 1;
                    let to = root.pdfModel.lastSelected ? root.pdfModel.lastSelected : 1;
                    mvDlgComp.createObject(null, {
                        range: APP.range(from, to)
                    });
                }
            }
        }

        ActionToolButton {
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "toggle-bookmarks-pane"

                checkable: true
                checked: root.showBookmarks

                onToggled: root.booksmarksToggled(checked)
            }
        }

        ActionToolButton {
            id: selectAction
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "toggle-multi-select"

                checkable: true
                checked: root.multiSelect

                onToggled: {
                    if (checked) {
                        checked = true;
                    } else {
                        let currPage = pdfView.currentIndex > -1 ? pdfView.currentIndex : 0;
                        root.pdfModel.selectPage(currPage, pdfView.currentIndex > -1, false);
                        checked = Qt.binding(() => APP.ctrlPressed);
                    }
                }
            }
        }

        ActionToolButton {
            id: labelsAction
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "toggle-page-labels"

                checkable: true
                checked: root.showLabels

                onTriggered:  root.labelsToggled(checked);
                
            }
        }

        ActionToolButton {
            enabled: root.pdfModel.columns > 1
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "zoom-in"

                onTriggered: root.pdfModel.zoomIn()
            }
        }

        ActionToolButton {
            enabled: root.pdfModel.maxPageWidth > Kirigami.Units.gridUnit * 7
            action: Kirigami.Action {
                Kirigami.ActionCollection.collection: "org.kde.karp.actions"
                Kirigami.ActionCollection.action: "zoom-out"

                onTriggered: root.pdfModel.zoomOut()
            }
        }

        QQC2.SpinBox {
            id: pageSpin
            property bool canIndexAtY: true
            from: 1
            to: root.pdfModel.pageCount
            textFromValue: value => {
                return i18n("Page %1 of %2", value, to);
            }
            onValueModified: {
                canIndexAtY = false;
                pdfView.positionViewAtIndex(value - 1, GridView.Center);
                canIndexAtY = true;
            }
        }

        Component {
            id: delDlgComp
            SelectPagesDialog {
                visible: true
                title: i18n("Select pages to delete")
                acceptText: i18nc("@action:button", "Delete")
                pageCount: root.pdfModel.pageCount
                onAccepted: {
                    root.pdfModel.deletePages(range);
                    pdfView.currentIndex = -1;
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
                pageCount: root.pdfModel.pageCount
                topItem: Kirigami.AbstractCard {
                    Layout.fillWidth: true
                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing
                        Item {
                            Layout.preferredHeight: 1
                            Layout.fillWidth: true
                        }
                        QQC2.Label {
                            text: i18n("Angle")
                        }
                        QQC2.ComboBox {
                            id: angleCombo
                            model: ["90°", "180°", "270°"]
                            currentIndex: rotDlg.angle / 90 - 1
                            onActivated: index => {
                                rotDlg.angle = (index + 1) * 90;
                            }
                        }
                        Item {
                            Layout.preferredHeight: 1
                            Layout.fillWidth: true
                        }
                    }
                }
                onAccepted: root.pdfModel.rotatePages(range, rotDlg.angle)
            }
        }
        Component {
            id: mvDlgComp
            MovePagesDialog {
                id: mvDlg
                visible: true
                title: i18n("Select pages to move")
                pageCount: root.pdfModel.pageCount
                onAccepted: root.pdfModel.movePages(range, targetPage)
            }
        }
    }

    Connections {
        target: pdfView
        function onContentYChanged(): void {
            if (pageSpin.canIndexAtY)
                pageSpin.value = pdfView.indexAt(10, pdfView.contentY + Math.min(pdfView.cellHeight, pdfView.height / 2)) + 1;
        }
    }
}
