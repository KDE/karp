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
    * Page number, after or before page range is going to be pasted.
    * Negative value means: paste before.
    */
    property int targetPage: 1

    /**
     * @brief page range - just @p from and @p to are used
     */
    property pageRange range

    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    implicitWidth: Kirigami.Units.gridUnit * 30
    implicitHeight: Kirigami.Units.gridUnit * 18
    padding: Kirigami.Units.gridUnit

    QQC2.ButtonGroup { id: radioGr }

    ColumnLayout {
        id: inner

        property int selectedCount: toSpin.value - fromSpin.value + 1
        property var okButton // it is valid only after component creation

        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label { text: i18n("Page range") }
        Kirigami.AbstractCard {
            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                QQC2.Button {
                    icon.name: "go-first"
                    onClicked: fromSpin.value = 1
                }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.Label { text: i18n("from") }
                QQC2.SpinBox {
                    id: fromSpin
                    from: 1
                    to: root.pageCount
                    onValueModified: checkTargetValue()
                }
                QQC2.Label { text: i18n("to") }
                QQC2.SpinBox {
                    id: toSpin
                    from: fromSpin.value
                    to: root.pageCount
                    onValueModified: checkTargetValue()
                }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.Button {
                    icon.name: "go-last"
                    onClicked: toSpin.value = root.pageCount
                }
            }
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Move to")
        }
        Kirigami.AbstractCard {
            id: targetCard
            enabled: fromSpin.value !== 1 || toSpin.value !== root.pageCount
            Layout.fillWidth: true
            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                QQC2.Button {
                    icon.name: "go-first"
                    onClicked: {
                        targetSpin.value = 1
                        checkTargetValue()
                    }
                }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.ComboBox {
                    id: targetCombo
                    model: [ i18nc("@combobox:before/after", "before"), i18nc("@combobox:before/after", "after") ]
                    currentIndex: root.targetPage > 0 ? 1 : 0
                    onActivated: root.targetPage = targetSpin.value * (currentIndex === 0 ? -1 : 1)
                }
                QQC2.SpinBox {
                    id: targetSpin
                    property int prevValue: Math.abs(root.targetPage)
                    from: 1
                    to: pdfModel.pageCount
                    value: Math.abs(root.targetPage)
                    onValueModified: {
                        if (value === fromSpin.value) {
                            if (value === toSpin.value) {
                                if (value < prevValue) {
                                    if (fromSpin.value > 1)
                                        value = fromSpin.value - 1
                                    else
                                        value = toSpin.value + 1
                                } else {
                                    if (toSpin.value < root.pageCount)
                                        value = toSpin.value + 1
                                    else
                                        value = fromSpin.value - 1
                                }

                            } else if (toSpin.value < root.pageCount)
                                value = toSpin.value + 1
                            else
                                value = fromSpin.value - 1
                        } else if (value === toSpin.value) {
                            if (fromSpin.value > 1)
                                value = fromSpin.value - 1
                            else
                                value = toSpin.value + 1
                        } else
                            checkTargetValue()
                        root.targetPage = value * (targetCombo.currentIndex === 0 ? -1 : 1)
                        prevValue = value
                    }
                }
                QQC2.Label { text: i18nc("Move page range after/before (number)", "page") }
                Item { height: 1; Layout.fillWidth: true }
                QQC2.Button {
                    icon.name: "go-last"
                    onClicked: {
                        targetSpin.value = root.pageCount
                        checkTargetValue()
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        inner.okButton = standardButton(Kirigami.Dialog.Ok)
        inner.okButton.text = i18nc("@action:button", "Move")
        inner.okButton.icon.name = "transform-move"
        inner.okButton.enabled = Qt.binding(function(){ return targetCard.enabled })
        // Do not bind result properties, assign them just once
        fromSpin.value = range.from
        toSpin.value = range.to
        checkTargetValue()
    }

    onAccepted: {
        range.from = fromSpin.value
        range.to = toSpin.value
        targetPage = targetSpin.value * (targetCombo.currentIndex === 0 ? -1 : 1)
    }

    onClosed: destroy()

    function checkTargetValue() : void {
        if (targetSpin.value >= fromSpin.value && targetSpin.value <= toSpin.value) {
            let mid = Math.max(fromSpin.value + (toSpin.value - fromSpin.value) / 2, 1)
            if (targetSpin.value >= mid) {
                if (toSpin.value < root.pageCount)
                    targetSpin.value = toSpin.value + 1
                else
                    targetSpin.value = fromSpin.value - 1
            } else {
                if (fromSpin.value > 1)
                    targetSpin.value = fromSpin.value - 1
                else
                    targetSpin.value = toSpin.value + 1
            }
        }
    }
}
