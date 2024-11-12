// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import QtQuick.Layouts

Kirigami.PromptDialog {
    id: passDialog

    required property string fileName
    required property int fileId
    property alias passKey: passField.text
    property alias passLabel: passField.label

    visible: true
    title: i18n("Password required")
    standardButtons: QQC2.DialogButtonBox.Cancel | QQC2.DialogButtonBox.Ok
    closePolicy: Kirigami.PromptDialog.NoAutoClose
    // popupType: Kirigami.PromptDialog.Native

    // when dialog is too narrow it looks ugly
    width: Math.min(Kirigami.Units.gridUnit * 30, Kirigami.ApplicationWindow.window.width - Kirigami.Units.gridUnit * 2)

    ColumnLayout {
        FormCard.FormCard {
            FormCard.FormPasswordFieldDelegate {
                id: passField
                label: i18n("Provide password to open file %1", "\n" + fileName)
                onAccepted: passDialog.accept()
            }
        }
    }

    Component.onCompleted: passField.forceActiveFocus()

    onClosed: destroy()
}
