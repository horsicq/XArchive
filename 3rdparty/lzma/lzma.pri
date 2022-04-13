INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/src

# TODO ARM
win32-g++ {
    contains(QT_ARCH, i386) {
        LIBS += $$PWD/libs/win32-g++/liblzma.a
    } else {
        LIBS += $$PWD/libs/win64-g++/liblzma.a
    }
}
# TODO ARM
win32-msvc* {
    contains(QMAKE_TARGET.arch, x86_64) {
        LIBS += $$PWD/libs/win64-msvc/lzma.lib
    } else {
        LIBS += $$PWD/libs/win32-msvc/lzma.lib
    }
}
unix:!macx {
    BITSIZE = $$system(getconf LONG_BIT)
    if (contains(BITSIZE, 64)) {
        LIBS +=  $$PWD/libs/lin64/liblzma.a
    }
    if (contains(BITSIZE, 32)) {
        LIBS +=  $$PWD/libs/lin32/liblzma.a
    }
}
unix:macx {
    LIBS +=  $$PWD/libs/mac/liblzma.a
}
