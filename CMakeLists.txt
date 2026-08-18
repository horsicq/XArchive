cmake_minimum_required(VERSION 3.14)

project(XArchive)

# bzip2 / zlib / lzma / ppmd, built from the Algos amalgamations. Kept in a
# separate file so standalone demo/test projects that link these by name can
# include() it without adding all of XArchive as a subdirectory.
include("${CMAKE_CURRENT_LIST_DIR}/algos_codecs.cmake")

# The xarchive_7zip OBJECT library. Split out for the same reason as the codecs
# above: standalone demo/test projects that compile XARCHIVE_SOURCES need it
# (xarchives_ip7z.cpp is unconditional there) but cannot include() this file.
include("${CMAKE_CURRENT_LIST_DIR}/algos_7zip.cmake")

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared library" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static library" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Build zstd tests" FORCE)
# zstd_static is not built here any more. A symbol-level check showed the Algos
# zstd objects (zstddeclib.cpp, zstd_xxhash.cpp, zstdlegacy_v04-v07.cpp) define all
# 309 externals they reference between them -- including ERR_getErrorString,
# which zstddeclib.cpp provides -- so nothing here resolved against the 3.4 MB
# library. XBinaryViewer links libzstd_static directly and is unaffected.
