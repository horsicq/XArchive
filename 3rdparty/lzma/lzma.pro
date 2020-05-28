QT       -= core gui

TARGET = lzma
TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += $$PWD/lzma
DEPENDPATH += $$PWD/lzma

include(../../build.pri)

CONFIG(debug, debug|release) {
    TARGET = lzmad
} else {
    TARGET = lzma
}

TARGETLIB_PATH = $$PWD

win32-g++ {
    contains(QT_ARCH, i386) {
        DESTDIR=$${TARGETLIB_PATH}/libs/win32-g++
    } else {
        DESTDIR=$${TARGETLIB_PATH}/libs/win64-g++
    }
}
win32-msvc* {
    contains(QMAKE_TARGET.arch, x86_64) {
        DESTDIR=$${TARGETLIB_PATH}/libs/win64-msvc
    } else {
        DESTDIR=$${TARGETLIB_PATH}/libs/win32-msvc
    }

}
unix:!macx {
    BITSIZE = $$system(getconf LONG_BIT)
    if (contains(BITSIZE, 64)) {
        DESTDIR=$${TARGETLIB_PATH}/libs/lin64
    }
    if (contains(BITSIZE, 32)) {
        DESTDIR=$${TARGETLIB_PATH}/libs/lin32
    }
}
unix:macx {
    DESTDIR=$${TARGETLIB_PATH}/libs/mac
}

SOURCES += \
    $$PWD/src/LzmaDec.c \
    $$PWD/src/Ppmd7.c \
    $$PWD/src/Ppmd7Dec.c

