// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: root

    property real pdfVersion: 0.0

    readonly property var versions: [ 0.0, 1.4, 1.5, 1.6, 1.7 ]

    title: i18n("Set PDF version")
    visible: true

    ListView {
        id: verView
        implicitWidth: Kirigami.Units.gridUnit * 16
        implicitHeight: contentHeight

        model: root.versions

        delegate: QQC2.RadioDelegate {
            required property int index
            topPadding: Kirigami.Units.smallSpacing * 2
            bottomPadding: Kirigami.Units.smallSpacing * 2
            implicitWidth: verView.width
            text: index === 0 ? i18nc("like default PDF version", "default") : root.versions[index]
            checked: root.pdfVersion === root.versions[index]
            onClicked: {
                root.pdfVersion = root.versions[index]
                root.accept()
            }
        }
    }
}
