include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/Algos/)
# The 7-Zip C public headers (LzmaDec.h, Ppmd7.h, 7zTypes.h, ...) live in
# Algos/include, kept apart from the first-party and generated sources.
# Consumers outside XArchive -- XStaticUnpacker's xnsis.cpp and xmew.cpp
# include LzmaDec.h -- resolve them from here without the vendored tree.
include_directories(${CMAKE_CURRENT_LIST_DIR}/Algos/include/)
# bzip2 headers (bzlib.h) moved to Algos/include with its sources; that path
# is already on the include list above.
# zlib headers moved to Algos/include with its sources; that path is already
# on the include list above.

# The ZIP + decompression core (XZip family, XArchive base, XCompress/
# XDecompress, ALL Algos decoders, and xbinary/xoptions) lives in xzip.cmake so
# it can be used WITHOUT the USE_ARCHIVE define; this file adds the remaining
# archive formats on top.
if (NOT DEFINED XZIP_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/xzip.cmake)
    set(XARCHIVE_SOURCES ${XARCHIVE_SOURCES} ${XZIP_SOURCES})
endif()

#include(${CMAKE_CURRENT_LIST_DIR}/../Formats/exec/xmach.cmake)

set(XARCHIVE_SOURCES
    ${XARCHIVE_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/x_ar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/x_ar.h
    ${CMAKE_CURRENT_LIST_DIR}/xseaarc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xseaarc.h
    ${CMAKE_CURRENT_LIST_DIR}/xexternalarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xexternalarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xfreearc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xfreearc.h
    ${CMAKE_CURRENT_LIST_DIR}/xzpaq.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xzpaq.h
    ${CMAKE_CURRENT_LIST_DIR}/xbcm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xbcm.h
    ${CMAKE_CURRENT_LIST_DIR}/xlpaq8.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlpaq8.h
    ${CMAKE_CURRENT_LIST_DIR}/xpea.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xpea.h
    ${CMAKE_CURRENT_LIST_DIR}/xarj.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xarj.h
    ${CMAKE_CURRENT_LIST_DIR}/xace.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xace.h
    ${CMAKE_CURRENT_LIST_DIR}/xfilteredarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xfilteredarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xcab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcab.h
    ${CMAKE_CURRENT_LIST_DIR}/xcfbf.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcfbf.h
    ${CMAKE_CURRENT_LIST_DIR}/xcpio.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcpio.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xpak.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xpak.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xwad.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xwad.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xgrp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xgrp.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xgamestorearchive_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xgamestorearchive_p.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xckp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xckp.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xedp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xedp.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xmpq.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xmpq.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xbigf.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xbigf.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xparsecarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xparsecarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xpmm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xpmm.h
    ${CMAKE_CURRENT_LIST_DIR}/xwarc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xwarc.h
    ${CMAKE_CURRENT_LIST_DIR}/xmtree.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xmtree.h
    ${CMAKE_CURRENT_LIST_DIR}/xuu.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xuu.h
    ${CMAKE_CURRENT_LIST_DIR}/xdeb.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xdeb.h
    ${CMAKE_CURRENT_LIST_DIR}/xgzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xgzip.h
    ${CMAKE_CURRENT_LIST_DIR}/xiso9660.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xiso9660.h
    ${CMAKE_CURRENT_LIST_DIR}/xudf.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xudf.h
    ${CMAKE_CURRENT_LIST_DIR}/xwim.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xwim.h
    ${CMAKE_CURRENT_LIST_DIR}/xrpm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xrpm.h
    ${CMAKE_CURRENT_LIST_DIR}/xkwaj.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xkwaj.h
    ${CMAKE_CURRENT_LIST_DIR}/xasar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xasar.h
    ${CMAKE_CURRENT_LIST_DIR}/xxar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xxar.h
    ${CMAKE_CURRENT_LIST_DIR}/xzoo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xzoo.h
    ${CMAKE_CURRENT_LIST_DIR}/xstk.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xstk.h
    ${CMAKE_CURRENT_LIST_DIR}/xlha.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xsar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xsar.h
    ${CMAKE_CURRENT_LIST_DIR}/xarx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xarx.h
    ${CMAKE_CURRENT_LIST_DIR}/xlha.h
    ${CMAKE_CURRENT_LIST_DIR}/xmachofat.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xmachofat.h
    ${CMAKE_CURRENT_LIST_DIR}/xrar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xrar.h
    ${CMAKE_CURRENT_LIST_DIR}/xsevenzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xsevenzip.h
    ${CMAKE_CURRENT_LIST_DIR}/xsquashfs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xsquashfs.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_bzip2.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_bzip2.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_compress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_compress.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_gz.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_gz.h
    ${CMAKE_CURRENT_LIST_DIR}/xtarcompressed.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtarcompressed.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lzip.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lzma.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lzma.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lzop.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lzop.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_xz.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_xz.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_zstd.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_zstd.h
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lz4.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xtar_lz4.h
    ${CMAKE_CURRENT_LIST_DIR}/xzstd.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xzstd.h
    ${CMAKE_CURRENT_LIST_DIR}/xlz4.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlz4.h
    ${CMAKE_CURRENT_LIST_DIR}/xlz5.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlz5.h
    ${CMAKE_CURRENT_LIST_DIR}/xlizard.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlizard.h
    ${CMAKE_CURRENT_LIST_DIR}/xlzma.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlzma.h
    ${CMAKE_CURRENT_LIST_DIR}/xlzo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlzo.h
    ${CMAKE_CURRENT_LIST_DIR}/xcompressz.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcompressz.h
    ${CMAKE_CURRENT_LIST_DIR}/xzlib.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xzlib.h
    ${CMAKE_CURRENT_LIST_DIR}/xnpm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xnpm.h
    ${CMAKE_CURRENT_LIST_DIR}/xdos16.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xdos16.h
    ${CMAKE_CURRENT_LIST_DIR}/xszdd.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xszdd.h
    ${CMAKE_CURRENT_LIST_DIR}/xbzip2.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xbzip2.h
    ${CMAKE_CURRENT_LIST_DIR}/xbrotli.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xbrotli.h
    ${CMAKE_CURRENT_LIST_DIR}/xlzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlzip.h
    ${CMAKE_CURRENT_LIST_DIR}/xxz.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xxz.h
    ${CMAKE_CURRENT_LIST_DIR}/xminidump.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xminidump.h
    ${CMAKE_CURRENT_LIST_DIR}/xdmg.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xdmg.h
)
