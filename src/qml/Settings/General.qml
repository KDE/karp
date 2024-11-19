// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.karp
import org.kde.karp.config

FormCard.FormCardPage {
    id: root

    // title: i18n("Karp Settings")

    FormCard.FormHeader {
        title: i18n("Open file dialog at")
    }
    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            text: i18n("Last opened directory")
            checked: KarpConf.openLastDir
            onToggled: KarpConf.openLastDir = checked
        }
        FormCard.FormRadioDelegate {
            id: staticPathRadio
            text: i18n("Fixed path")
            checked: !KarpConf.openLastDir
            onToggled: KarpConf.openLastDir = !checked
        }
        FormPathDelegate {
            id: fixDirButton
            enabled: staticPathRadio.checked
            pathType: FormPathDelegate.Folder
            path: KarpConf.fixedLastDir
            labelVisible: false
            onAccepted: KarpConf.fixedLastDir = path
        }
    }

    FormCard.FormHeader {
        title: i18n("Default name for output PDF")
    }
    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            text: i18n("Always ask for file name")
            checked: KarpConf.askForOutFile
            onToggled: KarpConf.askForOutFile = true
        }
        FormCard.FormRadioDelegate {
            id: nameXfixRadio
            text: i18n("Combine with input file name")
            checked: !KarpConf.askForOutFile
            onToggled: KarpConf.askForOutFile = false
        }
        FormCard.AbstractFormDelegate {
            contentItem: RowLayout {
                enabled: nameXfixRadio.checked
                QQC2.ComboBox {
                    id: appPrepCombo
                    property bool doPrepend: currentIndex === 0
                    model: [ i18n("Append"), i18n("Prepend") ]
                    currentIndex: KarpConf.appendXfix ? 0 : 1
                }
                QQC2.TextField {
                    id: fixText
                    Layout.fillWidth: true
                    text: KarpConf.outFileXfix
                    onEditingFinished: KarpConf.outFileXfix = text !== "" ? text : "-out"
                }
            }
        }
        FormCard.FormTextDelegate {
            enabled: nameXfixRadio.checked
            text: i18n("It will produce: <b>%1filename%2.pdf</b>", appPrepCombo.doPrepend ? "" : fixText.text, appPrepCombo.doPrepend ? fixText.text : "")
        }
    }
}
