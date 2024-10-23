// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczzuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.deafed

FormCard.FormCardDialog {
    property alias metaDataModel: fieldRepeater.model

    title: i18n("PDF properties")
    visible: true
    standardButtons: QQC2.DialogButtonBox.Close // TODO | QQC2.DialogButtonBox.Save

    width: Math.min(Kirigami.Units.gridUnit * 40, mainWin.width * 0.9)
    height: Math.min(Kirigami.Units.gridUnit * 30, mainWin.height * 0.9)


    Flickable {
        x: parent.width * 0.05; y: parent.height * 0.05
        width: parent.width * 0.9
        height: parent.height * 0.9 - Kirigami.Units.gridUnit * 2
        contentWidth: width; contentHeight: metaCard.height
        clip: true
        FormCard.FormCard {
            id: metaCard
            width: parent.width
            Repeater {
                id: fieldRepeater
                FormCard.FormCard {
                    required property string modelData
                    property var metaData: modelData.split("|")
                    FormCard.FormTextFieldDelegate {
                        label: metaData[0]
                        text: metaData[1]
                    }
                }
            }
        }
    }

    onAccepted: console.log("Not yet implemented")
    onClosed: destroy()
}
