// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import QtQuick.Layouts
import org.kde.karp

Kirigami.Dialog {
    id: dialog

    required property PdfEditModel pdfModel

    title: i18n("Generating PDF file") + "..."
    padding: Kirigami.Units.largeSpacing
    showCloseButton: false
    standardButtons: Kirigami.Dialog.Cancel
    flatFooterButtons: false
    visible: true
    modal: true
    closePolicy: Kirigami.Dialog.NoAutoClose

    ColumnLayout {
        Item {
            id: progWrap
            width: messRect.width; height: messRect.height
            visible: !messRect.visible
            QQC2.ProgressBar {
                id: progBar
                anchors.centerIn: parent
                width: progWrap.width - Kirigami.Units.gridUnit
                from: 0.0
                to: 1.0
                value: pdfModel.progress
                indeterminate: false
            }
        }
        Rectangle {
            id: messRect
            color: "#20ff0000"
            width: childrenRect.width; height: childrenRect.height
            radius: Kirigami.Units.smallSpacing
            visible: false
            Text {
                width: Kirigami.Units.gridUnit * 23
                padding: Kirigami.Units.smallSpacing
                wrapMode: Text.WordWrap
                color: Kirigami.Theme.textColor
                horizontalAlignment: Text.AlignHCenter
                text: i18n("Reducing PDF file size did not produce expected results!")
            }
        }
    }

    Connections {
        target: pdfModel
        function onPdfGenerated() : void {
            if (!messRect.visible)
                dialog.close()
        }
        function onReductionNotWorked() : void {
            messRect.visible = true
            dialog.standardButtons = Kirigami.Dialog.Ok
        }
    }

    onRejected: pdfModel.cancel()

    onClosed: destroy()
}
