# ZIP + decompression core: XZip (with its XJAR/XAPK/XAPKS/XIPA family), the
# XArchive base class, XCompress/XDecompress and ALL Algos decoders.
#
# This unit works WITHOUT the USE_ARCHIVE define: consumers that only need ZIP
# parsing and the Deflate/store/... decoders include this file instead of
# xarchive.cmake. xarchive.cmake composes this file and adds the other archive
# formats (rar/7z/tar/iso/...) on top; USE_ARCHIVE keeps gating those.
#
# Link requirements: the decoder amalgamations reference the zlib / bzip2 /
# lzma / ppmd targets defined in algos_codecs.cmake -- a standalone consumer
# must include(algos_codecs.cmake) and link those targets (an xformats/
# xarchive consumer already does).
#
# Shared sources: xdecompress.cpp needs XPNG, xjar needs XJavaClass and xapk
# needs XAndroidBinary. When this file is reached through xformats.cmake /
# xdex.cmake those sources are already in the build (XFORMATS_SOURCES /
# XDEX_SOURCES are defined) and are not added again here.

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/Algos/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/Algos/include/)

if (NOT DEFINED XBINARY_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../Formats/xbinary.cmake)
    set(XZIP_SOURCES ${XZIP_SOURCES} ${XBINARY_SOURCES})
endif()
# xbinary.h includes the format *_def.h headers from Formats/exec
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/exec)

if (NOT DEFINED XOPTIONS_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../XOptions/xoptions.cmake)
    set(XZIP_SOURCES ${XZIP_SOURCES} ${XOPTIONS_SOURCES})
endif()

if (NOT DEFINED XFORMATS_SOURCES)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/images)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/formats)
    set(XZIP_SOURCES
        ${XZIP_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/../Formats/images/xpng.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../Formats/images/xpng.h
        ${CMAKE_CURRENT_LIST_DIR}/../Formats/formats/xjavaclass.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../Formats/formats/xjavaclass.h
    )
endif()

if (NOT DEFINED XDEX_SOURCES)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/../XDEX)
    set(XZIP_SOURCES
        ${XZIP_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/../XDEX/xandroidbinary.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../XDEX/xandroidbinary.h
    )
endif()

set(XZIP_SOURCES
    ${XZIP_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/xarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xcompress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcompress.h
    ${CMAKE_CURRENT_LIST_DIR}/xdecompress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xdecompress.h
    ${CMAKE_CURRENT_LIST_DIR}/xcompresseddevice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcompresseddevice.h
    ${CMAKE_CURRENT_LIST_DIR}/xzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xzip.h
    ${CMAKE_CURRENT_LIST_DIR}/xjar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xjar.h
    ${CMAKE_CURRENT_LIST_DIR}/xapk.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xapk.h
    ${CMAKE_CURRENT_LIST_DIR}/xapks.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xapks.h
    ${CMAKE_CURRENT_LIST_DIR}/xipa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xipa.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/algo_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/algo_utils.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xalgo_local.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarcdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarcdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarjdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarjdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xacedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xacedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xucldecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xucldecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xucldecoder_acc.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzhdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzhdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzssdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzssdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzhdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzhdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrardecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrardecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xit214decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xit214decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdeflatedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdeflatedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/ximplodedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/ximplodedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzmadecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzmadecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzwdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzwdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xascii85decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xascii85decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xasciihexdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xasciihexdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrunlengthdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrunlengthdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xstoredecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xstoredecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xamigalzxdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xamigalzxdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbzip2decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbzip2decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbrotlidecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbrotlidecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzssdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzssdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xcoktellzdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xcoktellzdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xwinzipjpegdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xwinzipjpegdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xwavpackdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xwavpackdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xshrinkdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xshrinkdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xreducedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xreducedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xzipcryptodecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xzipcryptodecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmddecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmddecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmdrangedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmdrangedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmdmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmdmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmd7model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xppmd7model.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xaesdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xaesdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbcj2decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbcj2decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbranchdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbranchdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzxdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzxdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xquantumdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xquantumdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xxpressdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xxpressdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsha256decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsha256decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xblake2sp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xblake2sp.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xzstddecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xzstddecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlz4decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlz4decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlz5decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlz5decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlizarddecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlizarddecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/lz5lizarddeclib.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/lz4declib.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/include/wavpack.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/zstdlegacydeclib.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/zstddeclib.cpp
    # The five entries below were converted from C to C++ on 2026-08-17 and must
    # stay C++: they define ZSTDv05/06/07_* and ZBUFFv07_* symbols that no header
    # declares, so a C build of one and a C++ build of its caller fail at link,
    # not at compile. Each file carries an #ifndef __cplusplus / #error guard.
    ${CMAKE_CURRENT_LIST_DIR}/Algos/zstd_xxhash.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/zstdlegacy_v04.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/zstdlegacy_v05.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/zstdlegacy_v06.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/zstdlegacy_v07.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzodecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzodecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xcompressdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xcompressdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/lz4declib.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/wavpackdeclib.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/lz5lizarddeclib.cpp
)
