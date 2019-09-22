INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

win32-g++ {
    LIBS += $$PWD/libs/win32-g++/zlib.lib // TODO
}
win32-msvc* {
    contains(QMAKE_TARGET.arch, x86_64) {
        LIBS += $$PWD/libs/win64-msvc/zlib.lib
    } else {
        LIBS += $$PWD/libs/win32-msvc/zlib.lib
    }
}

unix:!macx {
    BITSIZE = $$system(getconf LONG_BIT)
    if (contains(BITSIZE, 64)) {
        LIBS +=  $$PWD/libs/lin64/libzlib.a
    }
    if (contains(BITSIZE, 32)) {
        LIBS +=  $$PWD/libs/lin32/libzlib.a
    }
}
unix:macx {
    LIBS +=  $$PWD/libs/mac/libzlib.a
}
