cmake_minimum_required(VERSION 3.14)

project(zlib LANGUAGES C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${PROJECT_SOURCE_DIR}/src/)

add_library(zlib STATIC
${PROJECT_SOURCE_DIR}/src/deflate.c
${PROJECT_SOURCE_DIR}/src/deflate.h
${PROJECT_SOURCE_DIR}/src/inflate.c
${PROJECT_SOURCE_DIR}/src/inflate.h
${PROJECT_SOURCE_DIR}/src/adler32.c
${PROJECT_SOURCE_DIR}/src/crc32.c
${PROJECT_SOURCE_DIR}/src/crc32.h
${PROJECT_SOURCE_DIR}/src/inffast.c
${PROJECT_SOURCE_DIR}/src/inffast.h
${PROJECT_SOURCE_DIR}/src/inftrees.c
${PROJECT_SOURCE_DIR}/src/inftrees.h
${PROJECT_SOURCE_DIR}/src/trees.c
${PROJECT_SOURCE_DIR}/src/trees.h
${PROJECT_SOURCE_DIR}/src/zutil.c
${PROJECT_SOURCE_DIR}/src/zutil.h
)

set_target_properties(zlib PROPERTIES LINKER_LANGUAGE C)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
