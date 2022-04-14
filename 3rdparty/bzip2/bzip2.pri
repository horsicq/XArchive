INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/src

# TODO ARM
win32-g++ {
    contains(QT_ARCH, i386) {
        LIBS += $$PWD/libs/win32-g++/libbzip2.a
    } else {
        LIBS += $$PWD/libs/win64-g++/libbzip2.a
    }
}
# TODO ARM
win32-msvc* {
    contains(QMAKE_TARGET.arch, x86_64) {
        LIBS += $$PWD/libs/win64-msvc/bzip2.lib
    } else {
        LIBS += $$PWD/libs/win32-msvc/bzip2.lib
    }
}
# TODO ARM
unix:!macx {
    BITSIZE = $$system(getconf LONG_BIT)
    if (contains(BITSIZE, 64)) {
        LIBS +=  $$PWD/libs/lin64/libbzip2.a
    }
    if (contains(BITSIZE, 32)) {
        LIBS +=  $$PWD/libs/lin32/libbzip2.a
    }
}
# TODO ARM M1
unix:macx {
    LIBS +=  $$PWD/libs/mac/libbzip2.a
}
