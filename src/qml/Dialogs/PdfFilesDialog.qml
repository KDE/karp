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
    width: Kirigami.ApplicationWindow.window.width - Kirigami.Units.gridUnit * 2
    height: Kirigami.ApplicationWindow.window.height - Kirigami.Units.gridUnit * 2
    // popupType: FormCard.FormCardDialog.Native

    standardButtons: QQC2.DialogButtonBox.Cancel | QQC2.DialogButtonBox.Apply

    PdfsOrganizer {
        id: pdfOrg
        onPasswordRequired: (fName, fId) => {
            let passDlg = Qt.createComponent("org.kde.deafed", "PdfPassDialog").createObject(pdfsDialog, { fileName: fName, fileId: fId })
            passDlg.accepted.connect(function(){ pdfOrg.setPdfPassword(passDlg.fileId, passDlg.passKey) })
            passDlg.rejected.connect(function(){ pdfOrg.setPdfPassword(passDlg.fileId, "") })
        }
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

    contentItem: ColumnLayout {
        Layout.margins: Kirigami.Units.gridUnit
        Layout.fillHeight: true
        Layout.fillWidth: true

        Kirigami.ActionToolBar {
            id: toolBar
            actions: [
                Kirigami.Action {
                    icon.name: "application-pdf"
                    text: i18n("add PDF files")
                    onTriggered: pdfOrg.addMorePDFs()
                },
                Kirigami.Action {
                    id: showPathAction
                    text: i18n("show PDF path")
                    checkable: true
                    checked: false
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

            delegate: PdfFileDelegate {}
        }
    }

    onApplied: {
        pdfOrg.aplyNewFiles()
        close()
    }
    onClosed: destroy()
}
