// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.Dialog {
    id: menuDialog
    title: i18n("PDF properties")
    visible: true
    standardButtons: QQC2.DialogButtonBox.NoButton //Save

    width: Kirigami.Units.gridUnit * 35
    height: Kirigami.Units.gridUnit * 25

    Kirigami.FormLayout {
        x: parent.width * 0.05; y: parent.height * 0.05
        width: parent.width * 0.9; height: parent.height * 0.9

        Repeater {
            model: pdfModel.metaDataModel()
            QQC2.TextField {
                property var metaData: modelData.split("|")
                Kirigami.FormData.label: metaData[0]
                text: metaData[1]
            }
        }
    }
}
