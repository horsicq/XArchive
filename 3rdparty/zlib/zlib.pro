QT       -= core gui

TARGET = zlib
TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += $$PWD/zlib
DEPENDPATH += $$PWD/zlib

include(../../build.pri)

CONFIG(debug, debug|release) {
    TARGET = zlibd
} else {
    TARGET = zlib
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
    $$PWD/src/deflate.c \
    $$PWD/src/inflate.c \
    $$PWD/src/adler32.c \
    $$PWD/src/crc32.c \
    $$PWD/src/inffast.c \
    $$PWD/src/inftrees.c \
    $$PWD/src/trees.c \
    $$PWD/src/zutil.c
