INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/Algos
DEPENDPATH += $$PWD/Algos
INCLUDEPATH += $$PWD/3rdparty/ppmd/src
DEPENDPATH += $$PWD/3rdparty/ppmd/src

HEADERS += \
    $$PWD/Algos/xlzhdecoder.h \
    $$PWD/Algos/xrardecoder.h \
    $$PWD/Algos/xit214decoder.h \
    $$PWD/Algos/xdeflatedecoder.h \
    $$PWD/Algos/ximplodedecoder.h \
    $$PWD/Algos/xlzmadecoder.h \
    $$PWD/Algos/xlzwdecoder.h \
    $$PWD/Algos/xascii85decoder.h \
    $$PWD/Algos/xstoredecoder.h \
    $$PWD/Algos/xbzip2decoder.h \
    $$PWD/Algos/xlzssdecoder.h \
    $$PWD/Algos/xshrinkdecoder.h \
    $$PWD/Algos/xreducedecoder.h \
    $$PWD/Algos/xzipcryptodecoder.h \
    $$PWD/Algos/xzipaesdecoder.h \
    $$PWD/Algos/xppmddecoder.h \
    $$PWD/Algos/xppmdrangedecoder.h \
    $$PWD/Algos/xppmdmodel.h \
    $$PWD/Algos/xppmd7model.h \
    $$PWD/Algos/xaesdecoder.h \
    $$PWD/Algos/xsha256decoder.h \
    $$PWD/x_ar.h \
    $$PWD/xapk.h \
    $$PWD/xapks.h \
    $$PWD/xarchive.h \
    $$PWD/xcab.h \
    $$PWD/xcfbf.h \
    $$PWD/xcpio.h \
    $$PWD/xcompress.h \
    $$PWD/xdecompress.h \
    $$PWD/xcompresseddevice.h \
    $$PWD/xdeb.h \
    $$PWD/xdos16.h \
    $$PWD/xgzip.h \
    $$PWD/xipa.h \
    $$PWD/xiso9660.h \
    $$PWD/xjar.h \
    $$PWD/xlha.h \
    $$PWD/xmachofat.h \
    $$PWD/xrar.h \
    $$PWD/xsevenzip.h \
    $$PWD/xsquashfs.h \
    $$PWD/xtar.h \
    $$PWD/xtgz.h \
    $$PWD/xzip.h \
    $$PWD/xnpm.h \
    $$PWD/xszdd.h \
    $$PWD/xbzip2.h \
    $$PWD/xlzip.h \
    $$PWD/xxz.h \
    $$PWD/xzlib.h \
    $$PWD/xminidump.h \
    $$PWD/xdmg.h

SOURCES += \
    $$PWD/Algos/xlzhdecoder.cpp \
    $$PWD/Algos/xrardecoder.cpp \
    $$PWD/Algos/xit214decoder.cpp \
    $$PWD/Algos/xdeflatedecoder.cpp \
    $$PWD/Algos/ximplodedecoder.cpp \
    $$PWD/Algos/xlzmadecoder.cpp \
    $$PWD/Algos/xlzwdecoder.cpp \
    $$PWD/Algos/xascii85decoder.cpp \
    $$PWD/Algos/xstoredecoder.cpp \
    $$PWD/Algos/xbzip2decoder.cpp \
    $$PWD/Algos/xlzssdecoder.cpp \
    $$PWD/Algos/xshrinkdecoder.cpp \
    $$PWD/Algos/xreducedecoder.cpp \
    $$PWD/Algos/xzipcryptodecoder.cpp \
    $$PWD/Algos/xzipaesdecoder.cpp \
    $$PWD/Algos/xppmddecoder.cpp \
    $$PWD/Algos/xppmdrangedecoder.cpp \
    $$PWD/Algos/xppmdmodel.cpp \
    $$PWD/Algos/xppmd7model.cpp \
    $$PWD/Algos/xaesdecoder.cpp \
    $$PWD/Algos/xsha256decoder.cpp \
    $$PWD/x_ar.cpp \
    $$PWD/xapk.cpp \
    $$PWD/xapks.cpp \
    $$PWD/xarchive.cpp \
    $$PWD/xcab.cpp \
    $$PWD/xcfbf.cpp \
    $$PWD/xcpio.cpp \
    $$PWD/xcompress.cpp \
    $$PWD/xdecompress.cpp \
    $$PWD/xcompresseddevice.cpp \
    $$PWD/xdeb.cpp \
    $$PWD/xdos16.cpp \
    $$PWD/xgzip.cpp \
    $$PWD/xipa.cpp \
    $$PWD/xiso9660.cpp \
    $$PWD/xjar.cpp \
    $$PWD/xlha.cpp \
    $$PWD/xmachofat.cpp \
    $$PWD/xrar.cpp \
    $$PWD/xsevenzip.cpp \
    $$PWD/xsquashfs.cpp \
    $$PWD/xtar.cpp \
    $$PWD/xtgz.cpp \
    $$PWD/xzip.cpp \
    $$PWD/xnpm.cpp \
    $$PWD/xszdd.cpp \
    $$PWD/xbzip2.cpp \
    $$PWD/xlzip.cpp \
    $$PWD/xxz.cpp \
    $$PWD/xzlib.cpp \
    $$PWD/xminidump.cpp \
    $$PWD/xdmg.cpp

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

!contains(XCONFIG, xpyc) {
    XCONFIG += xpyc
    include($$PWD/../Formats/formats/xpyc.pri)
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

!contains(XCONFIG, ppmd) {
    XCONFIG += ppmd
    include($$PWD/3rdparty/ppmd/ppmd.pri)
}

DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/xarchive.cmake
