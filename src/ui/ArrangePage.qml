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
        visible: pdfDoc.status !== PdfDocument.Ready
        anchors.centerIn: parent
        text: i18n("Select PDF file")
        onClicked: PDFED.getPdfFile()
    }

    DeaFEdMultiPageView {
        visible: pdfDoc.status === PdfDocument.Ready
        width: parent.width * 0.9; height: parent.height
        clip: true
        document: PdfDocument {
            id: pdfDoc
            source: "file:/" + PDFED.path
        }
    }

}
