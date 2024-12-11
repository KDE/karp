# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2024 by Tomasz Bojczuk <seelook@gmail.com>

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fexceptions")

add_executable(karp)

ecm_qt_declare_logging_category(karp HEADER karp_debug.h IDENTIFIER KARP_LOG
	CATEGORY_NAME org.kde.karp DESCRIPTION "karp" EXPORT KARP)

qt_add_qml_module(karp
    URI org.kde.karp
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/About.qml
        qml/MainPage.qml
        qml/MainToolbar.qml
        qml/EditDelegate.qml

        qml/Dialogs/SelectPagesDialog.qml
        qml/Dialogs/MovePagesDialog.qml
        qml/Dialogs/PdfMetadataDialog.qml
        qml/Dialogs/PdfFilesDialog.qml
        qml/Dialogs/PdfPassDialog.qml
        qml/Dialogs/PdfFileDelegate.qml

        qml/Settings/SettingsPage.qml
        qml/Settings/PdfTools.qml
        qml/Settings/General.qml
        qml/Settings/FormPathDelegate.qml

        qml/Messages/MissingPdfTool.qml
        qml/Messages/ProgressDialog.qml
        qml/Messages/InitialInfo.qml
)


# Below definition is for qpdf library.
# @see https://qpdf.readthedocs.io/en/stable/design.html#smart-pointers
# For now there is not much use of it in Karp case
target_compile_definitions(karp PRIVATE POINTERHOLDER_TRANSITION=0)

target_sources(karp PUBLIC
    main.cpp
    app.cpp
    pdfpage.cpp
    pdffile.cpp
    pdfmetadata.cpp
    pdfeditmodel.cpp
    pdfpageitem.cpp
    pdfsorganizer.cpp
    toolsthread.cpp
    pagerange.cpp
    qpdfproxy.cpp

    app.h
    pdfpage.h
    pdffile.h
    pdfmetadata.h
    pdfeditmodel.h
    pdfpageitem.h
    pdfsorganizer.h
    toolsthread.h
    pagerange.h
    qpdfproxy.h
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
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Kirigami
    KF6::Crash
    KirigamiAddonsStatefulApp
    qpdf::libqpdf
)

if(TARGET KF6::I18nQml)
target_link_libraries(karp PUBLIC
        KF6::I18nQml
)
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

kconfig_add_kcfg_files(karp GENERATE_MOC karpconfig.kcfgc)

install(TARGETS karp ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
