cmake_minimum_required(VERSION 3.16)
project(
  dsbmixer
  HOMEPAGE_URL https://github.com/mrclksr/DSBMixer
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROGRAM ${CMAKE_PROJECT_NAME})
set(BACKEND "${CMAKE_PROJECT_NAME}_backend")
set(PA_SCRIPT "${CMAKE_PROJECT_NAME}-pa")
set(DBUS_SCRIPT "${CMAKE_PROJECT_NAME}-dbus")
set(SCRIPTS_DIR "${CMAKE_INSTALL_PREFIX}/libexec")

if(NOT DEFINED DATADIR)
  set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME})
endif(NOT DEFINED DATADIR)

set(BACKEND_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/libexec)
set(PATH_BACKEND ${BACKEND_INSTALL_DIR}/${BACKEND})
set(PATH_DSBWRTSYSCTL ${CMAKE_INSTALL_PREFIX}/bin/dsbwrtsysctl)
set(PATH_DOC_URL "https://github.com/mrclksr/DSBMixer/blob/master/README.adoc")
set(PATH_DSBMIXER_PA "${SCRIPTS_DIR}/${PA_SCRIPT}")
set(LOCALE_PATH "${DATADIR}")

if(PROJECT_VERSION)
  set(PROGRAM_VERSION ${PROJECT_VERSION})
else()
  execute_process(
    COMMAND sh -c "git describe --abbrev=12 --always --tags | tr -d '\n'"
    OUTPUT_VARIABLE PROGRAM_VERSION)
  set(PROGRAM_VERSION "git hash: ${PROGRAM_VERSION}")
endif()

option(USE_QT6 "Build with Qt6." OFF)
if(USE_QT6)
  set(QT_MAJOR_VERSION 6)
else()
  set(QT_MAJOR_VERSION 5)
endif()
option(WITHOUT_DEVD "Build without devd support." OFF)
option(PORTS "Set to ON when building from ports" OFF)

if(NOT WITHOUT_DEVD)
  set(OPTIONAL_SOURCES src/thread.cpp)
endif()

set(SOURCE_FILES
    ${OPTIONAL_SOURCES}
    src/aboutwin.cpp
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
    src/soundsettings.cpp
    lib/qt-helper/qt-helper.cpp
    lib/config.c
    lib/dsbcfg/dsbcfg.c
    lib/libdsbmixer.c)

find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS Widgets LinguistTools
                                                       DBus)

if(USE_QT6)
  find_package(KF${QT_MAJOR_VERSION}StatusNotifierItem)
else()
  find_package(KF${QT_MAJOR_VERSION}Notifications)
endif()

if(USE_QT6)
  qt_standard_project_setup()
else()
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
endif()

set(TS_FILES locale/dsbmixer_de.ts locale/dsbmixer_fr.ts locale/dsbmixer_sv.ts)

if(NOT USE_QT6)
  qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
  list(APPEND SOURCE_FILES ${QM_FILES})
endif()

add_executable(dsbmixer ${SOURCE_FILES} resources.qrc)

if(USE_QT6)
  qt_add_translations(dsbmixer TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE
                      QM_FILES)
endif()

target_include_directories(
  dsbmixer
  PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}/lib"
         "${CMAKE_CURRENT_SOURCE_DIR}/lib/dsbcfg"
         "${CMAKE_CURRENT_SOURCE_DIR}/lib/qt-helper")

configure_file(defs.h.in defs.h @ONLY)

add_subdirectory(lib/backend)

set(LIBS Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::DBus pthread)

if(USE_QT6)
  list(APPEND LIBS KF${QT_MAJOR_VERSION}::StatusNotifierItem)
else()
  list(APPEND LIBS KF${QT_MAJOR_VERSION}::Notifications)
endif()

target_link_libraries(dsbmixer PRIVATE ${LIBS})

install(TARGETS dsbmixer DESTINATION bin)
install(PROGRAMS scripts/${DBUS_SCRIPT} DESTINATION bin)
install(PROGRAMS scripts/${PA_SCRIPT} DESTINATION ${SCRIPTS_DIR})
install(FILES "${CMAKE_PROJECT_NAME}.desktop" DESTINATION share/applications)
install(FILES ${QM_FILES} DESTINATION "${DATADIR}")
