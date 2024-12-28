// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import QtQuick.Layouts
import org.kde.karp

ColumnLayout {
    id: bookPane

    property alias model: treeView.model
    property alias rows: treeView.rows

    Kirigami.SearchField {
        visible: !globalDrawer.collapsed
        Layout.fillWidth: true
    }

    TreeView {
        id: treeView

        Layout.fillWidth: true
        Layout.fillHeight: true

        clip: true

        delegate: QQC2.TreeViewDelegate {
            id: treeDelegate
            required property string title
            required property int page

            implicitWidth: TableView.view.width

            QQC2.ToolTip {
                text: title
            }

            contentItem: RowLayout {
                id: layout
                spacing: Kirigami.Units.smallSpacing

                KD.TitleSubtitle {
                    id: nameLabel
                    title: treeDelegate.title
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: treeDelegate.page
                    Layout.alignment: Qt.AlignRight
                }
            }
        }
    }
}
