include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/archives/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/compressors/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/core/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/diskimages/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/documents/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/filesystems/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/games/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/installers/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/packages/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/sfx/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/tar/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/transport/)

if (NOT DEFINED XARCHIVE_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/xarchive.cmake)
    set(XARCHIVES_SOURCES ${XARCHIVES_SOURCES} ${XARCHIVE_SOURCES})
endif()

set(XARCHIVES_SOURCES
    ${XARCHIVES_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/core/xarchives.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/xarchives.h
)
