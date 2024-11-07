// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 by Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * @brief A Form delegate with text field and button to select a path
 * of a folder or a file
 *
 * It shows suggestions (auto completion) when 'enableSuggest' is set.
 * When path is typed a textField.popup appears with current folder content.
 * The top-most suggestion can be selected by TAB key
 * or the list can be navigated by arrow keys and then selected.
 * 'nameFilters' array can limit suggested file to desired types.
 *
 * ```qml
 * FormPathDelegate {
 *   nameFilters: [ "*.png" ]
 *   icon.source: "image-png"
 *   path: "/home/user/Images"
 * }
 * ```
 *
 * @inherit AbstractFormDelegate
 */
FormCard.AbstractFormDelegate {
    id: root

    /**
     * @brief File or Folder path. Just alias of 'text' property to clear it purpose.
     */
    property alias path: root.text

    /**
     * @brief The enum to determine is a path for file of for folder/directory.
     */
    enum PathType {
        File,
        Folder
    }

    /**
     * @brief pathType can be File or Folder. By default is File.
     */
    property int pathType: FormPathDelegate.File

    /**
     * @brief A label containing primary text that appears above and
     * to the left the text field.
     */
    property string label

    property alias labelVisible: pathLabel.visible

    /**
     * @brief Icon can be adjusted by icon property. By default it is folder-image.
     */
    icon.source: "document-open-folder"

    /**
     * @brief enableSuggest enables file/folder name suggestions during typing.
     */
    property bool enableSuggest: true

    /**
     * @brief array of file extensions to filtering names when
     * enableSuggest is set. Default is [ "*" ] - all files.
     */
    property alias nameFilters: folderModel.nameFilters

    /**
     * @brief This property keeps name filters for file dialog.
     * By default it is the same like nameFilters for suggestions.
     *
     * @see <a href="https://doc.qt.io/qt-6/qml-qtquick-dialogs-filedialog.html#nameFilters-prop">FileDialog.nameFilters</a>
     * The syntax differs from nameFilters of FolderListModel.
     * For FileDialog any filter has to be wrapped with ().
     * For nameFilter: { "*.png" } it is dialogNameFilters: [ "(*.png)" ]
     * and it will be done automatically from nameFilters property
     * when dialogNameFilters are set to default [].
     * To add any details to the filter in FileDialog dialogNameFilters has to be used directly:
     * dialogNameFilters: [ "Png image files (*.png)" ]
     */
    property var dialogNameFilters: []

    /**
     * @brief This property holds the `placeholderText` of the
     * internal TextField.
     *
     * This consists of secondary text shown by default on the text field
     * if no text has been written in it.
     */
    property alias placeholderText: textField.placeholderText

    /**
     * @brief This property holds the `validator` of the internal TextField.
     */
    property alias validator: textField.validator

    /**
     * @This signal is emitted when the Return or Enter key is pressed.
     *
     * Note that if there is a validator or inputMask set on the text input,
     * the signal will only be emitted if the input is in an acceptable
     * state.
     */
    signal accepted();

    /**
     * @brief This signal is emitted when the Return or Enter key is pressed
     * or the text input loses focus.
     *
     * Note that if there is a validator or inputMask set on the text input
     * and enter/return is pressed, this signal will only be emitted if
     * the input follows the inputMask and the validator returns an
     * acceptable state.
     */
    signal editingFinished();

    /**
     * @brief This signal is emitted whenever the text is edited.
     *
     * Unlike textChanged(), this signal is not emitted when the text
     * is changed programmatically, for example, by changing the
     * value of the text property or by calling ::clear().
     */
    signal textEdited();

    /**
     * @brief Clears the contents of the text input and resets partial
     * text input from an input method.
     */
    function clear(): void {
        textField.clear();
    }

    /**
     * Inserts text into the TextInput at position.
     */
    function insert(position: int, text: string): void {
        textField.insert(position, text);
    }

    /**
     * Causes all text to be selected.
     * @since Kirigami Addons 1.4.0
     */
    function selectAll(): void {
        textField.selectAll();
    }

    /**
     * Causes the text from start to end to be selected.
     * @since Kirigami Addons 1.4.0
     */
    function select(start: int, end: int): void {
        textField.select(start, end);
    }

    onActiveFocusChanged: { // propagate focus to the text field
        if (activeFocus) {
            textField.forceActiveFocus();
        }
    }

    onClicked: textField.forceActiveFocus()
    background: null
    Accessible.role: Accessible.EditableText

    contentItem: ColumnLayout {
        // spacing: Private.FormCardUnits.verticalSpacing
        spacing: Kirigami.Settings.isMobile ? Kirigami.Units.smallSpacing : Math.round(Kirigami.Units.smallSpacing / 2)
        QQC2.Label {
            id: pathLabel
            Layout.fillWidth: true
            text: root.label
            elide: Text.ElideRight
            color: root.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
            maximumLineCount: 2
            Accessible.ignored: true
        }
        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            QQC2.TextField {
                id: textField
                property QQC2.Popup popup: null
                Layout.fillWidth: true
                placeholderText: root.placeholderText
                text: root.text
                onTextChanged: root.text = text
                onAccepted: {
                    if (root.enableSuggest)
                        textField.popup?.close()
                    root.accepted()
                }
                onEditingFinished: root.editingFinished()
                onTextEdited: {
                    root.textEdited()
                    if (!root.enableSuggest)
                        return
                    if (length > 0)
                        root.updateSuggestions()
                    else
                        textField.popup?.close()
                }
                activeFocusOnTab: false
                Keys.onDownPressed: {
                    if (root.enableSuggest) {
                        textField.popup.forceActiveFocus();
                        popup?.hintListView.itemAtIndex(0)?.forceActiveFocus();
                    }
                }
                Keys.onTabPressed: (event) => {
                    if (textField.popup?.visible) {
                        if (folderModel.hintCount) {
                            let fileName = folderModel.get(folderModel.hintArray[0], "fileName")
                            if (folderModel.isFolder(folderModel.hintArray[0])) {
                                folderModel.dir += fileName + folderModel.separator
                                textField.text = folderModel.dir
                            } else {
                                textField.text = folderModel.dir + fileName
                                // TODO: hide textField.popup or allow to go trough suggestions with TAB key
                            }
                        }
                    } else
                        event.accepted = false
                }
                Keys.onEscapePressed: (event) => {
                    textField.popup?.close()
                    event.accepted = false
                }
                Keys.onPressed: (event) => { // handle pasting text
                    if (root.enableSuggest && event.matches(StandardKey.Paste)) {
                        folderModel.lastSlash = textField.text.lastIndexOf(folderModel.separator) + 1
                        folderModel.dir = textField.text.slice(0, folderModel.lastSlash)
                        // after paste updateSuggestions() will be triggered by textChanged
                    } else
                        event.accepted = false
                }
            }
            QQC2.Button {
                id: button
                icon: root.icon
                onClicked: {
                    if (root.pathType === FormPathDelegate.File) {
                        if (root.dialogNameFilters.length === 0) {
                            for (var i = 0; i < root.nameFilters.length; ++i) {
                                root.dialogNameFilters.push("(" + root.nameFilters[i] + ")")
                            }
                        }
                        fileDlgComp.createObject(root, { currentFile: folderModel.prefix + folderModel.dir, nameFilters: root.dialogNameFilters })
                    } else
                        folderDlgComp.createObject(root)
                }
            }
        }
    }

    FolderListModel {
        id: folderModel
         // Array with reference numbers to folderModel items which match current user text
        property var hintArray: []
        property int hintCount: 0 // due to JS array length is not dynamic
        property string separator: Qt.platform.os === "windows" ? "\\" : "/"
        property string dir: root.path.slice(0, lastSlash)
        property int lastSlash: textField.text.lastIndexOf(separator) + 1
        readonly property string prefix: "file://"
        showFiles: root.pathType === FormPathDelegate.File
        nameFilters: [ "*" ]
        folder: prefix + dir
        onStatusChanged: {
            if (root.enableSuggest && folderModel.dir !== "" && status == FolderListModel.Ready)
                root.updateSuggestions();
        }
    }

    function updateSuggestions() {
        if (textField.text[textField.length - 1] === folderModel.separator) {
            folderModel.lastSlash = textField.length
            folderModel.dir = textField.text.slice(0, folderModel.lastSlash)
        } else if (folderModel.lastSlash >= textField.length) {
            folderModel.lastSlash = textField.text.lastIndexOf(folderModel.separator) + 1
            folderModel.dir = textField.text.slice(0, folderModel.lastSlash)
        }
        if (folderModel.status !== FolderListModel.Ready)
            return;
        folderModel.hintArray.length = 0
        folderModel.hintCount = 0

        let searchText = textField.text.slice(folderModel.lastSlash, textField.length);
        for (var i = 0; i < folderModel.count; i++) {
            let file = folderModel.get(i, "fileName");
            if (searchText === "" || file.startsWith(searchText))
                folderModel.hintArray.push(i)
        }
        folderModel.hintCount = folderModel.hintArray.length
        // console.log(folderModel.dir, textField.text, searchText, folderModel.lastSlash, folderModel.hintCount)
        if (folderModel.hintCount && textField.activeFocus) {
            if (!textField.popup)
                textField.popup = popupComp.createObject(textField)
            textField.popup.open()
        } else
            textField.popup?.close()
    }

    Component {
        id: popupComp
        QQC2.Popup {
            property alias hintListView: hintListView
            y: textField.height
            x: Kirigami.Units.gridUnit
            width: textField.width - Kirigami.Units.gridUnit * 2
            height: Math.min(Kirigami.Units.gridUnit * 8 + Kirigami.Units.smallSpacing * 7, hintListView.contentHeight)
            padding: 0
            ListView {
                id: hintListView
                width: parent.width
                height: textField.popup?.height
                model: folderModel.hintCount
                visible: folderModel.hintCount > 0
                clip: true
                delegate: QQC2.ItemDelegate {
                    required property int index
                    implicitWidth: hintListView.width
                    text: folderModel.get(folderModel.hintArray[index], "fileName")
                    onClicked: {
                        if (folderModel.isFolder(folderModel.hintArray[index])) {
                            folderModel.dir += text + folderModel.separator
                            textField.text = folderModel.dir
                        } else
                            textField.text = folderModel.dir + text
                            textField.forceActiveFocus();
                        textField.popup.close();
                    }
                    Keys.onReturnPressed: clicked()
                    Keys.onEnterPressed: clicked()
                }
                QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
            }
        }
    }
    Component {
        id: folderDlgComp
        FolderDialog {
            visible: true
            currentFolder: folderModel.prefix + folderModel.dir
            onAccepted: {
                root.path = selectedFolder.toString().replace(folderModel.prefix, "")
                folderModel.dir = root.path
                root.accepted()
            }
            onVisibleChanged: if (!visible) destroy()
        }
    }
    Component {
        id: fileDlgComp
        FileDialog {
            visible: true
            onAccepted: {
                root.path = selectedFile.toString().replace(folderModel.prefix, "")
                folderModel.dir = root.path
                root.accepted()
            }
            onVisibleChanged: if (!visible) destroy()
        }
    }
}

