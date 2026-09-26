// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 by Darshan Phaldesai <dev.darshanphaldesai@gmail.com>

import QtQuick
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.karp
import org.kde.karp.config

FormCard.FormCardDialog {
    id: exportDialog
    title: i18nc("@title:dialog", "Export Options")
    standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Cancel
    closePolicy: Kirigami.PromptDialog.NoAutoClose

    required property PdfEditModel pdfModel

    //TODO: add descriptions

    FileDialog {
        id: saveDialog
        title: i18nc("@title:window", "Save PDF file")
        fileMode: FileDialog.SaveFile
        currentFolder: APP.getOpenDirUrl()
        nameFilters: [i18n("PDF files (*.pdf)")]
        defaultSuffix: "pdf"
        onAccepted: {
            startExport(APP.urlToPath(selectedFile))
        }
    }

    function startExport(outPath: string): void {
        Qt.createComponent("org.kde.karp", "ProgressDialog").createObject(exportDialog, {
            pdfModel: pdfModel
        });
        pdfModel.generate(outPath);
    }

    FormCard.FormSwitchDelegate {
        id: optimizeSwitch
        enabled: false //TODO make it work again
        text: i18nc("@action:inmenu", "Optimize PDF")
    }

    FormCard.FormSwitchDelegate {
        id: sizeSwitch
        text: i18nc("@action:inmenu", "Reduce Size")
    }

    FormCard.FormComboBoxDelegate {
        id: versionComboBox
        text: i18nc("@action:inmenu", "PDF Version")
        model: metadataModel
        currentIndex: 0
        textRole: "text"
        valueRole: "version"
    }

    FormCard.FormPasswordFieldDelegate {
        id: passwordField
        label: i18nc("@action:inmenu", "Password")
        placeholderText: i18nc("@action:inmenu password placeholder", "No Password")
    }

    FormCard.FormButtonDelegate {
        text: i18nc("@action:inmenu", "Edit PDF Metadata")
        icon.name: "document-properties"
        onClicked: {
            Qt.createComponent("org.kde.karp", "PdfMetadataDialog").createObject(this, {
                pdfModel
            });
        }
    }

    onAccepted: {
        pdfModel.passKey = passwordField.text;
        //pdfModel.optimizeImages = optimizeSwitch.checked; TODO
        pdfModel.reduceSize = sizeSwitch.checked;
        pdfModel.pdfVersion = versionComboBox.currentValue;

        if (KarpConf.askForOutFile) {
            saveDialog.open();
        } else {
            startExport("");
        }
    }

    ListModel {
        id: metadataModel
        //TODO: make them translateable
        ListElement {
            text: "Default"
            version: 0.0
        }
        ListElement {
            text: "1.4"
            version: 1.4
        }
        ListElement {
            text: "1.5"
            version: 1.5
        }
        ListElement {
            text: "1.6"
            version: 1.6
        }
        ListElement {
            text: "1.7"
            version: 1.7
        }
    }
}
