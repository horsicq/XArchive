INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/Algos
DEPENDPATH += $$PWD/Algos
INCLUDEPATH += $$PWD/Algos/include
DEPENDPATH += $$PWD/Algos/include

# 7-Zip 26.01 all-format in-process archive core.
# Upstream tag commit: 8c63d71ff886bda90c86db28466287f977374237.
#
# The six LZMA/PPMd decoder sources deliberately remain in lzma.pri and
# ppmd.pri. The upstream tree has been folded into the Algos amalgamations,
# so this list mirrors the xarchive_7zip CMake OBJECT target file-for-file
# and still keeps each decoder symbol compiled exactly once.
#
# szc_01/02/03 must stay C++; see the note above the zlib block below.

!contains(XCONFIG, xarchive_7zip_2601_sources) {
    XCONFIG += xarchive_7zip_2601_sources




    INCLUDEPATH += $$PWD/Algos
    DEPENDPATH += $$PWD/Algos

    DEFINES += UNICODE _UNICODE NOMINMAX
    win32: DEFINES += _CRT_SECURE_NO_WARNINGS

    XARCHIVE_7ZIP_SOURCES = \
        $$PWD/Algos/szc_03.cpp \
        $$PWD/Algos/szc_02.cpp \
        $$PWD/Algos/szc_01.cpp \
        $$PWD/Algos/szhash_07.cpp \
        $$PWD/Algos/szhash_06.cpp \
        $$PWD/Algos/szhash_05.cpp \
        $$PWD/Algos/szhash_04.cpp \
        $$PWD/Algos/szhash_03.cpp \
        $$PWD/Algos/szhash_02.cpp \
        $$PWD/Algos/szhash_01.cpp \
        $$PWD/Algos/szcr_05.cpp \
        $$PWD/Algos/szcr_04.cpp \
        $$PWD/Algos/szcr_03.cpp \
        $$PWD/Algos/szcr_02.cpp \
        $$PWD/Algos/szcr_01.cpp \
        $$PWD/Algos/szznr_04.cpp \
        $$PWD/Algos/szznr_03.cpp \
        $$PWD/Algos/szznr_02.cpp \
        $$PWD/Algos/szznr_01.cpp \
        $$PWD/Algos/sz7z_03.cpp \
        $$PWD/Algos/sz7z_02.cpp \
        $$PWD/Algos/sz7z_01.cpp \
        $$PWD/Algos/szaf_08.cpp \
        $$PWD/Algos/szaf_07.cpp \
        $$PWD/Algos/szaf_06.cpp \
        $$PWD/Algos/szaf_05.cpp \
        $$PWD/Algos/szaf_04.cpp \
        $$PWD/Algos/szaf_03.cpp \
        $$PWD/Algos/szaf_02.cpp \
        $$PWD/Algos/szaf_01.cpp \
        $$PWD/Algos/sevenzip_arch_wim.cpp \
        $$PWD/Algos/sevenzip_arch_udf.cpp \
        $$PWD/Algos/sevenzip_arch_tar.cpp \
        $$PWD/Algos/sevenzip_arch_rar.cpp \
        $$PWD/Algos/sevenzip_arch_iso.cpp \
        $$PWD/Algos/sevenzip_arch_common.cpp \
        $$PWD/Algos/sevenzip_arch_chm.cpp \
        $$PWD/Algos/sevenzip_arch_cab.cpp \
        $$PWD/Algos/sevenzip_crypto.cpp \
        $$PWD/Algos/sevenzip_compress.cpp \
        $$PWD/Algos/sevenzip_7zcommon.cpp \
        $$PWD/Algos/sevenzip_common.cpp

    # 42 amalgamated units, matching XARCHIVE_7ZIP_EXPECTED_SOURCE_COUNT in
    # CMakeLists.txt. Was 275 (one per upstream translation unit) before the
    # amalgamation; the CMake twin was updated and this one was missed, which
    # made every qmake project that reaches this file fail hard.
    !count(XARCHIVE_7ZIP_SOURCES, 42) {
        error(7-Zip 26.01 source manifest must contain exactly 42 files)
    }

    for(XARCHIVE_7ZIP_SOURCE, XARCHIVE_7ZIP_SOURCES) {
        !exists($$XARCHIVE_7ZIP_SOURCE) {
            error(7-Zip source is missing: $$XARCHIVE_7ZIP_SOURCE)
        }
    }

    SOURCES += $$XARCHIVE_7ZIP_SOURCES

    win32-msvc*: LIBS += oleaut32.lib ole32.lib user32.lib advapi32.lib shell32.lib
    win32-g++: LIBS += -loleaut32 -lole32 -luser32 -ladvapi32 -lshell32
}

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
    $$PWD/Algos/xxpressdecoder.h \
    $$PWD/Algos/xsha256decoder.h \
    $$PWD/Algos/xblake2sp.h \
    $$PWD/Algos/xzstddecoder.h \
    $$PWD/Algos/xlz4decoder.h \
    $$PWD/Algos/xlz5decoder.h \
    $$PWD/Algos/xlizarddecoder.h \
    $$PWD/Algos/lz5lizarddeclib.h \
    $$PWD/Algos/lz4declib.h \
    $$PWD/Algos/zstdlegacydeclib.h \
    $$PWD/Algos/xucldecoder.h \
    $$PWD/Algos/xucldecoder_acc.h \
    $$PWD/Algos/xlzodecoder.h \
    $$PWD/Algos/xcompressdecoder.h \
    $$PWD/x_ar.h \
    $$PWD/xapk.h \
    $$PWD/xapks.h \
    $$PWD/xseaarc.h \
    $$PWD/xfreearc.h \
    $$PWD/xarj.h \
    $$PWD/xace.h \
    $$PWD/xarchive.h \
    $$PWD/xfilteredarchive.h \
    $$PWD/xcab.h \
    $$PWD/xcfbf.h \
    $$PWD/xcpio.h \
    $$PWD/xgamearchive.h \
    $$PWD/xwarc.h \
    $$PWD/xmtree.h \
    $$PWD/xuu.h \
    $$PWD/xcompress.h \
    $$PWD/xdecompress.h \
    $$PWD/xcompresseddevice.h \
    $$PWD/xdeb.h \
    $$PWD/xdos16.h \
    $$PWD/xgzip.h \
    $$PWD/xipa.h \
    $$PWD/xiso9660.h \
    $$PWD/xudf.h \
    $$PWD/xwim.h \
    $$PWD/xrpm.h \
    $$PWD/xkwaj.h \
    $$PWD/xasar.h \
    $$PWD/xxar.h \
    $$PWD/xzoo.h \
    $$PWD/xjar.h \
    $$PWD/xlha.h \
    $$PWD/xsar.h \
    $$PWD/xarx.h \
    $$PWD/xmachofat.h \
    $$PWD/xrar.h \
    $$PWD/xsevenzip.h \
    $$PWD/xsquashfs.h \
    $$PWD/xtar.h \
    $$PWD/xtarcompressed.h \
    $$PWD/xtar_gz.h \
    $$PWD/xtar_bzip2.h \
    $$PWD/xtar_lzip.h \
    $$PWD/xtar_lzma.h \
    $$PWD/xtar_lzop.h \
    $$PWD/xtar_xz.h \
    $$PWD/xtar_zstd.h \
    $$PWD/xtar_lz4.h \
    $$PWD/xtar_compress.h \
    $$PWD/xzip.h \
    $$PWD/xnpm.h \
    $$PWD/xszdd.h \
    $$PWD/xbzip2.h \
    $$PWD/xbrotli.h \
    $$PWD/xlzip.h \
    $$PWD/xxz.h \
    $$PWD/xzlib.h \
    $$PWD/xzstd.h \
    $$PWD/xlz4.h \
    $$PWD/xlz5.h \
    $$PWD/xlizard.h \
    $$PWD/xlzma.h \
    $$PWD/xlzo.h \
    $$PWD/xcompressz.h \
    $$PWD/xminidump.h \
    $$PWD/xdmg.h

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
    $$PWD/x_ar.cpp \
    $$PWD/xapk.cpp \
    $$PWD/xapks.cpp \
    $$PWD/xseaarc.cpp \
    $$PWD/xfreearc.cpp \
    $$PWD/xarj.cpp \
    $$PWD/xace.cpp \
    $$PWD/xarchive.cpp \
    $$PWD/xfilteredarchive.cpp \
    $$PWD/xcab.cpp \
    $$PWD/xcfbf.cpp \
    $$PWD/xcpio.cpp \
    $$PWD/xgamearchive.cpp \
    $$PWD/xwarc.cpp \
    $$PWD/xmtree.cpp \
    $$PWD/xuu.cpp \
    $$PWD/xcompress.cpp \
    $$PWD/xdecompress.cpp \
    $$PWD/xcompresseddevice.cpp \
    $$PWD/xdeb.cpp \
    $$PWD/xdos16.cpp \
    $$PWD/xgzip.cpp \
    $$PWD/xipa.cpp \
    $$PWD/xiso9660.cpp \
    $$PWD/xudf.cpp \
    $$PWD/xwim.cpp \
    $$PWD/xrpm.cpp \
    $$PWD/xkwaj.cpp \
    $$PWD/xasar.cpp \
    $$PWD/xxar.cpp \
    $$PWD/xzoo.cpp \
    $$PWD/xjar.cpp \
    $$PWD/xlha.cpp \
    $$PWD/xsar.cpp \
    $$PWD/xarx.cpp \
    $$PWD/xmachofat.cpp \
    $$PWD/xrar.cpp \
    $$PWD/xsevenzip.cpp \
    $$PWD/xarchives_ip7z.cpp \
    $$PWD/xsquashfs.cpp \
    $$PWD/xtar.cpp \
    $$PWD/xtarcompressed.cpp \
    $$PWD/xtar_gz.cpp \
    $$PWD/xtar_bzip2.cpp \
    $$PWD/xtar_lzip.cpp \
    $$PWD/xtar_lzma.cpp \
    $$PWD/xtar_lzop.cpp \
    $$PWD/xtar_xz.cpp \
    $$PWD/xtar_zstd.cpp \
    $$PWD/xtar_lz4.cpp \
    $$PWD/xtar_compress.cpp \
    $$PWD/xzip.cpp \
    $$PWD/xnpm.cpp \
    $$PWD/xszdd.cpp \
    $$PWD/xbzip2.cpp \
    $$PWD/xbrotli.cpp \
    $$PWD/xlzip.cpp \
    $$PWD/xxz.cpp \
    $$PWD/xzlib.cpp \
    $$PWD/xzstd.cpp \
    $$PWD/xlz4.cpp \
    $$PWD/xlz5.cpp \
    $$PWD/xlizard.cpp \
    $$PWD/xlzma.cpp \
    $$PWD/xlzo.cpp \
    $$PWD/xcompressz.cpp \
    $$PWD/xminidump.cpp \
    $$PWD/xdmg.cpp

SOURCES += \
    $$PWD/Algos/lz4declib.cpp \
    $$PWD/Algos/lz5lizarddeclib.cpp

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

# The Algos amalgamations listed from here down -- zlib, the legacy zstd
# decoders, bzip2, lzma, ppmd, and the szc_* units above -- were converted from
# C to C++ on 2026-08-17 and must stay C++ in this build too. Several of their
# internal symbols are declared in no header, so their linkage names follow the
# language each unit is compiled as; compiling one here as C while CMake builds
# it as C++ fails at link with unresolved externals, not at compile. Each file
# carries an #ifndef __cplusplus / #error guard so it fails at compile instead.

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
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/xarchive.cmake
