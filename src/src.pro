include(../defs.inc)

TEMPLATE    = app
TARGET 	    = ../dsbmixer 
DEPENDPATH  += . qt-helper ../lib ../lib/backend
INCLUDEPATH += . qt-helper ../lib
RESOURCES += ../resources.qrc
QT += widgets

# Input
HEADERS += chanslider.h \
	   mainwin.h \
	   mixer.h \
	   preferences.h \
	   thread.h \
	   mixertrayicon.h \
	   ../lib/qt-helper/qt-helper.h
SOURCES += chanslider.cpp \
	   main.cpp \
	   mainwin.cpp \
	   mixer.cpp \
	   preferences.cpp \
	   thread.cpp \
	   mixertrayicon.cpp \
	   ../lib/qt-helper/qt-helper.cpp \
	   ../lib/libdsbmixer.c \
	   ../lib/config.c \
	   ../lib/dsbcfg/dsbcfg.c

QMAKE_POST_LINK=$(STRIP) $(TARGET)

