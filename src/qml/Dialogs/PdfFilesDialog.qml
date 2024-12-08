// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQml.Models
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import QtQuick.Layouts
import org.kde.karp

FormCard.FormCardDialog {
    id: pdfsDialog

    property alias pdfEdit: pdfOrg.editModel
    property alias initFiles: pdfOrg.initFiles

    visible: true
    title: i18nc("@title:dialog", "Add and Arrange PDF files")
    width: Kirigami.ApplicationWindow.window.width - Kirigami.Units.gridUnit * 2
    height: Kirigami.ApplicationWindow.window.height - Kirigami.Units.gridUnit * 2

    standardButtons: QQC2.DialogButtonBox.Cancel | QQC2.DialogButtonBox.Apply

    // HACK: we should be using Binding here
    readonly property bool hasPdfs: fileView.count > 0
    onHasPdfsChanged: standardButton(QQC2.DialogButtonBox.Apply).enabled = hasPdfs
    Component.onCompleted: standardButton(QQC2.DialogButtonBox.Apply).enabled = hasPdfs

    PdfsOrganizer {
        id: pdfOrg
        onPasswordRequired: (fName, fId) => {
            let passDlg = Qt.createComponent("org.kde.karp", "PdfPassDialog").createObject(pdfsDialog, { fileName: fName, fileId: fId })
            passDlg.accepted.connect(() => pdfOrg.setPdfPassword(passDlg.fileId, passDlg.passKey))
            passDlg.rejected.connect(() => pdfOrg.setPdfPassword(passDlg.fileId, ""))
        }
    }

    // place this information atop of footer - on the left. There is plenty of space
    QQC2.Label {
        visible: pdfOrg.totalPages
        parent: footer
        x: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        anchors.verticalCenter: parent.verticalCenter
        font.bold: true
        text: i18nc("@info", "Total pages: %1", pdfOrg.totalPages)
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            spacing: Kirigami.Units.mediumSpacing

            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.leftMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing

            QQC2.ToolButton {
                icon.name: "list-add-symbolic"
                text: i18nc("@action:button", "Add PDF…")
                onClicked: pdfOrg.addMorePDFs()
            }

            QQC2.CheckBox {
                id: showPathAction
                text: i18nc("@option:check", "Show PDF File Path")
                checkable: true
                checked: false

                Layout.alignment: Qt.AlignRight
            }
        }

        Kirigami.CardsListView {
            id: fileView

            property alias pdfModel: visualModel.model

            Layout.fillHeight: true
            Layout.fillWidth: true

            clip: true
            spacing: Kirigami.Units.smallSpacing

            model: DelegateModel {
                id: visualModel

                model: pdfOrg.fileModel

                delegate: DropArea {
                    id: delegate

                    required property int index
                    required property string path
                    required property string fileName
                    required property int pageCount
                    required property bool locked
                    required property bool selectAll
                    property int visualIndex: DelegateModel.itemsIndex
                    property ListView lv: ListView.view

                    width: lv.width - Kirigami.Units.gridUnit * 2
                    height: pdfDelegate.height

                    PdfFileDelegate {
                        id: pdfDelegate

                        property int visualIndex: delegate.visualIndex

                        z: dragActive ? 5 : 1
                        width: parent.width
                        Drag.active: dragActive
                        Drag.source: pdfDelegate
                        Drag.hotSpot.y: height / 2

                        states: [
                            State {
                                when: pdfDelegate.dragActive
                                ParentChange {
                                    target: pdfDelegate
                                    parent: fileView.contentItem
                                }
                            }
                        ]
                    }
                    onEntered: function(drag) {
                        let from = (drag.source as PdfFileDelegate).visualIndex
                        let to = pdfDelegate.visualIndex
                        // previously loaded PDF-s are locked and we don't allow it move new files between them
                        // so replacing with index bigger than 0 is ignored
                        if (locked) {
                            if (to > 0 && to < lv.count - 1)
                                return
                        }
                        visualModel.items.move(from, to)
                        pdfOrg.fileModel.move(from, to)
                    }
                }
            }

            displaced: Transition {
                NumberAnimation {
                    property: "y"
                    easing.type: Easing.OutQuad
                }
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.largeSpacing * 4)
                visible: fileView.count === 0
                text: i18nc("@info:placeholder", "Add one or more PDF file to proceed")
                helpfulAction: Kirigami.Action {
                    icon.name: "list-add-symbolic"
                    text: i18nc("@action:button", "Add PDF...")
                    onTriggered: pdfOrg.addMorePDFs()
                }
            }
        }
    }

    onApplied: {
        pdfOrg.applyNewFiles()
        close()
    }
    onClosed: destroy()
}
