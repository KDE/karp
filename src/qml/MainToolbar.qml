// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.ActionToolBar {
    required property PdfEditModel pdfModel
    property alias labelsVisible: labelsAction.checked

    visible: pdfModel.pageCount
    alignment: Qt.AlignHCenter

    actions: [
        Kirigami.Action {
            icon.name: "edit-delete"
            icon.color: "red"
            tooltip: i18n("Select pages to delete")
            onTriggered: {
                let pageNr = pdfView.currentColumn > -1 ? pdfView.currentRow * pdfView.columns + pdfView.currentColumn + 1: 1
                delDlgComp.createObject(null, { range: APP.range(pageNr, pageNr) })
            }
        },
        Kirigami.Action {
            id: labelsAction
            icon.name: "label"
            tooltip: i18n("Show page labels")
            checkable: true
            checked: true
        },
        Kirigami.Action {
            icon.name: "zoom-out"
            tooltip: i18n("Zoom Out")
            onTriggered: pdfModel.zoomOut()
            enabled: pdfModel.maxPageWidth > Kirigami.Units.gridUnit * 7
        },
        Kirigami.Action {
            icon.name: "zoom-in"
            tooltip: i18n("Zoom In")
            onTriggered: pdfModel.zoomIn()
            enabled: pdfView.columns > 1
        },
        Kirigami.Action {
            displayComponent: QQC2.SpinBox {
                id: pageSpin
                from: 1; to: pdfModel.pageCount
                textFromValue: function(value) {
                    return i18n("Page %1 of %2", value, to)
                }
                onValueModified: pdfView.positionViewAtRow((value - 1) / pdfView.columns, TableView.AlignTop)
                Binding {
                    pageSpin.value: pdfView.cellAtPosition(10, pdfView.contentY + 10, true).y * pdfView.columns + 1
                    delayed: true
                }
            }
        }
    ]

    Component {
        id: delDlgComp
        SelectPagesDialog {
            visible: true
            title: i18n("Delete selected pages")
            pageCount: pdfModel.pageCount
            onAccepted: pdfModel.deletePages(range)
        }
    }
}
