# ZIP + decompression core: XZip (with its XJAR/XAPK/XAPKS/XIPA family), the
# XArchive base class, XCompress/XDecompress and ALL Algos decoders.
#
# This unit works WITHOUT the USE_ARCHIVE define: consumers that only need ZIP
# parsing and the Deflate/store/... decoders include this file instead of
# xarchive.pri. xarchive.pri composes this file and adds the other archive
# formats (rar/7z/tar/iso/...) on top; USE_ARCHIVE keeps gating those.

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/Algos
DEPENDPATH += $$PWD/Algos
INCLUDEPATH += $$PWD/Algos/include
DEPENDPATH += $$PWD/Algos/include

HEADERS += \
    $$PWD/Algos/algo_utils.h \
    $$PWD/Algos/xalgo_local.h \
    $$PWD/Algos/xarcdecoder.h \
    $$PWD/Algos/xarjdecoder.h \
    $$PWD/Algos/xacedecoder.h \
    $$PWD/Algos/xlzhdecoder.h \
    $$PWD/Algos/xkwajlzssdecoder.h \
    $$PWD/Algos/xkwajlzhdecoder.h \
    $$PWD/Algos/xrardecoder.h \
    $$PWD/Algos/xit214decoder.h \
    $$PWD/Algos/xdeflatedecoder.h \
    $$PWD/Algos/ximplodedecoder.h \
    $$PWD/Algos/xlzmadecoder.h \
    $$PWD/Algos/xlzwdecoder.h \
    $$PWD/Algos/xascii85decoder.h \
    $$PWD/Algos/xasciihexdecoder.h \
    $$PWD/Algos/xrunlengthdecoder.h \
    $$PWD/Algos/xstoredecoder.h \
    $$PWD/Algos/xbzip2decoder.h \
    $$PWD/Algos/xbrotlidecoder.h \
    $$PWD/Algos/xlzssdecoder.h \
    $$PWD/Algos/xcoktellzdecoder.h \
    $$PWD/Algos/xwinzipjpegdecoder.h \
    $$PWD/Algos/xwavpackdecoder.h \
    $$PWD/Algos/xshrinkdecoder.h \
    $$PWD/Algos/xreducedecoder.h \
    $$PWD/Algos/xzipcryptodecoder.h \
    $$PWD/Algos/xppmddecoder.h \
    $$PWD/Algos/xppmdrangedecoder.h \
    $$PWD/Algos/xppmdmodel.h \
    $$PWD/Algos/xppmd7model.h \
    $$PWD/Algos/xaesdecoder.h \
    $$PWD/Algos/xbcj2decoder.h \
    $$PWD/Algos/xbranchdecoder.h \
    $$PWD/Algos/xlzxdecoder.h \
    $$PWD/Algos/xquantumdecoder.h \
    $$PWD/Algos/xxpressdecoder.h \
    $$PWD/Algos/xsha256decoder.h \
    $$PWD/Algos/xblake2sp.h \
    $$PWD/Algos/xzstddecoder.h \
    $$PWD/Algos/xlz4decoder.h \
    $$PWD/Algos/xlz5decoder.h \
    $$PWD/Algos/xlizarddecoder.h \
    $$PWD/Algos/lz5lizarddeclib.h \
    $$PWD/Algos/lz4declib.h \
    $$PWD/Algos/include/wavpack.h \
    $$PWD/Algos/zstdlegacydeclib.h \
    $$PWD/Algos/xucldecoder.h \
    $$PWD/Algos/xucldecoder_acc.h \
    $$PWD/Algos/xlzodecoder.h \
    $$PWD/Algos/xcompressdecoder.h \
    $$PWD/xarchive.h \
    $$PWD/xcompress.h \
    $$PWD/xdecompress.h \
    $$PWD/xcompresseddevice.h \
    $$PWD/xzip.h \
    $$PWD/xjar.h \
    $$PWD/xapk.h \
    $$PWD/xapks.h \
    $$PWD/xipa.h

SOURCES += \
    $$PWD/Algos/algo_utils.cpp \
    $$PWD/Algos/xarcdecoder.cpp \
    $$PWD/Algos/xarjdecoder.cpp \
    $$PWD/Algos/xacedecoder.cpp \
    $$PWD/Algos/xlzhdecoder.cpp \
    $$PWD/Algos/xkwajlzssdecoder.cpp \
    $$PWD/Algos/xkwajlzhdecoder.cpp \
    $$PWD/Algos/xrardecoder.cpp \
    $$PWD/Algos/xit214decoder.cpp \
    $$PWD/Algos/xdeflatedecoder.cpp \
    $$PWD/Algos/ximplodedecoder.cpp \
    $$PWD/Algos/xlzmadecoder.cpp \
    $$PWD/Algos/xlzwdecoder.cpp \
    $$PWD/Algos/xascii85decoder.cpp \
    $$PWD/Algos/xasciihexdecoder.cpp \
    $$PWD/Algos/xrunlengthdecoder.cpp \
    $$PWD/Algos/xstoredecoder.cpp \
    $$PWD/Algos/xbzip2decoder.cpp \
    $$PWD/Algos/xbrotlidecoder.cpp \
    $$PWD/Algos/xlzssdecoder.cpp \
    $$PWD/Algos/xcoktellzdecoder.cpp \
    $$PWD/Algos/xwinzipjpegdecoder.cpp \
    $$PWD/Algos/xwavpackdecoder.cpp \
    $$PWD/Algos/xshrinkdecoder.cpp \
    $$PWD/Algos/xreducedecoder.cpp \
    $$PWD/Algos/xzipcryptodecoder.cpp \
    $$PWD/Algos/xppmddecoder.cpp \
    $$PWD/Algos/xppmdrangedecoder.cpp \
    $$PWD/Algos/xppmdmodel.cpp \
    $$PWD/Algos/xppmd7model.cpp \
    $$PWD/Algos/xaesdecoder.cpp \
    $$PWD/Algos/xbcj2decoder.cpp \
    $$PWD/Algos/xbranchdecoder.cpp \
    $$PWD/Algos/xlzxdecoder.cpp \
    $$PWD/Algos/xquantumdecoder.cpp \
    $$PWD/Algos/xxpressdecoder.cpp \
    $$PWD/Algos/xsha256decoder.cpp \
    $$PWD/Algos/xblake2sp.cpp \
    $$PWD/Algos/xzstddecoder.cpp \
    $$PWD/Algos/xlz4decoder.cpp \
    $$PWD/Algos/xlz5decoder.cpp \
    $$PWD/Algos/xlizarddecoder.cpp \
    $$PWD/Algos/xucldecoder.cpp \
    $$PWD/Algos/xlzodecoder.cpp \
    $$PWD/Algos/xcompressdecoder.cpp \
    $$PWD/Algos/lz4declib.cpp \
    $$PWD/Algos/wavpackdeclib.cpp \
    $$PWD/Algos/lz5lizarddeclib.cpp \
    $$PWD/xarchive.cpp \
    $$PWD/xcompress.cpp \
    $$PWD/xdecompress.cpp \
    $$PWD/xcompresseddevice.cpp \
    $$PWD/xzip.cpp \
    $$PWD/xjar.cpp \
    $$PWD/xapk.cpp \
    $$PWD/xapks.cpp \
    $$PWD/xipa.cpp

!contains(XCONFIG, xbinary) {
    XCONFIG += xbinary
    include($$PWD/../Formats/xbinary.pri)
}

!contains(XCONFIG, xoptions) {
    XCONFIG += xoptions
    include($$PWD/../XOptions/xoptions.pri) # XDecompress is an XThreadObject
}

!contains(XCONFIG, xpng) {
    XCONFIG += xpng
    include($$PWD/../Formats/images/xpng.pri) # XDecompress builds PNG previews
}

!contains(XCONFIG, xjavaclass) {
    XCONFIG += xjavaclass
    include($$PWD/../Formats/formats/xjavaclass.pri) # XJAR record detection
}

# XAPK needs the AXML parser; xdex.pri guards its copy with the same flag.
!contains(XCONFIG, xandroidbinary_sources) {
    XCONFIG += xandroidbinary_sources
    INCLUDEPATH += $$PWD/../XDEX
    DEPENDPATH += $$PWD/../XDEX
    HEADERS += \
        $$PWD/../XDEX/xandroidbinary.h \
        $$PWD/../XDEX/xandroidbinary_def.h
    SOURCES += \
        $$PWD/../XDEX/xandroidbinary.cpp
}

# The Algos amalgamations below -- zlib, the zstd decoders, bzip2, lzma, ppmd --
# were converted from C to C++ on 2026-08-17 and must stay C++ in this build
# too. Several of their internal symbols are declared in no header, so their
# linkage names follow the language each unit is compiled as; compiling one
# here as C while CMake builds it as C++ fails at link with unresolved
# externals, not at compile. Each file carries an #ifndef __cplusplus / #error
# guard so it fails at compile instead.

# zlib is built from the Algos amalgamations rather than a prebuilt library, so
# the qmake and CMake builds compile the same sources.
!contains(XCONFIG, zlib) {
    XCONFIG += zlib
    INCLUDEPATH += $$PWD/Algos/include
    DEPENDPATH += $$PWD/Algos/include
    SOURCES += \
        $$PWD/Algos/zlibutil.cpp \
        $$PWD/Algos/zlibdeclib.cpp \
        $$PWD/Algos/zlibenclib.cpp
}

# Guarded: other modules(XDWARF, XUpdate) pull the same single-file zstd decoder
!contains(XCONFIG, zstddeclib) {
    XCONFIG += zstddeclib
    SOURCES += $$PWD/Algos/zstddeclib.cpp
}

# Versioned Zstandard symbols and namespaced xxHash are independent of the
# guarded single-file current-format decoder above. Keep a separate guard so
# projects that included zstddeclib through another module still receive the
# legacy stream implementation needed by XArchive.
!contains(XCONFIG, xarchive_zstd_legacy_decode) {
    XCONFIG += xarchive_zstd_legacy_decode
    SOURCES += \
        $$PWD/Algos/zstd_xxhash.cpp \
        $$PWD/Algos/zstdlegacy_v04.cpp \
        $$PWD/Algos/zstdlegacy_v05.cpp \
        $$PWD/Algos/zstdlegacy_v06.cpp \
        $$PWD/Algos/zstdlegacy_v07.cpp
}

!contains(XCONFIG, bzip2) {
    XCONFIG += bzip2
    INCLUDEPATH += $$PWD/Algos/include
    DEPENDPATH += $$PWD/Algos/include
    SOURCES += $$PWD/Algos/bzip2declib.cpp
}

!contains(XCONFIG, lzma) {
    XCONFIG += lzma
    INCLUDEPATH += $$PWD/Algos $$PWD/Algos/include
    DEPENDPATH += $$PWD/Algos
    SOURCES += \
        $$PWD/Algos/sevenzip_extcodec_lzmadec.cpp \
        $$PWD/Algos/sevenzip_extcodec_lzma2dec.cpp
}

!contains(XCONFIG, ppmd) {
    XCONFIG += ppmd
    INCLUDEPATH += $$PWD/Algos $$PWD/Algos/include
    DEPENDPATH += $$PWD/Algos
    SOURCES += \
        $$PWD/Algos/sevenzip_extcodec_ppmd7.cpp \
        $$PWD/Algos/sevenzip_extcodec_ppmd7dec.cpp \
        $$PWD/Algos/sevenzip_extcodec_ppmd8.cpp \
        $$PWD/Algos/sevenzip_extcodec_ppmd8dec.cpp
}

DISTFILES += \
    $$PWD/xzip.cmake
