// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCard {
    id: root

    signal clicked()

    width: parent.width
    height: addButt.height + Kirigami.Units.gridUnit * 2
    anchors { centerIn: parent; margins: Kirigami.Units.gridUnit }
    FormCard.FormButtonDelegate {
        id: addButt
        icon.name: "application-pdf"
        text: i18n("Select one or more PDF files.")
        description: i18n(
"When more files is selected - order them first.\n\
Then arrange all pages: reorder, rotate, remove.\n\
At the end save new PDF with selected options."
        )
        onClicked: root.clicked()
    }
}
