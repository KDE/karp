// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.Page {
    id: page

    title: PDFED.name

    property alias pdfModel: pdfModel

    actions: [
        Kirigami.Action {
            visible: pdfModel.pageCount
            enabled: pdfModel.edited
            icon.name: "application-pdf"
            text: i18n("Generate")
            onTriggered: pdfModel.generate()
        },
        Kirigami.Action {
            // visible: pdfModel.pageCount
            icon.name: "settings-configure"
            text: i18n("PDF Options")
            Kirigami.Action {
                icon.name: "image-x-generic"
                text: i18n("Optimize images")
                checkable: true
                checked: pdfModel.optimizeImages
                onTriggered: pdfModel.optimizeImages = checked
            }
            Kirigami.Action {
                icon.name: "application-x-compressed-tar"
                text: i18n("Reduce size")
                checkable: true
                checked: pdfModel.reduceSize
                onTriggered: pdfModel.reduceSize = checked
            }
            Kirigami.Action {
                icon.name: "viewpdf"
                text: i18n("PDF properties")
                onTriggered: Qt.createComponent("qrc:/qt/qml/org/kde/deafed/ui/PdfMetadataDialog.qml")
                                                .createObject(page, { metaDataModel: pdfModel.metaDataModel() })
            }
        }
    ]

    PdfEditModel {
        id: pdfModel
        viewWidth: pdfView.width
        spacing: pdfView.columnSpacing
    }

    QQC2.Button {
        visible: !pdfModel.pageCount
        anchors.centerIn: parent
        text: i18n("Select PDF file")
        icon.name: "application-pdf"
        onClicked: pdfModel.loadPdfFile(PDFED.getPdfFile())
    }

    Rectangle {
        visible: pdfModel.pageCount
        anchors.fill: pdfView
        color: Kirigami.Theme.alternateBackgroundColor
    }

    TableView {
        id: pdfView
        visible: pdfModel.pageCount
        width: page.width - Kirigami.Units.largeSpacing * 4
        height: page.height - Kirigami.Units.largeSpacing * 3 - bottomBar.height
        clip: true
        columnSpacing: Kirigami.Units.smallSpacing
        rowSpacing: Kirigami.Units.smallSpacing
        model: pdfModel
        editTriggers: TableView.SingleTapped | TableView.AnyKeyPressed

        property int dragTargetPage: -1

        selectionModel: ItemSelectionModel { id: selModel }

        delegate: Rectangle {
            id: delegRect
            visible: pageNr < pdfModel.pageCount
            // required property bool selected // TODO - multiple selection
            required property bool current
            implicitWidth: pdfModel.maxPageWidth; implicitHeight: pdfModel.maxPageWidth * pageRatio
            color: "transparent"
            border {
                width: current ? 5 : 0
                color: Kirigami.Theme.highlightColor
            }
            PdfPageItem {
                id: img
                property bool enableAnimation: false
                z: -1
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                image: pageImg
                scale: parent.width / (rotated === 90 || rotated === 270 ? height : width)
                Behavior on x { enabled: img.enableAnimation; NumberAnimation {} }
                Behavior on y { enabled: img.enableAnimation; NumberAnimation {} }
                rotation: rotated
            }
            Loader {
                active: labelsAction.checked
                anchors { bottom: parent.bottom; right: parent.right; margins: 2 }
                height: Kirigami.Units.gridUnit * 2
                width: height * 3
                sourceComponent: Rectangle {
                    anchors.fill: parent
                    color: "#80000000"
                    Text {
                        width: parent.width * 0.9; height: parent.height
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        color: "#fff"
                        fontSizeMode: Text.HorizontalFit
                        minimumPixelSize: 6
                        font { pixelSize: parent.height * 0.8; bold: true }
                        text: (pageNr + 1) + " <font size=\"1\">(" + (origPage + 1) + ")</font>"
                    }
                }
            }
            TableView.editDelegate: EditDelegate {}
            Loader {
                active: deleted
                anchors.fill: parent
                sourceComponent: DeletedDelegate {
                    buttonVisible: current
                    onWantRevert: pdfModel.addDeletion(pageNr, false)
                }
            }
            Loader {
                active: pdfView.dragTargetPage === pageNr
                z: 5
                sourceComponent: Rectangle {
                    x: delegRect.width * 0.9 + Kirigami.Units.smallSpacing
                    y: delegRect.height * 0.05
                    width: delegRect.width * 0.1; height: delegRect.height * 0.9
                    color: Kirigami.Theme.highlightColor
                }
            }
        } // delegate
        QQC2.ScrollBar.vertical: QQC2.ScrollBar { visible: true }
    } // ListView

    footer: Kirigami.ActionToolBar {
        id: bottomBar
        visible: pdfModel.pageCount
        alignment: Qt.AlignHCenter

        actions: [
            Kirigami.Action {
                id: labelsAction
                icon.name: "label"
                tooltip: i18n("show page labels")
                checkable: true
                checked: true
            },
            Kirigami.Action {
                icon.name: "zoom-out"
                tooltip: i18n("Zoom Out")
                // shortcutsName: "ZoomOut"
                onTriggered: pdfModel.zoomOut()
                enabled: pdfModel.maxPageWidth > Kirigami.Units.gridUnit * 7
            },
            Kirigami.Action {
                icon.name: "zoom-in"
                tooltip: i18n("Zoom In")
                // shortcutsName: "ZoomIn"
                onTriggered: pdfModel.zoomIn()
                enabled: pdfView.columns > 1
            },
            Kirigami.Action {
                displayComponent: QQC2.Label {
                    text: i18n("go to page") + " "
                }
            },
            Kirigami.Action {
                displayComponent: QQC2.SpinBox {
                    id: pageSpin
                    from: 1; to: pdfModel.pageCount
                    onValueModified: pdfView.positionViewAtRow((value - 1) / pdfView.columns, TableView.AlignTop)
                    Binding {
                        pageSpin.value: pdfView.cellAtPosition(10, pdfView.contentY + 10, true).y * pdfView.columns + 1
                        delayed: true
                    }
                }
            }
        ]
    }

    function movePage(from, to) {
        var pageNr = pdfModel.addMove(from, to)
        if (pageNr > -1) {
            selModel.setCurrentIndex(pdfView.index(pageNr / pdfView.columns, pageNr % pdfView.columns), ItemSelectionModel.Current)
            pdfView.edit(selModel.currentIndex)
        }
    }

    Component.onCompleted: {
        if (PDFED.path !== "")
            pdfModel.loadPdfFile(PDFED.path)
    }
}
