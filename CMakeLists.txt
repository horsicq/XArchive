cmake_minimum_required(VERSION 3.14)

project(XArchive)

# Keep the direct-module source export in sync with xarchive.cmake.  The game
# formats are stored containers and add no codec library or external dependency.
set(XARCHIVE_SOURCES
    ${XARCHIVE_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/games/xpak.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xpak.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xwad.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xwad.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xgrp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xgrp.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xgodotpck.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xgodotpck.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xwbfsarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xwbfsarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/games/xrvzarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/games/xrvzarchive.h
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
)

# bzip2 / zlib / lzma / ppmd have no library targets any more. algos_codecs.cmake
# declared them, and every consumer had to include() that file AND name all four
# in target_link_libraries; miss either half and the bare name became a raw
# linker input that failed at link on "inflate". Their Algos amalgamations are
# now listed in the ZIP core in Formats/xformats.cmake and compile straight into
# whatever target consumes XZIP_SOURCES, the way the qmake build always did.

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared library" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static library" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Build zstd tests" FORCE)
# zstd_static is not built here any more. A symbol-level check showed the Algos
# zstd objects (zstddeclib.cpp, zstd_xxhash.cpp, zstdlegacy_v04-v07.cpp) define all
# 309 externals they reference between them -- including ERR_getErrorString,
# which zstddeclib.cpp provides -- so nothing here resolved against the 3.4 MB
# library. XBinaryViewer links libzstd_static directly and is unaffected.

# ancient.cmake / deark.cmake / legacydisk.cmake are gone: the XAncientDecoder
# codecs are inlined in the ZIP core (Formats/xformats.cmake) because
# core/xdecompress.cpp calls them, and the Deark and legacy-disk decoders are
# inlined in xarchive.cmake next to the readers that reach them.
set(XARCHIVE_SOURCES
    ${XARCHIVE_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/xancient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xancient.h
    ${CMAKE_CURRENT_LIST_DIR}/xdearkarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xdearkarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xlibdskarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xlibdskarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xadfarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xadfarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xcompactproarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcompactproarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/xdiskdoublerarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xdiskdoublerarchive.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmaclegacydecoders.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xmaclegacydecoders.h)
list(APPEND XARCHIVE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xpyinstallercarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xpyinstallercarchive.h)
