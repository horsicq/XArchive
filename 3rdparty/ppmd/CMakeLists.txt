cmake_minimum_required(VERSION 3.10)
project(ppmd C)

# PPMd8 library from 7-Zip
# Original code by Igor Pavlov (public domain)
# Based on PPMd var.I (2002) by Dmitry Shkarin

set(CMAKE_C_STANDARD 99)

# Source files
set(PPMD_SOURCES
    src/Ppmd8.c
    src/Ppmd8Dec.c
)

# Header files
set(PPMD_HEADERS
    src/Ppmd8.h
)

# Create static library
add_library(ppmd STATIC ${PPMD_SOURCES} ${PPMD_HEADERS})

# Include directories
target_include_directories(ppmd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Disable warnings for 3rd party code
if(MSVC)
    target_compile_options(ppmd PRIVATE /W0)
else()
    target_compile_options(ppmd PRIVATE -w)
endif()
