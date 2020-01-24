INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xarchive.h \
    $$PWD/xsevenzip.h \
    $$PWD/xzip.h \
    $$PWD/xcab.h \
    $$PWD/xrar.h

SOURCES += \
    $$PWD/xarchive.cpp \
    $$PWD/xsevenzip.cpp \
    $$PWD/xzip.cpp \
    $$PWD/xcab.cpp \
    $$PWD/xrar.cpp

!contains(XCONFIG, xbinary) {
    XCONFIG += xbinary
    include(../Formats/xbinary.pri)
}

!contains(XCONFIG, zlib) {
    XCONFIG += zlib
    include(3rdparty/zlib/zlib.pri)
}

!contains(XCONFIG, bzip2) {
    XCONFIG += bzip2
    include(3rdparty/bzip2/bzip2.pri)
}

!contains(XCONFIG, lzma) {
    XCONFIG += lzma
    include(3rdparty/lzma/lzma.pri)
}
