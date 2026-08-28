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

# bzip2 / zlib / lzma / ppmd, built from the Algos amalgamations. Kept in a
# separate file so standalone demo/test projects that link these by name can
# include() it without adding all of XArchive as a subdirectory.
include("${CMAKE_CURRENT_LIST_DIR}/algos_codecs.cmake")

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared library" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static library" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Build zstd tests" FORCE)
# zstd_static is not built here any more. A symbol-level check showed the Algos
# zstd objects (zstddeclib.cpp, zstd_xxhash.cpp, zstdlegacy_v04-v07.cpp) define all
# 309 externals they reference between them -- including ERR_getErrorString,
# which zstddeclib.cpp provides -- so nothing here resolved against the 3.4 MB
# library. XBinaryViewer links libzstd_static directly and is unaffected.
