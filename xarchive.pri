INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xarchive.h \
    $$PWD/xzip.h

SOURCES += \
    $$PWD/xarchive.cpp \
    $$PWD/xzip.cpp

!contains(XCONFIG, qbinary) {
    XCONFIG += qbinary
    include(../Formats/qbinary.pri)
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
