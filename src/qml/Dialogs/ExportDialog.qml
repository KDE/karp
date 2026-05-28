// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 by Darshan Phaldesai <dev.darshanphaldesai@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardDialog {
    visible: true
    title: i18nc("@title:dialog", "Export Options")
    standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Cancel
    closePolicy: Kirigami.PromptDialog.NoAutoClose

    required property var pdfModel

    onClosed: destroy()

    //TODO: add descriptions
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
    }

    FormCard.FormButtonDelegate {
        text: i18nc("@action:inmenu", "Edit PDF Metadata")
        icon.name: "document-properties"
        onClicked: {
            Qt.createComponent("org.kde.karp", "PdfMetadataDialog").createObject(this, {
                pdfModel: pdfModel
            });
        }
    }

    onAccepted: {
        pdfModel.passKey = passwordField.text;
        //pdfModel.optimizeImages = optimizeSwitch.checked; TODO
        pdfModel.reduceSize = sizeSwitch.checked;
        pdfModel.pdfVersion = versionComboBox.currentValue;

        Qt.createComponent("org.kde.karp", "ProgressDialog").createObject(this, {
            pdfModel: pdfModel
        });
        pdfModel.generate();
    }

    ListModel {
        id: metadataModel
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
