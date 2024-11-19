// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import QtQuick.Layouts
import org.kde.karp

Kirigami.Dialog {
    id: root

    property int pageCount: 1

    /**
     * @brief pageRange has the following properties:
     * from, to, n (step) and type.
     *
     *
     * @p type of Page range:
     * - AllInRange
     * - EveryNPage
     * - AllOutOfRange
     * @p n is used when type is EveryNPage
     */
    property pageRange range

    /**
     * Extra item above page range card
     */
    property alias topItem: topLoader.sourceComponent

    /**
     * Text on confirmation/accept button, i.e.: Delete
     */
    property string acceptText: ""

    /**
     * Text on confirmation/accept button, i.e.: edit-delete
     */
    property string acceptIcon: ""

    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: Kirigami.Units.gridUnit * 20
    padding: Kirigami.Units.gridUnit

    QQC2.ButtonGroup { id: radioGr }

    ColumnLayout {
        id: inner

        property int selectedCount: toSpin.value - fromSpin.value + 1

        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        Loader {
            id: topLoader
            active: true
            height: item?.implicitHeight
            Layout.fillWidth: true
        }

        QQC2.Label { text: i18n("Page range") }
        Kirigami.AbstractCard {
            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                QQC2.Button {
                    text: i18n("first")
                    onClicked: fromSpin.value = 1
                }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.Label { text: i18n("from") }
                QQC2.SpinBox {
                    id: fromSpin
                    from: 1
                    to: root.pageCount
                }
                QQC2.Label { text: i18n("to") }
                QQC2.SpinBox {
                    id: toSpin
                    from: fromSpin.value
                    to: root.pageCount
                }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.Button {
                    text: i18n("last")
                    onClicked: toSpin.value = root.pageCount
                }
            }
        }
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            QQC2.RadioButton {
                id: allPagesRadio
                text: i18n("all pages in selected range")
                QQC2.ButtonGroup.group: radioGr
            }
            RowLayout {
                enabled: inner.selectedCount > 1
                spacing: Kirigami.Units.largeSpacing
                QQC2.RadioButton {
                    id: everyNpageRadio
                    text: i18n("every %1 page", nSpin.value)
                    QQC2.ButtonGroup.group: radioGr
                }
                QQC2.SpinBox {
                    enabled: everyNpageRadio.checked
                    id: nSpin
                    from: 1
                    to: inner.selectedCount / 2
                }
            }
            QQC2.RadioButton {
                id: allOutRadio
                enabled: inner.selectedCount < root.pageCount
                text: i18n("all pages out of range")
                QQC2.ButtonGroup.group: radioGr
            }
        }
    }

    Component.onCompleted: {
        if (root.acceptText !== "")
            standardButton(Kirigami.Dialog.Ok).text = root.acceptText
        if (root.acceptIcon !== "")
            standardButton(Kirigami.Dialog.Ok).icon.name = root.acceptIcon
        // Do not bind result properties, assign them just one
        fromSpin.value = range.from
        toSpin.value = range.to
        nSpin.value = range.n
        allPagesRadio.checked = range.allInRange
        everyNpageRadio.checked = range.everyN
        allOutRadio.checked = range.allOutOfRange
    }

    onAccepted: {
        range.from = fromSpin.value
        range.to = toSpin.value
        range.n = nSpin.value
        range.type = allPagesRadio.checked ? PageRange.AllInRange : (everyNpageRadio.checked ? PageRange.EveryNPage : PageRange.AllOutOfRange)
    }

    onClosed: destroy()
}
