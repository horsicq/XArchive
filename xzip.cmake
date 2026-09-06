# Archive-format core shared by Formats and XArchive: all classes relocated to
# Formats (XArchive, the ZIP family, GZIP, compressed-TAR/NPM formats, ISO9660,
# MACHOFat, DOS16, Deflate and store decoders), their TAR support,
# XCompress/XDecompress and ALL Algos decoders.
#
# This unit works WITHOUT the USE_ARCHIVE define. xarchive.cmake composes it
# and adds the archive formats that remain in XArchive (rar/7z/tar/...) on top;
# USE_ARCHIVE keeps gating those remaining formats.
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
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/archives)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/Algos)

# XDecompress dispatches these codecs even for ZIP-only consumers, so their
# implementations belong to the ZIP/decompression core rather than only to
# the wider archive-format layer.
include("${CMAKE_CURRENT_LIST_DIR}/ancient.cmake")

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

# XMACHOFat calls non-inline helpers from XMACH. Full XFormats consumers own
# those sources already; direct core/XArchive consumers need them composed here.
if (NOT DEFINED XFORMATS_SOURCES)
    if (NOT DEFINED XMACH_SOURCES)
        include(${CMAKE_CURRENT_LIST_DIR}/../Formats/exec/xmach.cmake)
    endif()
    set(XZIP_SOURCES ${XZIP_SOURCES} ${XMACH_SOURCES})
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
    ${XARCHIVE_ANCIENT_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/xarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/xarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xcompress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcompress.h
    ${CMAKE_CURRENT_LIST_DIR}/xdecompress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xdecompress.h
    # Decoders dispatched from xdecompress.cpp: they must live in the shared core
    # (this file), not in xarchive.cmake, or a USE_ARCHIVE-less consumer such as
    # xbinaryviewerc fails to link (LNK2019 XAldusDecoder/XAMPKDecoder/...).
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xaldusdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xaldusdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xampkdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xampkdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarcv2decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarcv2decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xasymetrixdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xasymetrixdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xborlandpackdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xborlandpackdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbzip1decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbzip1decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbsndecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbsndecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbthpakdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xbthpakdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdcldecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdcldecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/xcompresseddevice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcompresseddevice.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtar.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtarcompressed.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtarcompressed.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xzip.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xjar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xjar.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xapk.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xapk.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xapks.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xapks.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xipa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xipa.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xgzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xgzip.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xiso9660.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xiso9660.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtar_gz.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtar_gz.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtar_compress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xtar_compress.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xnpm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/archives/xnpm.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/exec/xmachofat.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/exec/xmachofat.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/exec/xdos16.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/exec/xdos16.h
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
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlha_legacy_lzs_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlha_legacy_lz5_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlha_legacy_lhx_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlha_legacy_lk7_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlha_legacy_pm1_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlha_legacy_pm2_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzhdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzhdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlha_legacy_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzssdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzssdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzhdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkwajlzhdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrardecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrardecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xit214decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xit214decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/Algos/xdeflatedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/Algos/xdeflatedecoder.h
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
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/Algos/xstoredecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Formats/Algos/xstoredecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xspisrledecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xspisrledecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xamigalzxdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xamigalzxdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmaclegacydecoders.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmaclegacydecoders.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpaxdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpaxdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xvisedeflatedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xvisedeflatedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmi10decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmi10decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xfpakdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xfpakdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xftcompdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xftcompdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdndecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdndecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsqzdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsqzdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xflsdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xflsdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpakdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpakdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xssmdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xssmdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrtpatchdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrtpatchdecoder.h
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
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkolibrikpackdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmathcaddecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpcommos2decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xnetwarepackdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xearefpackdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzpis2decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xnpackdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xcorelltecdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xirwinpacdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xgashuffdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsilmarilsdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrawlzw15vdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xriddecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrompaqdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarcv4decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xealzwdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xslsdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpcsecuredecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xqnxbasedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhuffdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzhcxpdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdsquantumdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpktdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhdcopydecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xstylusdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsettlersftdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsqdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xis11decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpaperportdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xealibdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xniddecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhapdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzdietdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzv1decoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsafdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhfedecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrsvkdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhzldecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlofidecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xinfogramespakdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/lz4declib.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/wavpackdeclib.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/lz5lizarddeclib.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xkolibrikpackdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmathcaddecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpcommos2decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xnetwarepackdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xearefpackdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzpis2decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xnpackdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xcorelltecdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xirwinpacdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xgashuffdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsilmarilsdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrawlzw15vdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xriddecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrompaqdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xarcv4decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xealzwdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xslsdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpcsecuredecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xqnxbasedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhuffdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzhcxpdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdsquantumdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpktdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhdcopydecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xstylusdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsettlersftdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsqdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xis11decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xpaperportdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xealibdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xniddecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhapdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzdietdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlzv1decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xsafdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhfedecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xrsvkdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xhzldecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xlofidecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xinfogramespakdecoder.cpp
)
