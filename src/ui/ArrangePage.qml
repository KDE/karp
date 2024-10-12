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
        onClicked: {
            PDFED.getPdfFile()
            PDFED.pdfModel.maxPageWidth = Qt.binding(function() { return page.width / 3 })
        }
    }

    ListView {
        id: pdfView
        visible: PDFED.pdfLoaded
        width: page.width / 2; height: page.height * 0.8
        clip: true
        spacing: height * 0.1
        model: PDFED.pdfModel

        delegate: PdfPageItem {
          x: 2; y: 2
          image: pageImg
          Rectangle {
              anchors { bottom: parent.bottom; right: parent.right }
              height: Kirigami.Units.gridUnit * 1.5
              width: height * 4
              color: PDFED.alpha(Kirigami.Theme.textColor, 150)
              Text {
                  anchors.fill: parent
                  horizontalAlignment: Text.AlignRight
                  verticalAlignment: Text.AlignVCenter
                  color: Kirigami.Theme.backgroundColor
                  font { pixelSize: parent.height * 0.8; bold: true }
                  text: index + 1
              }
          }
        }
    }

}
