cmake_minimum_required(VERSION 3.16)
project(dsbmixer LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROGRAM ${CMAKE_PROJECT_NAME})
set(BACKEND "${CMAKE_PROJECT_NAME}_backend")
set(RESTART_PA "${CMAKE_PROJECT_NAME}_restart_pa")
set(SCRIPTS_DIR "${CMAKE_INSTALL_PREFIX}/libexec")

if (NOT DEFINED DATADIR)
  set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME})
endif(NOT DEFINED DATADIR)

set(BACKEND_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/libexec)
set(PATH_BACKEND ${BACKEND_INSTALL_DIR}/${BACKEND})
set(PATH_DSBWRTSYSCTL ${CMAKE_INSTALL_PREFIX}/bin/dsbwrtsysctl)
set(PATH_RESTART_PA ${SCRIPTS_DIR}/${RESTART_PA})

option(USE_QT6 "Build with Qt6." OFF)
if (USE_QT6)
  set(QT_MAJOR_VERSION 6)
else()
  set(QT_MAJOR_VERSION 5)
endif()
option(WITHOUT_DEVD "Build without devd support." OFF)

add_subdirectory(lib/backend)
if (NOT WITHOUT_DEVD)
  set(OPTIONAL_SOURCES src/thread.cpp)
endif()

set(SOURCE_FILES
  ${OPTIONAL_SOURCES}
  src/appsmixer.cpp
  src/chanslider.cpp
  src/mixer.cpp
  src/preferences.cpp
  src/mixerlist.cpp
  src/mixertrayicon.cpp
  src/iconloader.cpp
  src/main.cpp
  src/mainwin.cpp
  src/mixertabs.cpp
  src/mixersettings.cpp
  src/restartapps.cpp
  lib/qt-helper/qt-helper.cpp
  lib/config.c
  lib/dsbcfg/dsbcfg.c
  lib/libdsbmixer.c)

find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS Widgets LinguistTools)
if (${QT_MAJOR_VERSION} GREATER OR EQUAL 6)
  qt_standard_project_setup()
else()
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
endif()

set(TS_FILES
  locale/dsbmixer_de.ts
  locale/dsbmixer_fr.ts
  locale/dsbmixer_sv.ts)

if (NOT USE_QT6)
  qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
  list(APPEND SOURCE_FILES ${QM_FILES})
endif()

add_executable(dsbmixer ${SOURCE_FILES} resources.qrc)

if (USE_QT6)
  qt_add_translations(dsbmixer
   TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
endif()

target_include_directories(dsbmixer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/dsbcfg
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/qt-helper)

add_compile_definitions(
  PROGRAM="${CMAKE_PROJECT_NAME}"
  LOCALE_PATH="${DATADIR}"
  PATH_BACKEND="${BACKEND_INSTALL_DIR}/${BACKEND}"
  PATH_DSBWRTSYSCTL="${PATH_DSBWRTSYSCTL}"
  PATH_RESTART_PA="${SCRIPTS_DIR}/${RESTART_PA}")
if (WITHOUT_DEVD)
  add_compile_definitions("-DWITHOUT_DEVD")
endif()

target_link_libraries(dsbmixer PRIVATE Qt${QT_MAJOR_VERSION}::Widgets pthread)

install(TARGETS dsbmixer DESTINATION bin)
install(FILES "${CMAKE_PROJECT_NAME}.desktop"
  DESTINATION share/applications)
install(FILES ${QM_FILES} DESTINATION "${DATADIR}")
