// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

// import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.deafed
import org.kde.deafed.config

FormCard.FormCardPage {
    id: root

    title: i18n("PDF Tools")

    FormCard.FormHeader { title: "QPDF" }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "QPDF " + (PDFED.qpdfVersion === "" ? i18n("not found") : PDFED.qpdfVersion)
            description: i18n("Allows PDF page arrangements.")
        }
        FormPathDelegate {
            icon.name: "system-run"
            path: DeafEdConf.qpdfPath
            labelVisible: false
            onAccepted: PDFED.checkQPDF(text)
        }
    }
    FormCard.FormHeader { title: "Ghostscript" }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "GPL Ghostscript " + (PDFED.gsVersion === "" ? i18n("not found") : PDFED.gsVersion)
            description: i18n("Manage size and quality of output PDF file.")
        }
        FormPathDelegate {
            icon.name: "system-run"
            path: DeafEdConf.gsPath
            labelVisible: false
            onAccepted: PDFED.checkGS(text)
        }
    }

}
