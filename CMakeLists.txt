cmake_minimum_required(VERSION 3.16)
project(LazyMansClang VERSION 2.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent LinguistTools)

set(TS_FILES LazyMansClang_en_GB.ts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ${TS_FILES}
)

qt_add_executable(LazyMansClang
    MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
        aboutdialog.cpp
        aboutdialog.h
        mainwindow.cpp
        mainwindow.h
        main.cpp
)


# Translations
qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})


target_link_libraries(LazyMansClang
    PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent
)

# macOS bundle settings
set_target_properties(LazyMansClang PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LazyMansClang
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS LazyMansClang
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(LazyMansClang)
