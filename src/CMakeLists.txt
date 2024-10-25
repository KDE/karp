# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

add_executable(deafed)

qt_add_qml_module(deafed
    URI org.kde.deafed
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/About.qml
        ui/ArrangePage.qml
        ui/EditDelegate.qml
        ui/PdfMetadataDialog.qml
        ui/PdfFilesDialog.qml
        ui/Settings/SettingsPage.qml
        ui/Settings/PdfTools.qml
)

target_sources(deafed PUBLIC
    main.cpp
    deafed.cpp
    pdfpage.cpp
    pdffile.cpp
    pdfeditmodel.cpp
    pdfpageitem.cpp
    pdfsorganizer.cpp
)

target_link_libraries(deafed
    PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Pdf
    KF6::I18n
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Kirigami
)
target_include_directories(deafed PUBLIC ${CMAKE_BINARY_DIR})

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
        # app icon
    )
else()
    target_link_libraries(deafed PUBLIC Qt::Widgets)
endif()

kconfig_add_kcfg_files(deafed GENERATE_MOC deafedconfig.kcfgc)

install(TARGETS deafed ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
