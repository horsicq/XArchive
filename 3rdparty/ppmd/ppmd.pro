QT       -= core gui

TARGET = ppmd
TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/src

include(../../build.pri)

win32{
    TARGET = ppmd-win-$${QT_ARCH}
}
unix:!macx {
    TARGET = ppmd-unix-$${QT_ARCH}
}
unix:macx {
    TARGET = ppmd-macos-$${QT_ARCH}
}

SOURCES += \
    $$PWD/src/Ppmd8.c \
    $$PWD/src/Ppmd8Dec.c

HEADERS += \
    $$PWD/src/Ppmd8.h

TARGETLIB_PATH = $$PWD

DESTDIR=$${TARGETLIB_PATH}/libs

