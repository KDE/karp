// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.deafed
import org.kde.deafed.config

FormCard.FormCardPage {
    id: root

    title: i18n("PDF Tools")

    FormCard.FormHeader {
        title: ""
    }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "QPDF " + (PDFED.qpdfVersion === "" ? i18n("not found") : PDFED.qpdfVersion)
            description: i18n("Allows PDF page arrangements.")
        }
        FormCard.FormDelegateSeparator {
            above: qpdfButt
        }
        FormCard.FormButtonDelegate {
            id: qpdfButt
            icon.name: "list-add"
            text: DeafEdConf.qpdfPath
        }
    }
    FormCard.FormHeader {
      title: ""
    }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "GPL Ghostscript " + (PDFED.gsVersion === "" ? i18n("not found") : PDFED.gsVersion)
            description: i18n("Manage size and quality of output PDF file.")
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormButtonDelegate {
            icon.name: "list-add"
            text: DeafEdConf.pdf2psPath
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormButtonDelegate {
            icon.name: "list-add"
            text: DeafEdConf.ps2pdfPath
        }
    }

}
