// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import "../src/ui/Settings/"

/**
 * Test for FormPathDelegate
 */
Kirigami.ApplicationWindow {
    id: mainWin

    title: "FormFilePathDelegate Test"

    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 20

    pageStack.initialPage: FormCard.FormCardPage {
        FormCard.FormHeader {
            title: "Open file dialog at"
        }
        FormCard.FormCard {
            FormCard.FormRadioDelegate {
                text: "Last opened directory"
            }
            FormCard.FormRadioDelegate {
                id: staticPathRadio
                text: "Fixed path"
                checked: true
            }
            FormPathDelegate {
                id: fixDirButton
                enabled: staticPathRadio.checked
                pathType: FormPathDelegate.Folder
                labelVisible: false
            }
        }
        FormCard.FormHeader { title: "" }
        FormCard.FormCard {
            FormPathDelegate {
                // nameFilters: [ "*.png" ]
                icon.source: "path-reverse"
                label: "Label on the right path:"
                path: "/dev/null"
            }
            FormCard.FormDelegateSeparator {}

            FormPathDelegate {
                labelVisible: false
                placeholderText: "Label invisible"
            }
            FormCard.FormDelegateSeparator {}

            FormPathDelegate {
                placeholderText: "Label is empty"
            }
        }
    }
}
