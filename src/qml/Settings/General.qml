// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.deafed
import org.kde.deafed.config

FormCard.FormCardPage {
    id: root

    title: i18n("Deaf Ed Settings")

    FormCard.FormHeader {
        title: i18n("Open file dialog at")
    }
    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            text: i18n("Last opened directory")
            checked: DeafEdConf.openLastDir
            onToggled: DeafEdConf.openLastDir = checked
        }
        FormCard.FormRadioDelegate {
            id: staticPathRadio
            text: i18n("Fixed path")
            checked: !DeafEdConf.openLastDir
            onToggled: DeafEdConf.openLastDir = !checked
        }
        FormPathDelegate {
            id: fixDirButton
            enabled: staticPathRadio.checked
            pathType: FormPathDelegate.Folder
            path: DeafEdConf.fixedLastDir
            labelVisible: false
            onAccepted: DeafEdConf.fixedLastDir = path
        }
    }

    FormCard.FormHeader {
        title: i18n("Default name for output PDF")
    }
    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            text: i18n("Always ask for file name")
            checked: DeafEdConf.askForOutFile
            onToggled: {
                DeafEdConf.askForOutFile = true
                console.log(DeafEdConf.askForOutFile)
            }
        }
        FormCard.FormRadioDelegate {
            id: nameXfixRadio
            text: i18n("Combine with input file name")
            checked: !DeafEdConf.askForOutFile
            onToggled: {
                DeafEdConf.askForOutFile = false
                console.log(DeafEdConf.askForOutFile)
            }
        }
        FormCard.AbstractFormDelegate {
            contentItem: RowLayout {
                enabled: nameXfixRadio.checked
                QQC2.ComboBox {
                    id: appPrepCombo
                    property bool doPrepend: currentIndex === 0
                    model: [ i18n("Append"), i18n("Prepend") ]
                    currentIndex: DeafEdConf.appendXfix ? 0 : 1
                }
                QQC2.TextField {
                    id: fixText
                    Layout.fillWidth: true
                    text: DeafEdConf.outFileXfix
                    onEditingFinished: DeafEdConf.outFileXfix = text !== "" ? text : "-out"
                }
            }
        }
        FormCard.FormTextDelegate {
            enabled: nameXfixRadio.checked
            text: i18n("It will produce: <b>%1filename%2.pdf</b>", appPrepCombo.doPrepend ? "" : fixText.text, appPrepCombo.doPrepend ? fixText.text : "")
        }
    }
}
