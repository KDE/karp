// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.qmlmodels
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

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

    QQC2.ButtonGroup {
        id: tabsGr
    }

    contentItem: ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Flow {
            id: tabBar
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            Repeater {
                id: chipsRep
                model: metaDlg.pdfModel.pdfCount
                Kirigami.Chip {
                    required property int index
                    QQC2.ButtonGroup.group: tabsGr
                    text: metaDlg.pdfModel.getPdfName(index).replace(".pdf", "")
                    checkable: true
                    closable: false
                    onClicked: metaView.model = metaDlg.pdfModel.getMetaDataModel(index)
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
            property var metaArr: metaDlg.pdfModel.getTargetMetaData()
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cacheBuffer: Kirigami.Units.gridUnit * 40
            model: metaArr
            delegate: DelegateChooser {
                DelegateChoice {
                    index: 0
                    FormFieldComp {}
                }
                DelegateChoice {
                    index: 1
                    FormFieldComp {}
                }
                DelegateChoice {
                    index: 2
                    FormFieldComp {}
                }
                DelegateChoice {
                    index: 3
                    FormFieldComp {}
                }
                DelegateChoice {
                    index: 4
                    FormFieldComp {}
                }
                DelegateChoice {
                    index: 5
                    FormFieldComp {}
                }
                DelegateChoice {
                    index: 6
                    FormDateComp {}
                }
                DelegateChoice {
                    index: 7
                    FormDateComp {}
                }
            }
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        }

        Kirigami.CardsListView {
            id: metaView
            visible: !outChip.checked
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: metaDlg.pdfModel.getMetaDataModel(0)
            delegate: FormCard.FormCard {
                id: metaDelegate
                required property var modelData
                required property int index
                visible: metaText.text.length > 0
                width: metaView.contentItem.width
                FormCard.FormSectionText {
                    id: sectionText
                    visible: parent.visible
                    text: metaDlg.pdfModel.getMetaDataKey(metaDelegate.index)
                }
                FormCard.FormButtonDelegate {
                    id: metaText
                    visible: parent.visible
                    text: metaDelegate.index < 6 ? metaDelegate.modelData : Qt.formatDateTime(metaDelegate.modelData, "yyyy-MM-dd hh:mm:ss") // TODO: use more localized format
                    onClicked: metaDlg.singleClickNotification()
                    onDoubleClicked: {
                        copyAnim.start();
                        if (metaDelegate.index < 6)
                            targetView.itemAtIndex(metaDelegate.index).text = metaDelegate.modelData;
                        else
                            targetView.itemAtIndex(metaDelegate.index).dateTime = metaDelegate.modelData;
                    }
                }
                Component.onCompleted: {
                    copyAllButton.enabled |= visible;
                }
            }
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
            onModelChanged: {
                if (!metaView.count) // skip check when no delegates instantiated yet
                    return;
                copyAllButton.enabled = false;
                for (var m = 0; m < metaView.count; ++m) {
                    // enable 'copy all button only when there is any metadata
                    copyAllButton.enabled |= metaView.itemAtIndex(m).visible;
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
                onClicked: metaDlg.singleClickNotification()
                onDoubleClicked: {
                    copyAnim.start();
                    for (var m = 0; m < metaView.count; ++m) {
                        let it = metaView.itemAtIndex(m);
                        if (it.visible) {
                            if (m < 6)
                                targetView.itemAtIndex(m).text = it.modelData;
                            else
                                targetView.itemAtIndex(m).dateTime = it.modelData;
                        }
                    }
                }
            }
        }
    }

    SequentialAnimation {
        id: copyAnim
        loops: 4
        ScriptAction {
            script: outChip.visible = false
        }
        PauseAnimation {
            duration: Kirigami.Units.shortDuration
        }
        ScriptAction {
            script: outChip.visible = true
        }
        PauseAnimation {
            duration: Kirigami.Units.shortDuration
        }
    }

    function singleClickNotification(): void {
        hidePassiveNotification();
        showPassiveNotification(i18n("Double click to copy metadata into output PDF file."));
    }

    // At first make targetView visible to create all TextFileds,
    // then switch to first chip with first PDF metadata
    Component.onCompleted: {
        chipsRep.itemAt(0).checked = true;
    }

    component FormDateComp: FormCard.FormCard {
        id: dateCard
        required property var modelData
        required property int index
        property alias dateTime: dtForm.value
        width: targetView.contentItem.width
        FormCard.FormSectionText {
            text: metaDlg.pdfModel.getMetaDataKey(dateCard.index)
        }
        FormCard.FormDateTimeDelegate {
            id: dtForm
            value: dateCard.modelData
        }
    }

    component FormFieldComp: FormCard.FormCard {
        id: fieldCard
        width: parent?.width
        required property var modelData
        required property int index
        property alias text: metaField.text
        FormCard.FormTextFieldDelegate {
            id: metaField
            label: metaDlg.pdfModel.getMetaDataKey(fieldCard.index)
            text: targetView.metaArr[fieldCard.index]
        }
    }

    onAccepted: {
        for (let i = 0; i < targetView.count; ++i)
            if (i < 6)
                targetView.metaArr[i] = (targetView.itemAtIndex(i) as FormFieldComp).text;
            else
                targetView.metaArr[i] = (targetView.itemAtIndex(i) as FormDateComp).dateTime;
        pdfModel.setTargetMetaData(targetView.metaArr);
    }
    onClosed: destroy()
}
