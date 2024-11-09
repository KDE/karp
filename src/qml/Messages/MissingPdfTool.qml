// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    visible: true
    position: Kirigami.InlineMessage.Position.Footer
    type: Kirigami.MessageType.Error
    width: parent.width
    y: parent.height - height - Kirigami.Units.gridUnit
    showCloseButton: true
    // actions: [
    //     Kirigami.Action {
    //         text: qsTr("PDF Tools")
    //         icon.name: "application-pdf"
    //         // onTriggered:
    //     }
    // ]
    onVisibleChanged: {
        if (!visible)
            destroy()
    }
}
