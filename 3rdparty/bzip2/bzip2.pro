QT       -= core gui

TARGET = bzip2
TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += $$PWD/bzip2
DEPENDPATH += $$PWD/bzip2

include(../../build.pri)

CONFIG(debug, debug|release) {
    TARGET = bzip2d
} else {
    TARGET = bzip2
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
    $$PWD/src/bzip2.c \
    $$PWD/src/crctable.c \
    $$PWD/src/bzlib.c \
    $$PWD/src/compress.c \
    $$PWD/src/decompress.c \
    $$PWD/src/blocksort.c \
    $$PWD/src/randtable.c \
    $$PWD/src/huffman.c

