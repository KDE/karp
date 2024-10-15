# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

# Target: static library
qt_add_qml_module(deafed_static
    STATIC
    URI org.kde.deafed
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/About.qml
        ui/ArrangePage.qml
        ui/DeletedDelegate.qml
        ui/PdfMetadataDialog.qml
)

target_sources(deafed_static PUBLIC
    deafed.cpp
    pdfeditmodel.cpp
    pdfpageitem.cpp
)

target_link_libraries(deafed_static PUBLIC
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
)
target_include_directories(deafed_static PUBLIC ${CMAKE_BINARY_DIR})

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
        # app icon
    )
else()
    target_link_libraries(deafed_static PUBLIC Qt::Widgets)
endif()

kconfig_add_kcfg_files(deafed_static GENERATE_MOC deafedconfig.kcfgc)

# Target: main executable
add_executable(deafed main.cpp)
target_link_libraries(deafed PUBLIC deafed_static deafed_staticplugin)
install(TARGETS deafed ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
