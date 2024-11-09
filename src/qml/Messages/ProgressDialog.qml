// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.deafed

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

    QQC2.ProgressBar {
        id: progBar
        width: parent.width - Kirigami.Units.gridUnit
        from: 0.0
        to: 1.0
        value: pdfModel.progress
        indeterminate: false
    }

    Connections {
        target: pdfModel
        function onPdfGenerated() {
            dialog.close()
        }
    }

    onRejected: {
        console.log("Cancel generating")
        pdfModel.cancel()
    }

    onClosed: destroy()
}
