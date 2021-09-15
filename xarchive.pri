INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xarchive.h \
    $$PWD/xmachofat.h \
    $$PWD/xsevenzip.h \
    $$PWD/xzip.h \
    $$PWD/xcab.h \
    $$PWD/xrar.h

SOURCES += \
    $$PWD/xarchive.cpp \
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
    include($$PWD/../Formats/xmach.pri) # MACHFAT archive
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
