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
 * When path is typed a popup appears with current folder content.
 * The top-most suggestion can be selected by TAB key
 * or the list can be navigated by arrow keys and then selected.
 * 'nameFilters' array can limit suggested file to desired types.
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
     * @brief This property holds the `acceptableInput` of the internal TextField.
     */
    property alias acceptableInput: textField.acceptableInput

    /**
     * @brief This property holds the current status message type of
     * the text field.
     *
     * This consists of an inline message with a colorful background
     * and an appropriate icon.
     *
     * The status property will affect the color of ::statusMessage used.
     *
     * Accepted values:
     * - `Kirigami.MessageType.Information` (blue color)
     * - `Kirigami.MessageType.Positive` (green color)
     * - `Kirigami.MessageType.Warning` (orange color)
     * - `Kirigami.MessageType.Error` (red color)
     *
     * default: `Kirigami.MessageType.Information` if ::statusMessage is set,
     * nothing otherwise.
     *
     * @see Kirigami.MessageType
     */
    property var status: Kirigami.MessageType.Information

    /**
     * @brief This property holds the current status message of
     * the text field.
     *
     * If this property is not set, no ::status will be shown.
     */
    property string statusMessage: ""

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

/// FormPathDelegate

    /**
     * @brief The enum to determine is a path for file of for folder/directory.
     */
    enum PathType {
        File,
        Folder
    }

    /**
     * @brief pathType can be File or Folder and by default is File.
     */
    property int pathType: FormPathDelegate.File

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

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing
        QQC2.TextField {
            id: textField
            Layout.fillWidth: true
            placeholderText: root.placeholderText
            text: root.text
            onTextChanged: root.text = text
            onAccepted: {
                if (root.enableSuggest)
                    popup.close()
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
                    popup.close()
            }
            activeFocusOnTab: false
            QQC2.Popup {
                id: popup
                y: textField.height
                x: Kirigami.Units.gridUnit
                width: textField.width - Kirigami.Units.gridUnit * 2
                height: Math.min(Kirigami.Units.gridUnit * 8 + Kirigami.Units.smallSpacing * 7, hintListView.contentHeight)
                padding: 0
                ListView {
                    id: hintListView
                    width: parent.width
                    height: popup.height
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
                            popup.close();
                        }
                        Keys.onReturnPressed: clicked()
                        Keys.onEnterPressed: clicked()
                    }
                    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
                }
            }
            Keys.onDownPressed: {
                if (root.enableSuggest) {
                    popup.forceActiveFocus();
                    hintListView.itemAtIndex(0)?.forceActiveFocus();
                }
            }
            Keys.onTabPressed: (event) => {
                if (popup.visible) {
                    if (folderModel.hintCount) {
                        let fileName = folderModel.get(folderModel.hintArray[0], "fileName")
                        if (folderModel.isFolder(folderModel.hintArray[0])) {
                            folderModel.dir += fileName + folderModel.separator
                            textField.text = folderModel.dir
                        } else {
                            textField.text = folderModel.dir + fileName
                            // TODO: hide popup or allow to go trough suggestions with TAB key
                        }
                    }
                } else
                    event.accepted = false
            }
            Keys.onEscapePressed: (event) => {
                popup.close()
                event.accepted = false
            }
        }
        QQC2.Button {
            id: button
            icon: root.icon
            onClicked: {
                if (root.pathType === FormPathDelegate.File) {
                    selectFileDialog.currentFile = folderModel.prefix + folderModel.dir
                    selectFileDialog.open()
                } else
                    selectFolderDialog.open()
            }
        }

        Kirigami.InlineMessage {
            id: formErrorHandler
            visible: root.statusMessage.length > 0
            Layout.topMargin: visible ? Kirigami.Units.smallSpacing : 0
            Layout.fillWidth: true
            text: root.statusMessage
            type: root.status
        }
    }

    FolderListModel {
        id: folderModel
         // Array with reference numbers to @p folderModel items which match current path text
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
                folderModel.hintArray.push(i);
        }
        folderModel.hintCount = folderModel.hintArray.length
        // console.log(folderModel.dir, textField.text, searchText, folderModel.lastSlash, folderModel.hintCount)
        if (folderModel.hintCount && textField.activeFocus)
            popup.open();
        else
            popup.close();
    }

    FolderDialog {
        id: selectFolderDialog
        currentFolder: folderModel.prefix + folderModel.dir
        onAccepted: {
            root.path = selectedFolder.toString().replace(folderModel.prefix, "")
            folderModel.dir = root.path
            root.accepted()
        }
    }
    FileDialog {
        id: selectFileDialog
        // nameFilters: folderModel.nameFilters // TODO: convert
        onAccepted: {
            root.path = selectedFile.toString().replace(folderModel.prefix, "")
            folderModel.dir = root.path
            root.accepted()
        }
    }
}

