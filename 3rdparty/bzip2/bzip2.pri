INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

win32-g++ {
    LIBS += $$PWD/libs/win32-g++/bzip2.lib
}
win32-msvc* {
    contains(QMAKE_TARGET.arch, x86_64) {
        LIBS += $$PWD/libs/win64-msvc/bzip2.lib
    } else {
        LIBS += $$PWD/libs/win32-msvc/bzip2.lib
    }
#    LIBS += Advapi32.lib
}
unix:!macx {
    BITSIZE = $$system(getconf LONG_BIT)
    if (contains(BITSIZE, 64)) {
        LIBS +=  $$PWD/libs/lin64/bzip2.a
    }
    if (contains(BITSIZE, 32)) {
        LIBS +=  $$PWD/libs/lin32/bzip2.a
    }
}
unix:macx {
    LIBS +=  $$PWD/libs/mac/bzip2.a
}
