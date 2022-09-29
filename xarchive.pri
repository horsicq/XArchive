INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/x_ar.h \
    $$PWD/xarchive.h \
    $$PWD/xgzip.h \
    $$PWD/xmachofat.h \
    $$PWD/xsevenzip.h \
    $$PWD/xzip.h \
    $$PWD/xcab.h \
    $$PWD/xrar.h

SOURCES += \
    $$PWD/x_ar.cpp \
    $$PWD/xarchive.cpp \
    $$PWD/xgzip.cpp \
    $$PWD/xmachofat.cpp \
    $$PWD/xsevenzip.cpp \
    $$PWD/xzip.cpp \
    $$PWD/xcab.cpp \
    $$PWD/xrar.cpp

!contains(XCONFIG, xbinary) {
    XCONFIG += xbinary
    include($$PWD/../Formats/xbinary.pri)
}

!contains(XCONFIG, xmach) {
    XCONFIG += xmach
    include($$PWD/../Formats/xmach.pri) # MACHFAT archive contains Mach-O
}

!contains(XCONFIG, zlib) {
    XCONFIG += zlib
    include($$PWD/3rdparty/zlib/zlib.pri)
}

!contains(XCONFIG, bzip2) {
    XCONFIG += bzip2
    include($$PWD/3rdparty/bzip2/bzip2.pri)
}

!contains(XCONFIG, lzma) {
    XCONFIG += lzma
    include($$PWD/3rdparty/lzma/lzma.pri)
}

DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/xarchive.cmake
