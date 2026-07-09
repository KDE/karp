# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

kde_enable_exceptions()

add_executable(karp)

ecm_qt_declare_logging_category(karp
    HEADER karp_debug.h
    IDENTIFIER KARP_LOG
    CATEGORY_NAME org.kde.karp
    DESCRIPTION "karp"
    EXPORT KARP
)

qt_add_qml_module(karp
    URI org.kde.karp
    QML_FILES
        qml/Main.qml
        qml/Actions.qml
        qml/MainPage.qml
        qml/MainToolbar.qml
        qml/EditDelegate.qml
        qml/PdfView.qml

        qml/Dialogs/SelectPagesDialog.qml
        qml/Dialogs/MovePagesDialog.qml
        qml/Dialogs/PdfMetadataDialog.qml
        qml/Dialogs/PdfPassDialog.qml
        qml/Dialogs/OrganizerDialog.qml
        qml/Dialogs/OrganizerDelegate.qml
        qml/Dialogs/ExportDialog.qml

        qml/Settings/SettingsPage.qml
        qml/Settings/PdfTools.qml
        qml/Settings/General.qml
        qml/Settings/FormPathDelegate.qml

        qml/Messages/MissingPdfTool.qml
        qml/Messages/ProgressDialog.qml
        qml/Messages/InitialInfo.qml

        qml/Bookmarks/BookmarksView.qml
        qml/Bookmarks/EditBookmarkDialog.qml

    DEPENDENCIES
        QtCore
        QtQuick
    IMPORTS
        org.kde.karp.config

)


# Below definition is for qpdf library.
# @see https://qpdf.readthedocs.io/en/stable/design.html#smart-pointers
# For now there is not much use of it in Karp case
target_compile_definitions(karp PRIVATE POINTERHOLDER_TRANSITION=0)

target_sources(karp PUBLIC
    main.cpp

    app.h
    app.cpp
    bookmarkmodel.h
    bookmarkmodel.cpp
    pdfpage.h
    pdfpage.cpp
    pdffile.h
    pdffile.cpp
    pdfmetadata.h
    pdfmetadata.cpp
    pdfeditmodel.h
    pdfeditmodel.cpp
    pdfpageitem.h
    pdfpageitem.cpp
    pdfsorganizer.h
    pdfsorganizer.cpp
    pagerange.h
    pagerange.cpp
    qpdfproxy.h
    qpdfproxy.cpp
    outline.h
    outline.cpp
    toolsthread.h
    toolsthread.cpp
)

target_link_libraries(karp
    PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Pdf
    KF6::I18n
    KF6::IconThemes
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Kirigami
    KF6::KirigamiActionCollection
    KirigamiAddonsStatefulApp
    qpdf::libqpdf
    Poppler::Qt6
)

if (TARGET KF6::I18nQml)
    target_link_libraries(karp PUBLIC
        KF6::I18nQml
    )
endif()
if (TARGET KF6::Crash)
    target_link_libraries(karp PUBLIC KF6::Crash)
endif()

target_include_directories(karp PUBLIC ${CMAKE_BINARY_DIR})

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
        # app icon
    )
else()
    target_link_libraries(karp PUBLIC Qt::Widgets)
endif()


if(APPLE)
    set_target_properties(karp PROPERTIES
        MACOSX_BUNDLE_DISPLAY_NAME "Karp"
        MACOSX_BUNDLE_BUNDLE_NAME "Karp"
        MACOSX_BUNDLE_LONG_VERSION_STRING "Karp ${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.karp"
	MACOSX_BUNDLE_COPYRIGHT "2024-2025 The Karp Developers")
endif()

kconfig_add_kcfg_files(karp GENERATE_MOC karpconfig.kcfgc)

install(TARGETS karp ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
