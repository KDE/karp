// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Pdf
import org.kde.kirigami as Kirigami
import org.kde.deafed

Kirigami.Page {
    id: page

    title: i18n("Arrange PDF Pages") + ": " + PDFED.name

    QQC2.Button {
        visible: !PDFED.pdfLoaded
        anchors.centerIn: parent
        text: i18n("Select PDF file")
        onClicked: PDFED.getPdfFile()
    }

    ListView {
        id: pdfView
        visible: PDFED.pdfLoaded
        width: page.width / 2; height: page.height * 0.8
        clip: true
        spacing: height * 0.1
        model: PDFED.pdfModel

        delegate: Rectangle {
            // clip: true
            width: pdfView.width; height: pdfView.height * 0.4
            color: "transparent"
            border { width: 2; color: "red" }
            PdfPageItem {
                x: 1; y: 1
                image: pageImg
            }
        }
    }

}
