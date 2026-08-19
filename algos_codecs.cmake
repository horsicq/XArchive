# ---------------------------------------------------------------------------
# bzip2 / zlib / lzma / ppmd targets, built from the Algos amalgamations.
#
# These were four separate subprojects under 3rdparty/. Their sources are now
# Algos amalgamations, leaving each CMakeLists as a two-line wrapper, so the
# targets are defined here instead and those directories could go.
#
# This file exists separately from CMakeLists.txt because several standalone
# demo/test projects link bzip2/zlib/lzma/ppmd by name WITHOUT adding XArchive
# as a subdirectory. Before the removal they got the targets from
# add_subdirectory(3rdparty/<lib>); with those directories gone, a bare name in
# target_link_libraries silently degrades to a raw linker input (zlib.lib with
# an empty LINK_PATH) and fails only at link time. Such a project should
# include() this file rather than duplicate the definitions.
#
# The lzma/ppmd decoders stay in their own targets rather than folding into
# xarchive_7zip: several projects link them without linking xarchive_7zip, and
# keeping them separate preserves one definition of each C symbol.
#
# POSITION_INDEPENDENT_CODE is carried over from the four original subprojects.
# Dropping PIC is invisible on MSVC but hard-fails the ELF shared-library builds
# (die/nfd) with "relocation R_X86_64_PC32 against symbol 'z_errmsg' ...
# recompile with -fPIC". The amalgamations are now .cpp, so the former
# LINKER_LANGUAGE C / C_STANDARD 11 became LINKER_LANGUAGE CXX / CXX_STANDARD 14,
# matching xarchive_7zip.
#
# Those sources were converted from C to C++ on 2026-08-17 and must stay C++.
# Do not add them to a C target or rename any of them back to .c. Several of
# their internal symbols (LzmaDec_InitDicAndState, Ppmd7/8_UpdateModel, ...) are
# declared in no header, so their linkage names follow the language each unit is
# compiled as; a C build of one unit and a C++ build of its caller would not
# fail here but at link, with unresolved externals. Each file carries an
# #ifndef __cplusplus / #error guard so that fails at compile instead.
# ---------------------------------------------------------------------------
set(XARCHIVE_ALGOS_DIR "${CMAKE_CURRENT_LIST_DIR}/Algos")

if(NOT TARGET bzip2)
    add_library(bzip2 STATIC "${XARCHIVE_ALGOS_DIR}/bzip2declib.cpp")
    target_include_directories(bzip2 PUBLIC "${XARCHIVE_ALGOS_DIR}/include")
    set_target_properties(bzip2 PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 14
        POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    if(MSVC)
        target_compile_definitions(bzip2 PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

if(NOT TARGET zlib)
    add_library(zlib STATIC
        "${XARCHIVE_ALGOS_DIR}/zlibutil.cpp"
        "${XARCHIVE_ALGOS_DIR}/zlibdeclib.cpp"
        "${XARCHIVE_ALGOS_DIR}/zlibenclib.cpp"
    )
    target_include_directories(zlib PUBLIC "${XARCHIVE_ALGOS_DIR}/include")
    set_target_properties(zlib PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 14
        POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    if(MSVC)
        target_compile_definitions(zlib PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

if(NOT TARGET lzma)
    add_library(lzma STATIC
        "${XARCHIVE_ALGOS_DIR}/sevenzip_extcodec_lzmadec.cpp"
        "${XARCHIVE_ALGOS_DIR}/sevenzip_extcodec_lzma2dec.cpp"
    )
    target_include_directories(lzma PUBLIC "${XARCHIVE_ALGOS_DIR}" "${XARCHIVE_ALGOS_DIR}/include")
    set_target_properties(lzma PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 14
        POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    if(MSVC)
        target_compile_definitions(lzma PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

if(NOT TARGET ppmd)
    add_library(ppmd STATIC
        "${XARCHIVE_ALGOS_DIR}/sevenzip_extcodec_ppmd7.cpp"
        "${XARCHIVE_ALGOS_DIR}/sevenzip_extcodec_ppmd7dec.cpp"
        "${XARCHIVE_ALGOS_DIR}/sevenzip_extcodec_ppmd8.cpp"
        "${XARCHIVE_ALGOS_DIR}/sevenzip_extcodec_ppmd8dec.cpp"
    )
    target_include_directories(ppmd PUBLIC "${XARCHIVE_ALGOS_DIR}" "${XARCHIVE_ALGOS_DIR}/include")
    set_target_properties(ppmd PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 14
        POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    if(MSVC)
        target_compile_definitions(ppmd PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()
