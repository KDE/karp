// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.deafed

FormCard.FormCardDialog {
    id: metaDlg

    required property var pdfModel

    title: i18n("PDF properties")
    visible: true
    standardButtons: QQC2.DialogButtonBox.Close | QQC2.DialogButtonBox.Save

    width: Math.min(Kirigami.Units.gridUnit * 40, mainWin.width * 0.9)
    height: Math.min(Kirigami.Units.gridUnit * 40, mainWin.height * 0.9)

    padding: Kirigami.Units.largeSpacing

    QQC2.ButtonGroup { id: tabsGr }

    contentItem: ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Flow {
            id: tabBar
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            Repeater {
                id: chipsRep
                model: pdfModel.pdfCount
                Kirigami.Chip {
                    QQC2.ButtonGroup.group: tabsGr
                    text: pdfModel.getPdfName(index).replace(".pdf", "")
                    checkable: true
                    closable: false
                    onClicked: metaView.model = pdfModel.getMetaDataModel(index)
                }
            }
            Kirigami.Chip {
                id: outChip
                QQC2.ButtonGroup.group: tabsGr
                text: i18n("Output PDF")
                checkable: true
                checked: true
                closable: false
            }
        }

        Kirigami.CardsListView {
            id: metaView
            visible: !outChip.checked
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: pdfModel.getMetaDataModel(0)
            delegate: FormCard.FormCard {
                required property string modelData
                required property int index
                property var metaData: modelData.split("|")
                visible: metaData[1] !== ""
                width: metaView.contentItem.width
                FormCard.FormSectionText {
                    visible: parent.visible
                    text: metaData[0]
                }
                FormCard.FormButtonDelegate {
                    visible: parent.visible
                    text: metaData[1]
                    onDoubleClicked: {
                        copyAnim.start()
                        targetView.itemAtIndex(index).text = text
                    }
                }
            }
            footer: FormCard.FormCard {
                width: metaView.contentItem.width
                FormCard.FormButtonDelegate {
                    text: i18n("Copy all metadata keys")
                    icon.name: "edit-copy"
                    onDoubleClicked: {
                        copyAnim.start()
                        for (var m = 0; m < metaView.count; ++m) {
                            let it = metaView.itemAtIndex(m)
                            if (it.visible)
                                targetView.itemAtIndex(m).text = it.metaData[1]
                        }
                    }
                }
            }
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        }
        Kirigami.CardsListView {
            id: targetView
            visible: outChip.checked
            property var metaArr: pdfModel.getTargetMetaData()
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cacheBuffer: Kirigami.Units.gridUnit * 40
            model: pdfModel.getMetaDataModel(0)
            delegate: FormCard.FormCard {
                width: parent?.width
                required property string modelData
                required property int index
                property alias text: metaField.text
                FormCard.FormTextFieldDelegate {
                    id: metaField
                    label: modelData.split("|")[0]
                    text: targetView.metaArr[index]
                }
            }
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        }

        Kirigami.InlineMessage {
            visible: true
            text: i18n("Double click to copy metadata into output PDF file.")
            icon.source: "edit-copy"
            Layout.fillWidth: true
            showCloseButton: true
        }
    }

    SequentialAnimation {
        id: copyAnim
        loops: 4
        ScriptAction { script: outChip.visible = false }
        PauseAnimation { duration: Kirigami.Units.shortDuration }
        ScriptAction { script: outChip.visible = true }
        PauseAnimation { duration: Kirigami.Units.shortDuration }
    }

    // At first make targetView visible to create all TextFileds,
    // then switch to first chip with first PDF metadata
    Component.onCompleted: {
        chipsRep.itemAt(0).checked = true
    }

    onAccepted: {
        for (let i = 0; i < targetView.count; ++i)
            targetView.metaArr[i] = targetView.itemAtIndex(i).text
        pdfModel.setTargetMetaData(targetView.metaArr)
    }
    onClosed: destroy()
}
