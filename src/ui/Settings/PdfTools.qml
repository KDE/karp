// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.deafed

FormCard.FormCardPage {
    id: root

    title: i18n("PDF Tools")

    FormCard.FormHeader {
        title: ""
    }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "QPDF 1.2.3"
            description: i18n("Allows PDF page arrangements.")
        }
        FormCard.FormDelegateSeparator {
            above: qpdfButt
        }
        FormCard.FormButtonDelegate {
            id: qpdfButt
            icon.name: "list-add"
            text: "/usr/bin/qpdf"
            onClicked: console.info("Look for qpdf")
        }
    }
    FormCard.FormHeader {
      title: ""
    }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "GPL Ghostscript"
            description: i18n("Manage quality of output PDF file and their size.")
        }
        FormCard.FormDelegateSeparator {
            above: ps2Button
        }
        FormCard.FormButtonDelegate {
            id: ps2Button
            icon.name: "list-add"
            text: "/usr/bin/ps2pdf"
        }
        FormCard.FormDelegateSeparator {
            above: pdf2Button
            below: ps2Button
        }
        FormCard.FormButtonDelegate {
            id: pdf2Button
            icon.name: "list-add"
            text: "/usr/bin/pdf2ps"
        }
    }

}
