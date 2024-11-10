// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Column {
    id: root

    signal clicked()

    width: parent.width
    anchors { centerIn: parent; margins: Kirigami.Units.gridUnit }
    spacing: Kirigami.Units.largeSpacing

    QQC2.Button {
        anchors.horizontalCenter: parent.horizontalCenter
        icon.name: "list-add"
        onClicked: root.clicked()
    }
    QQC2.Label {
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - Kirigami.Units.gridUnit * 2, Kirigami.Units.gridUnit * 30)
        horizontalAlignment: QQC2.Label.AlignHCenter
        text: i18n("Select one or more PDF files.\n\
When more files is selected - order them first.\n\
Then arrange all pages: reorder, rotate, remove.\n\
At the end save new PDF with selected options.")
        wrapMode: QQC2.Label.WordWrap
    }
}
