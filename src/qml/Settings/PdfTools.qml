// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

import org.kde.karp
import org.kde.karp.config
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root

    title: i18n("PDF Tools")

    FormCard.FormHeader {
        title: "QPDF"
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "QPDF " + APP.qpdfVersion
            description: i18n("Allows PDF page arrangements.")
        }

        FormCard.FormComboBoxDelegate {
            text: i18n("Force PDF version of generated file")
            description: i18n("When set to default, PDF version of input file will be used.")
            model: [i18n("default"), "1.4", "1.5", "1.6", "1.7"]
            currentIndex: KarpConf.pdfVersion >= 14 && KarpConf.pdfVersion <= 17 ? KarpConf.pdfVersion - 13 : 0
            onActivated: KarpConf.pdfVersion = currentIndex === 0 ? 0 : 13 + currentIndex
        }

    }

    FormCard.FormHeader {
        title: "Ghostscript"
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: "GPL Ghostscript " + (APP.gsVersion === "" ? i18n("not found") : APP.gsVersion)
            description: i18n("Manage size and quality of output PDF file.")
        }

        FormPathDelegate {
            icon.name: "system-run"
            path: KarpConf.gsPath
            labelVisible: false
            onAccepted: APP.checkGS(text)
        }

    }

}
