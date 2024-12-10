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

    PdfsOrganizer {
        id: pdfOrg
        onPasswordRequired: (fName, fId) => {
            let passDlg = Qt.createComponent("org.kde.karp", "PdfPassDialog").createObject(pdfsDialog, {
                fileName: fName,
                fileId: fId
            });
            passDlg.accepted.connect(() => pdfOrg.setPdfPassword(passDlg.fileId, passDlg.passKey));
            passDlg.rejected.connect(() => pdfOrg.setPdfPassword(passDlg.fileId, ""));
        }
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing
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
                    onEntered: function (drag) {
                        let from = (drag.source as PdfFileDelegate).visualIndex;
                        let to = pdfDelegate.visualIndex;
                        // Previously loaded PDF-s are locked and we don't allow to move new files between them.
                        // Item (new file) can be moved before or after already loaded PDF-s
                        if (locked) {
                            if (to > 0 && to < lv.count - 1)
                                return;
                        }
                        visualModel.items.move(from, to);
                        pdfOrg.fileModel.move(from, to);
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
                    text: i18nc("@action:button", "Add PDF")
                    onTriggered: pdfOrg.addMorePDFs()
                }
            }
        }
    }

    footer: QQC2.DialogButtonBox {
        // place this information atop of footer - on the left. There is plenty of space
        Kirigami.SelectableLabel {
            visible: pdfOrg.totalPages
            parent: footer
            x: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
            anchors.verticalCenter: parent.verticalCenter
            font.bold: true
            text: i18nc("@info", "Total pages: %1", pdfOrg.totalPages)
        }
        QQC2.Button {
            id: addPdfs
            icon.name: "list-add-symbolic"
            text: i18nc("@action:button", "Add PDF")
            visible: fileView.count !== 0
            QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.ActionRole
            onClicked: pdfOrg.addMorePDFs()
        }
        QQC2.Button {
            text: i18nc("@action:button", "Apply")
            QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.ApplyRole
            enabled: fileView.count !== 0
        }
        QQC2.Button {
            text: i18nc("@action:button", "Cancel")
            QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.RejectRole
        }
    }
    onApplied: {
        pdfOrg.applyNewFiles();
        close();
    }
}
