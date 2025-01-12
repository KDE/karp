// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.qmlmodels
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.karp

FormCard.FormCardDialog {
    id: metaDlg

    required property var pdfModel

    title: i18n("PDF properties")
    visible: true
    closePolicy: Kirigami.PromptDialog.NoAutoClose
    standardButtons: QQC2.DialogButtonBox.Close | QQC2.DialogButtonBox.Save

    property var mainWin: Kirigami.ApplicationWindow.window

    width: Math.min(Kirigami.Units.gridUnit * 40, mainWin.width * 0.9)
    height: Math.min(Kirigami.Units.gridUnit * 40, mainWin.height * 0.9 - Kirigami.Units.gridUnit * 2)

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
                    required property int index
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
            id: targetView
            visible: outChip.checked
            property var metaArr: pdfModel.getTargetMetaData()
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cacheBuffer: Kirigami.Units.gridUnit * 40
            model: metaArr
            delegate: DelegateChooser {
                DelegateChoice { index: 0; delegate: formFieldComp }
                DelegateChoice { index: 1; delegate: formFieldComp }
                DelegateChoice { index: 2; delegate: formFieldComp }
                DelegateChoice { index: 3; delegate: formFieldComp }
                DelegateChoice { index: 4; delegate: formFieldComp }
                DelegateChoice { index: 5; delegate: formFieldComp }
                DelegateChoice { index: 6; delegate: formDateComp }
                DelegateChoice { index: 7; delegate: formDateComp }
            }
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
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
                visible: modelData !== ""
                width: metaView.contentItem.width
                FormCard.FormSectionText {
                    id: sectionText
                    visible: parent.visible
                    text: pdfModel.getMetaDataKey(index)
                }
                FormCard.FormButtonDelegate {
                    visible: parent.visible
                    text: index < 6 ? modelData : Qt.formatDateTime(modelData, "yyyy-MM-dd hh:mm:ss") // TODO: use more localized format
                    onClicked: singleClickNotification()
                    onDoubleClicked: {
                        copyAnim.start()
                        if (index < 6)
                            targetView.itemAtIndex(index).text = modelData
                        else
                            targetView.itemAtIndex(index).dateTime = modelData
                    }
                }
                Component.onCompleted: {
                    copyAllButton.enabled |= visible
                }
            }
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
            onModelChanged: {
                if (!metaView.count) // skip check when no delegates instantiated yet
                    return
                copyAllButton.enabled = false
                for (var m = 0; m < metaView.count; ++m) {
                    // enable 'copy all button only when there is any metadata
                    copyAllButton.enabled |= metaView.itemAtIndex(m).visible
                }
            }
        }

        FormCard.FormCard {
            id: copyAllButton
            visible: metaView.visible
            enabled: false
            Layout.fillWidth: true
            FormCard.FormButtonDelegate {
                text: i18n("Copy all metadata keys to output file")
                icon.name: "edit-copy"
                onClicked: singleClickNotification()
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
    }

    SequentialAnimation {
        id: copyAnim
        loops: 4
        ScriptAction { script: outChip.visible = false }
        PauseAnimation { duration: Kirigami.Units.shortDuration }
        ScriptAction { script: outChip.visible = true }
        PauseAnimation { duration: Kirigami.Units.shortDuration }
    }

    function singleClickNotification(): void {
        hidePassiveNotification()
        showPassiveNotification(i18n("Double click to copy metadata into output PDF file."))
    }

    // At first make targetView visible to create all TextFileds,
    // then switch to first chip with first PDF metadata
    Component.onCompleted: {
        chipsRep.itemAt(0).checked = true
    }

    Component {
        id: formDateComp
        FormCard.FormCard {
            required property var modelData
            required property int index
            property alias dateTime: dtForm.value
            width: targetView.contentItem.width
            FormCard.FormSectionText {
                text: pdfModel.getMetaDataKey(index)
            }
            FormCard.FormDateTimeDelegate {
                id: dtForm
                value: modelData
            }
        }
    }
    Component {
        id: formFieldComp
        FormCard.FormCard {
            width: parent?.width
            required property string modelData
            required property int index
            property alias text: metaField.text
            FormCard.FormTextFieldDelegate {
                id: metaField
                label: pdfModel.getMetaDataKey(index)
                text: targetView.metaArr[index]
            }
        }
    }

    onAccepted: {
        for (let i = 0; i < targetView.count; ++i)
            if (i < 6)
                targetView.metaArr[i] = targetView.itemAtIndex(i).text
            else
                targetView.metaArr[i] = targetView.itemAtIndex(i).dateTime
        pdfModel.setTargetMetaData(targetView.metaArr)
    }
    onClosed: destroy()
}
