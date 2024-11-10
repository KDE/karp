// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.deafed

FormCard.FormCardDialog {
    required property var pdfModel

    title: i18n("PDF properties")
    visible: true
    standardButtons: QQC2.DialogButtonBox.Close // TODO | QQC2.DialogButtonBox.Save

    width: Math.min(Kirigami.Units.gridUnit * 40, mainWin.width * 0.9)
    height: Math.min(Kirigami.Units.gridUnit * 30, mainWin.height * 0.9)

    padding: Kirigami.Units.largeSpacing

    QQC2.ButtonGroup { id: tabsGr }

    contentItem: ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Flow {
            id: tabBar
            visible: pdfModel?.pdfCount > 1
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            Repeater {
                model: pdfModel.pdfCount
                Kirigami.Chip {
                    QQC2.ButtonGroup.group: tabsGr
                    text: pdfModel.getPdfName(index).replace(".pdf", "")
                    checkable: true
                    closable: false
                    checked: index === 0
                    onClicked: metaView.model = pdfModel.getMetaDataModel(index)
                }
            }
        }
        Kirigami.CardsListView {
            id: metaView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: pdfModel.getMetaDataModel(0)
            delegate: FormCard.FormCard {
                width: parent.width
                required property string modelData
                property var metaData: modelData.split("|")
                FormCard.FormTextFieldDelegate {
                    label: metaData[0]
                    text: metaData[1]
                }
            }
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        }
    }

    onAccepted: console.log("Not yet implemented")
    onClosed: destroy()
}
