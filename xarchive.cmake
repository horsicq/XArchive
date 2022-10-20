include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rdparty/bzip2/src/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rdparty/lzma/src/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rdparty/zlib/src/)

include(${CMAKE_CURRENT_LIST_DIR}/../Formats/xbinary.cmake)
#include(${CMAKE_CURRENT_LIST_DIR}/../Formats/xmach.cmake)

set(XARCHIVE_SOURCES
    ${XBINARY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/xarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xcab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xmachofat.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xrar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xsevenzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xzip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/x_ar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xgzip.cpp
)
