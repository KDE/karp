// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
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

            property int dragTargetIndex: -1

            clip: true
            spacing: Kirigami.Units.smallSpacing
            model: pdfOrg.fileModel
            delegate: PdfFileDelegate {}

            Layout.fillHeight: true
            Layout.fillWidth: true

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
