# (c) 2025 Stardust Softworks
cmake_minimum_required(VERSION 3.16)

project(LazyMansClang VERSION 2.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 5/6 compatible find
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES LazyMansClang_en_GB.ts)

set(PROJECT_SOURCES
    # a
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui


    aboutdialog.cpp
    aboutdialog.h


    resources.qrc

    ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(LazyMansClang
        MACOSX_BUNDLE
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(LazyMansClang SHARED ${PROJECT_SOURCES})

    else()
        add_executable(LazyMansClang ${PROJECT_SOURCES})
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()


target_link_libraries(LazyMansClang PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# mac bundle metadata
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LazyMansClang)
endif()

set_target_properties(LazyMansClang PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    WIN32_EXECUTABLE TRUE
)


if(APPLE)
  set(APP_ICNS ${CMAKE_SOURCE_DIR}/icons/LMC0.icns)
  if(EXISTS "${APP_ICNS}")
    set_source_files_properties(${APP_ICNS}
      PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(LazyMansClang PRIVATE ${APP_ICNS})

    set_target_properties(LazyMansClang PROPERTIES
      MACOSX_BUNDLE_ICON_FILE LMC0.icns)
  endif()
endif()

include(GNUInstallDirs)
install(TARGETS LazyMansClang
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LazyMansClang)
endif()
