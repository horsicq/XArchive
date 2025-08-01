INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/Compress
DEPENDPATH += $$PWD/Compress

HEADERS += \
    $$PWD/Compress/xlzhdecoder.h \
    $$PWD/Compress/xrardecoder.h \
    $$PWD/Compress/xit214decoder.h \
    $$PWD/Compress/xdeflatedecoder.h \
    $$PWD/Compress/ximplodedecoder.h \
    $$PWD/Compress/xlzmadecoder.h \
    $$PWD/Compress/xstoredecoder.h \
    $$PWD/Compress/xbzip2decoder.h \
    $$PWD/Compress/xlzssdecoder.h \
    $$PWD/Compress/xshrinkdecoder.h \
    $$PWD/Compress/xreducedecoder.h \
    $$PWD/x_ar.h \
    $$PWD/xapk.h \
    $$PWD/xapks.h \
    $$PWD/xarchive.h \
    $$PWD/xcab.h \
    $$PWD/xcfbf.h \
    $$PWD/xcompress.h \
    $$PWD/xdecompress.h \
    $$PWD/xcompresseddevice.h \
    $$PWD/xdeb.h \
    $$PWD/xdos16.h \
    $$PWD/xgzip.h \
    $$PWD/xipa.h \
    $$PWD/xjar.h \
    $$PWD/xlha.h \
    $$PWD/xmachofat.h \
    $$PWD/xrar.h \
    $$PWD/xsevenzip.h \
    $$PWD/xtar.h \
    $$PWD/xtgz.h \
    $$PWD/xzip.h \
    $$PWD/xnpm.h \
    $$PWD/xszdd.h \
    $$PWD/xbzip2.h \
    $$PWD/xxz.h \
    $$PWD/xzlib.h

SOURCES += \
    $$PWD/Compress/xlzhdecoder.cpp \
    $$PWD/Compress/xrardecoder.cpp \
    $$PWD/Compress/xit214decoder.cpp \
    $$PWD/Compress/xdeflatedecoder.cpp \
    $$PWD/Compress/ximplodedecoder.cpp \
    $$PWD/Compress/xlzmadecoder.cpp \
    $$PWD/Compress/xstoredecoder.cpp \
    $$PWD/Compress/xbzip2decoder.cpp \
    $$PWD/Compress/xlzssdecoder.cpp \
    $$PWD/Compress/xshrinkdecoder.cpp \
    $$PWD/Compress/xreducedecoder.cpp \
    $$PWD/x_ar.cpp \
    $$PWD/xapk.cpp \
    $$PWD/xapks.cpp \
    $$PWD/xarchive.cpp \
    $$PWD/xcab.cpp \
    $$PWD/xcfbf.cpp \
    $$PWD/xcompress.cpp \
    $$PWD/xdecompress.cpp \
    $$PWD/xcompresseddevice.cpp \
    $$PWD/xdeb.cpp \
    $$PWD/xdos16.cpp \
    $$PWD/xgzip.cpp \
    $$PWD/xipa.cpp \
    $$PWD/xjar.cpp \
    $$PWD/xlha.cpp \
    $$PWD/xmachofat.cpp \
    $$PWD/xrar.cpp \
    $$PWD/xsevenzip.cpp \
    $$PWD/xtar.cpp \
    $$PWD/xtgz.cpp \
    $$PWD/xzip.cpp \
    $$PWD/xnpm.cpp \
    $$PWD/xszdd.cpp \
    $$PWD/xbzip2.cpp \
    $$PWD/xxz.cpp \
    $$PWD/xzlib.cpp

!contains(XCONFIG, xbinary) {
    XCONFIG += xbinary
    include($$PWD/../Formats/xbinary.pri)
}

!contains(XCONFIG, xmach) {
    XCONFIG += xmach
    include($$PWD/../Formats/exec/xmach.pri) # MACHFAT archive contains Mach-O
}

!contains(XCONFIG, xjavaclass) {
    XCONFIG += xjavaclass
    include($$PWD/../Formats/formats/xjavaclass.pri)
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
