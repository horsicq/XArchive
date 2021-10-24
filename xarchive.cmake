include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rdparty/bzip2/src/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rdparty/lzma/src/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rdparty/zlib/src/)

set(XARCHIVE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xmachofat.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xsevenzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xrar.cpp
)
