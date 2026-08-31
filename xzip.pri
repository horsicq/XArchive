# Archive-format core shared by Formats and XArchive: all classes relocated to
# Formats (XArchive, the ZIP family, GZIP, compressed-TAR/NPM formats, ISO9660,
# MACHOFat, DOS16, Deflate and store decoders), their TAR support,
# XCompress/XDecompress and ALL Algos decoders.
#
# This unit works WITHOUT the USE_ARCHIVE define. xarchive.pri composes it and
# adds the archive formats that remain in XArchive (rar/7z/tar/...) on top;
# USE_ARCHIVE keeps gating those remaining formats.

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/Algos
DEPENDPATH += $$PWD/Algos
INCLUDEPATH += $$PWD/Algos/include
DEPENDPATH += $$PWD/Algos/include
INCLUDEPATH += $$PWD/../Formats
DEPENDPATH += $$PWD/../Formats
INCLUDEPATH += $$PWD/../Formats/archives
DEPENDPATH += $$PWD/../Formats/archives
INCLUDEPATH += $$PWD/../Formats/Algos
DEPENDPATH += $$PWD/../Formats/Algos
INCLUDEPATH += $$PWD/../Formats/exec
DEPENDPATH += $$PWD/../Formats/exec

# XDecompress references the Ancient-backed and legacy Mac/PAX/Vise codecs
# even when the consumer only composes the ZIP core.
include($$PWD/ancient.pri)

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
    $$PWD/../Formats/Algos/xdeflatedecoder.h \
    $$PWD/Algos/ximplodedecoder.h \
    $$PWD/Algos/xlzmadecoder.h \
    $$PWD/Algos/xlzwdecoder.h \
    $$PWD/Algos/xascii85decoder.h \
    $$PWD/Algos/xasciihexdecoder.h \
    $$PWD/Algos/xrunlengthdecoder.h \
    $$PWD/../Formats/Algos/xstoredecoder.h \
    $$PWD/Algos/xspisrledecoder.h \
    $$PWD/Algos/xamigalzxdecoder.h \
    $$PWD/Algos/xmaclegacydecoders.h \
    $$PWD/Algos/xpaxdecoder.h \
    $$PWD/Algos/xvisedeflatedecoder.h \
    $$PWD/Algos/xmi10decoder.h \
    $$PWD/Algos/xfpakdecoder.h \
    $$PWD/Algos/xftcompdecoder.h \
    $$PWD/Algos/xdndecoder.h \
    $$PWD/Algos/xsqzdecoder.h \
    $$PWD/Algos/xflsdecoder.h \
    $$PWD/Algos/xpakdecoder.h \
    $$PWD/Algos/xssmdecoder.h \
    $$PWD/Algos/xrtpatchdecoder.h \
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
    $$PWD/../Formats/xarchive.h \
    $$PWD/xcompress.h \
    $$PWD/xdecompress.h \
    $$PWD/xcompresseddevice.h \
    $$PWD/../Formats/archives/xtar.h \
    $$PWD/../Formats/archives/xtarcompressed.h \
    $$PWD/../Formats/archives/xzip.h \
    $$PWD/../Formats/archives/xjar.h \
    $$PWD/../Formats/archives/xapk.h \
    $$PWD/../Formats/archives/xapks.h \
    $$PWD/../Formats/archives/xipa.h \
    $$PWD/../Formats/archives/xgzip.h \
    $$PWD/../Formats/archives/xiso9660.h \
    $$PWD/../Formats/archives/xtar_gz.h \
    $$PWD/../Formats/archives/xtar_compress.h \
    $$PWD/../Formats/archives/xnpm.h \
    $$PWD/../Formats/exec/xmachofat.h \
    $$PWD/../Formats/exec/xdos16.h

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
    $$PWD/../Formats/Algos/xdeflatedecoder.cpp \
    $$PWD/Algos/ximplodedecoder.cpp \
    $$PWD/Algos/xlzmadecoder.cpp \
    $$PWD/Algos/xlzwdecoder.cpp \
    $$PWD/Algos/xascii85decoder.cpp \
    $$PWD/Algos/xasciihexdecoder.cpp \
    $$PWD/Algos/xrunlengthdecoder.cpp \
    $$PWD/../Formats/Algos/xstoredecoder.cpp \
    $$PWD/Algos/xspisrledecoder.cpp \
    $$PWD/Algos/xamigalzxdecoder.cpp \
    $$PWD/Algos/xmaclegacydecoders.cpp \
    $$PWD/Algos/xpaxdecoder.cpp \
    $$PWD/Algos/xvisedeflatedecoder.cpp \
    $$PWD/Algos/xmi10decoder.cpp \
    $$PWD/Algos/xfpakdecoder.cpp \
    $$PWD/Algos/xftcompdecoder.cpp \
    $$PWD/Algos/xdndecoder.cpp \
    $$PWD/Algos/xsqzdecoder.cpp \
    $$PWD/Algos/xflsdecoder.cpp \
    $$PWD/Algos/xpakdecoder.cpp \
    $$PWD/Algos/xssmdecoder.cpp \
    $$PWD/Algos/xrtpatchdecoder.cpp \
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
    $$PWD/../Formats/xarchive.cpp \
    $$PWD/xcompress.cpp \
    $$PWD/xdecompress.cpp \
    $$PWD/xcompresseddevice.cpp \
    $$PWD/../Formats/archives/xtar.cpp \
    $$PWD/../Formats/archives/xtarcompressed.cpp \
    $$PWD/../Formats/archives/xzip.cpp \
    $$PWD/../Formats/archives/xjar.cpp \
    $$PWD/../Formats/archives/xapk.cpp \
    $$PWD/../Formats/archives/xapks.cpp \
    $$PWD/../Formats/archives/xipa.cpp \
    $$PWD/../Formats/archives/xgzip.cpp \
    $$PWD/../Formats/archives/xiso9660.cpp \
    $$PWD/../Formats/archives/xtar_gz.cpp \
    $$PWD/../Formats/archives/xtar_compress.cpp \
    $$PWD/../Formats/archives/xnpm.cpp \
    $$PWD/../Formats/exec/xmachofat.cpp \
    $$PWD/../Formats/exec/xdos16.cpp

!contains(XCONFIG, xbinary) {
    XCONFIG += xbinary
    include($$PWD/../Formats/xbinary.pri)
}

!contains(XCONFIG, xmach) {
    XCONFIG += xmach
    include($$PWD/../Formats/exec/xmach.pri) # XMACHOFat calls XMACH helpers
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
