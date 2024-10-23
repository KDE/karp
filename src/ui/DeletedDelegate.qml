// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczzuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Shapes

Shape {
    id: delDel

    property alias buttonVisible: revertButton.visible
    signal wantRevert()

    anchors.fill: parent
    ShapePath {
        strokeWidth: Math.min(delDel.width, delDel.height) * 0.02
        strokeColor: "red"
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        PathLine { x: delDel.width; y: delDel.height }
        PathMove { x: delDel.width; y: 0 }
        PathLine { x: 0; y: delDel.height }
    }

    QQC2.Button {
        id: revertButton
        anchors.centerIn: parent
        icon.name: "document-revert"
        text: i18n("Revert deletion")
        onClicked: delDel.wantRevert()
    }
}
