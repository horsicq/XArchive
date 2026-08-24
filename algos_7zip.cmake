# ---------------------------------------------------------------------------
# 7-Zip 26.01 all-format in-process archive core.
# Upstream tag commit: 8c63d71ff886bda90c86db28466287f977374237.
#
# This module intentionally uses an OBJECT library. Archive and codec handlers
# register themselves from translation-unit-local static constructors, so an
# ordinary STATIC library can silently lose unreferenced handler objects.
#
# LzmaDec/Lzma2Dec/Ppmd7/Ppmd7Dec/Ppmd8/Ppmd8Dec stay owned by the separate
# lzma and ppmd targets so exactly one definition of each C symbol reaches the
# executable. Those targets are shared -- DIE_library and main_test link them
# too -- so they cannot be folded into xarchive_7zip. They are excluded from the
# Algos C amalgamations and compiled from Algos/sevenzip_extcodec/.
#
# This file exists separately from CMakeLists.txt for the same reason
# algos_codecs.cmake does: several standalone demo/test projects need the
# target WITHOUT adding XArchive as a subdirectory. XArchive/CMakeLists.txt
# opens with cmake_minimum_required()/project(), so it cannot be include()d.
# Such a project should include() this file and add xarchive_7zip to its
# target_link_libraries.
#
# The three szc_* units were converted from C to C++ on 2026-08-17 and must
# stay C++. Do not rename them back to .c or move them to a C-only target: they
# define internal symbols that no header declares (LzmaEnc_*, Sha1/Sha256/
# Sha512_UpdateBlocks, XzCrc64UpdateT12, g_LargePage*, ...), so those names
# mangle or not depending on the language each unit is compiled as, and a mixed
# build fails at link rather than at compile. Each file carries an
# #ifndef __cplusplus / #error guard to catch that at compile instead.
#
# It is needed by anything that compiles XARCHIVE_SOURCES: xarchives_ip7z.cpp
# is unconditional in that list (xarchive.cmake), and without this target it
# fails at link time with ~15 unresolved externals (GetNumberOfFormats,
# CreateArchiver, UString::UString, NWindows::NCOM::CPropVariant::*, ...)
# whose definitions live in the amalgamated 7-Zip units below.
# ---------------------------------------------------------------------------

# lzma / ppmd are INTERFACE dependencies of the target defined here, and zlib
# is needed by every consumer that compiles XARCHIVE_SOURCES. Including this is
# a no-op when the caller already did -- algos_codecs.cmake guards each target.
include("${CMAKE_CURRENT_LIST_DIR}/algos_codecs.cmake")

set(XARCHIVE_7ZIP_TARGET xarchive_7zip)

if(NOT TARGET ${XARCHIVE_7ZIP_TARGET})

set(XARCHIVE_7ZIP_PINNED_VERSION "26.01")
# The upstream tree has been folded into the Algos amalgamations, so there is
# no 7zVersion.h on disk to read; the pin is recorded here and in the banner of
# each generated file.

# The pin used to be verified by reading MY_VERSION_NUMBERS out of
# C/7zVersion.h. That header now lives inside Algos/sevenzip_api.h, so the
# check moved to the amalgamations themselves: sevenzip_api.h carries the
# upstream 7zVersion.h verbatim, and a mismatch would surface as a compile
# error against MY_VERSION_NUMBERS rather than a configure-time string compare.

# Explicitly resolved from CPP/7zip/Bundles/Format7zF/Arc.mak in tag 26.01,
# plus the portable C optimization fallbacks and ArchiveExports.cpp.
set(_xarchive_7zip_sources
    # 7-Zip C codecs, built as C++ (15 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szc_03.cpp"
    # 7-Zip C codecs, built as C++ (16 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szc_02.cpp"
    # 7-Zip C codecs, built as C++ (16 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szc_01.cpp"
    # 7-Zip szhash group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szhash_07.cpp"
    # 7-Zip szhash group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szhash_06.cpp"
    # 7-Zip szhash group (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szhash_05.cpp"
    # 7-Zip szhash group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szhash_04.cpp"
    # 7-Zip szhash group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szhash_03.cpp"
    # 7-Zip szhash group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szhash_02.cpp"
    # 7-Zip szhash group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szhash_01.cpp"
    # 7-Zip szcr group (5 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szcr_05.cpp"
    # 7-Zip szcr group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szcr_04.cpp"
    # 7-Zip szcr group (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szcr_03.cpp"
    # 7-Zip szcr group (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szcr_02.cpp"
    # 7-Zip szcr group (4 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szcr_01.cpp"
    # 7-Zip szznr group (5 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szznr_04.cpp"
    # 7-Zip szznr group (4 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szznr_03.cpp"
    # 7-Zip szznr group (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szznr_02.cpp"
    # 7-Zip szznr group (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szznr_01.cpp"
    # 7-Zip sz7z group (3 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sz7z_03.cpp"
    # 7-Zip sz7z group (3 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sz7z_02.cpp"
    # 7-Zip sz7z group (8 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sz7z_01.cpp"
    # 7-Zip szaf group (14 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_08.cpp"
    # 7-Zip szaf group (8 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_07.cpp"
    # 7-Zip szaf group (8 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_06.cpp"
    # 7-Zip szaf group (4 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_05.cpp"
    # 7-Zip szaf group (4 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_04.cpp"
    # 7-Zip szaf group (4 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_03.cpp"
    # 7-Zip szaf group (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_02.cpp"
    # 7-Zip szaf group (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/szaf_01.cpp"
    # CPP/7zip/Archive/wim reader (4 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_wim.cpp"
    # CPP/7zip/Archive/udf reader (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_udf.cpp"
    # CPP/7zip/Archive/tar reader (7 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_tar.cpp"
    # CPP/7zip/Archive/rar reader (1 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_rar.cpp"
    # CPP/7zip/Archive/iso reader (4 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_iso.cpp"
    # CPP/7zip/Archive/common reader (10 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_common.cpp"
    # CPP/7zip/Archive/chm reader (2 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_chm.cpp"
    # CPP/7zip/Archive/cab reader (5 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_arch_cab.cpp"
    # CPP/7zip/Crypto ciphers
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_crypto.cpp"
    # CPP/7zip/Compress codecs, non-registering (37 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_compress.cpp"
    # CPP/7zip/Common stream, coder and property support (21 units)
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_7zcommon.cpp"
    # Amalgamation of CPP/Common + CPP/Windows (the non-registering support
    # code). The eight *Reg.cpp hashers stay separate: REGISTER_HASHER emits
    # CreateHasherSpec / g_HasherInfo / g_RegisterHasher at global scope with
    # identical names in every file, so two of them in one TU is a redefinition.
    "${CMAKE_CURRENT_LIST_DIR}/Algos/sevenzip_common.cpp"
)


# Manifest size guard. This exists so a handler cannot be dropped by accident:
# handlers self-register from TU-local static constructors, so a missing source
# does not fail the build, it silently removes format support.
#
# The expected count moves down as upstream units are folded into Algos
# amalgamations. Each reduction must be deliberate and accounted for here:
#   276  original Arc.mak-derived manifest (275 Arc.mak units + MyWindows.cpp,
#        which was never in Arc.mak -- see the note below the ledger)
#   -28  CPP/Common (non-registering) + CPP/Windows  -> Algos/sevenzip_common.cpp
#    +1  the amalgamation itself
#   -21  CPP/7zip/Common stream, coder and property support (21 units) -> Algos/sevenzip_7zcommon.cpp
#    +1  the amalgamation itself
#   -37  CPP/7zip/Compress codecs, non-registering (37 units) -> Algos/sevenzip_compress.cpp
#    +1  the amalgamation itself
#   -14  CPP/7zip/Crypto ciphers -> Algos/sevenzip_crypto.cpp
#    +1  the amalgamation itself
#   -5  CPP/7zip/Archive/cab reader (5 units) -> Algos/sevenzip_arch_cab.cpp
#    +1  the amalgamation itself
#   -2  CPP/7zip/Archive/chm reader (2 units) -> Algos/sevenzip_arch_chm.cpp
#    +1  the amalgamation itself
#   -10  CPP/7zip/Archive/common reader (10 units) -> Algos/sevenzip_arch_common.cpp
#    +1  the amalgamation itself
#   -4  CPP/7zip/Archive/iso reader (4 units) -> Algos/sevenzip_arch_iso.cpp
#    +1  the amalgamation itself
#   -1  CPP/7zip/Archive/rar reader (1 units) -> Algos/sevenzip_arch_rar.cpp
#    +1  the amalgamation itself
#   -7  CPP/7zip/Archive/tar reader (7 units) -> Algos/sevenzip_arch_tar.cpp
#    +1  the amalgamation itself
#   -2  CPP/7zip/Archive/udf reader (2 units) -> Algos/sevenzip_arch_udf.cpp
#    +1  the amalgamation itself
#   -4  CPP/7zip/Archive/wim reader (4 units) -> Algos/sevenzip_arch_wim.cpp
#    +1  the amalgamation itself
#   -2  7-Zip szaf group (2 units) -> Algos/szaf_01.cpp
#    +1  the amalgamation itself
#   -2  7-Zip szaf group (2 units) -> Algos/szaf_02.cpp
#    +1  the amalgamation itself
#   -4  7-Zip szaf group (4 units) -> Algos/szaf_03.cpp
#    +1  the amalgamation itself
#   -4  7-Zip szaf group (4 units) -> Algos/szaf_04.cpp
#    +1  the amalgamation itself
#   -4  7-Zip szaf group (4 units) -> Algos/szaf_05.cpp
#    +1  the amalgamation itself
#   -8  7-Zip szaf group (8 units) -> Algos/szaf_06.cpp
#    +1  the amalgamation itself
#   -8  7-Zip szaf group (8 units) -> Algos/szaf_07.cpp
#    +1  the amalgamation itself
#   -14  7-Zip szaf group (14 units) -> Algos/szaf_08.cpp
#    +1  the amalgamation itself
#   -8  7-Zip sz7z group (8 units) -> Algos/sz7z_01.cpp
#    +1  the amalgamation itself
#   -3  7-Zip sz7z group (3 units) -> Algos/sz7z_02.cpp
#    +1  the amalgamation itself
#   -3  7-Zip sz7z group (3 units) -> Algos/sz7z_03.cpp
#    +1  the amalgamation itself
#   -2  7-Zip szznr group (2 units) -> Algos/szznr_01.cpp
#    +1  the amalgamation itself
#   -2  7-Zip szznr group (2 units) -> Algos/szznr_02.cpp
#    +1  the amalgamation itself
#   -4  7-Zip szznr group (4 units) -> Algos/szznr_03.cpp
#    +1  the amalgamation itself
#   -5  7-Zip szznr group (5 units) -> Algos/szznr_04.cpp
#    +1  the amalgamation itself
#   -4  7-Zip szcr group (4 units) -> Algos/szcr_01.cpp
#    +1  the amalgamation itself
#   -2  7-Zip szcr group (2 units) -> Algos/szcr_02.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szcr group (1 units) -> Algos/szcr_03.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szcr group (1 units) -> Algos/szcr_04.cpp
#    +1  the amalgamation itself
#   -5  7-Zip szcr group (5 units) -> Algos/szcr_05.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szhash group (1 units) -> Algos/szhash_01.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szhash group (1 units) -> Algos/szhash_02.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szhash group (1 units) -> Algos/szhash_03.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szhash group (1 units) -> Algos/szhash_04.cpp
#    +1  the amalgamation itself
#   -2  7-Zip szhash group (2 units) -> Algos/szhash_05.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szhash group (1 units) -> Algos/szhash_06.cpp
#    +1  the amalgamation itself
#   -1  7-Zip szhash group (1 units) -> Algos/szhash_07.cpp
#    +1  the amalgamation itself
#   -16  7-Zip szc group (16 units) -> Algos/szc_01.cpp
#    +1  the amalgamation itself
#   -16  7-Zip szc group (16 units) -> Algos/szc_02.cpp
#    +1  the amalgamation itself
#   -15  7-Zip szc group (15 units) -> Algos/szc_03.cpp
#    +1  the amalgamation itself
#   ---
#   42  amalgamations (276 upstream units, all folded)
#
# MyWindows.cpp -- the non-Windows Win32/OLE shim the amalgamations call into
# -- used to be the one unit compiled from the vendored upstream tree rather
# than from Algos/, making 43 sources against 42 amalgamations. That tree is no
# longer referenced by the build at all, so the unit is now folded into
# Algos/sevenzip_common.cpp as its 28th (18 CPP/Common + 10 CPP/Windows). Its
# whole body is guarded by #ifndef _WIN32 upstream, so the Windows build is
# unchanged, and one amalgamation holding the definitions still means exactly
# one definition of those symbols -- MyWindows.h stays header-only everywhere
# else.
#
# The szc block above used to read -16/-16/-16 plus a fourth
# "-5 ... -> Algos/szc_04.cpp" line, followed by three "-0 ... built as C++"
# lines re-adding szc_01..03. That was wrong three ways: szc_04 has never
# existed on disk (only szc_01..03 do), szc_03 holds 15 units and not 16, and
# the trailing three lines counted the same three amalgamations a second time
# -- seven "+1"s for three files. The sum as written came to 40 while the
# bottom line claimed 42 and the guard said 43, so no reading of it was
# self-consistent. Each -N above is now the count in that file's own generated
# banner ("N upstream translation units folded into one"); those banners total
# exactly 276 across exactly 42 files, which is what closes the ledger.
set(XARCHIVE_7ZIP_EXPECTED_SOURCE_COUNT 42)

list(LENGTH _xarchive_7zip_sources _xarchive_7zip_source_count)
set(XARCHIVE_7ZIP_SOURCE_COUNT "${_xarchive_7zip_source_count}")
if(NOT _xarchive_7zip_source_count EQUAL ${XARCHIVE_7ZIP_EXPECTED_SOURCE_COUNT})
    message(FATAL_ERROR
        "xarchive_7zip source manifest must contain "
        "${XARCHIVE_7ZIP_EXPECTED_SOURCE_COUNT} entries, found "
        "${_xarchive_7zip_source_count}")
endif()

set(_xarchive_7zip_unique_sources ${_xarchive_7zip_sources})
list(REMOVE_DUPLICATES _xarchive_7zip_unique_sources)
list(LENGTH _xarchive_7zip_unique_sources _xarchive_7zip_unique_source_count)
if(NOT _xarchive_7zip_unique_source_count EQUAL ${XARCHIVE_7ZIP_EXPECTED_SOURCE_COUNT})
    message(FATAL_ERROR
        "xarchive_7zip source manifest contains duplicate paths")
endif()

set(_xarchive_7zip_missing_sources)
foreach(_xarchive_7zip_source IN LISTS _xarchive_7zip_sources)
    if(NOT EXISTS "${_xarchive_7zip_source}")
        list(APPEND _xarchive_7zip_missing_sources "${_xarchive_7zip_source}")
    endif()
endforeach()

if(_xarchive_7zip_missing_sources)
    list(JOIN _xarchive_7zip_missing_sources "\n  "
        _xarchive_7zip_missing_sources_text)
    message(FATAL_ERROR
        "xarchive_7zip source files are missing:\n  "
        "${_xarchive_7zip_missing_sources_text}")
endif()

add_library(${XARCHIVE_7ZIP_TARGET} OBJECT ${_xarchive_7zip_sources})

# Qt's automatic moc/uic/rcc must not run over the amalgamations. When a
# standalone project sets CMAKE_AUTOMOC ON before include()ing this file, that
# variable is what initializes the AUTOMOC property of every target created
# afterwards -- including this one.
set_target_properties(${XARCHIVE_7ZIP_TARGET} PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

# OBJECT libraries do not link themselves; usage requirements for consumers.
target_link_libraries(${XARCHIVE_7ZIP_TARGET}
    INTERFACE
        lzma
        ppmd
)

target_include_directories(${XARCHIVE_7ZIP_TARGET}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/Algos"
)

# Z7_EXTERNAL_CODECS must remain undefined: this monolithic registry uses the
# built-in CreateCoder registrations and does not compile DllExports2.cpp.
target_compile_definitions(${XARCHIVE_7ZIP_TARGET}
    PUBLIC
        UNICODE
        _UNICODE
        NOMINMAX
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)

set_target_properties(${XARCHIVE_7ZIP_TARGET} PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    UNITY_BUILD OFF
)

if(MSVC)
    # Keep the parent project's CRT and default cdecl convention. In
    # particular, do not copy upstream's standalone /MT or x86 /Gr flags.
    target_compile_options(${XARCHIVE_7ZIP_TARGET}
        PRIVATE
            /bigobj
            $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
    )
endif()

# OBJECT libraries do not link themselves; these are usage requirements for
# the final target consuming xarchive_7zip.

if(WIN32)
    target_link_libraries(${XARCHIVE_7ZIP_TARGET}
        INTERFACE
            oleaut32
            ole32
            user32
            advapi32
            shell32
    )
else()
    # Algos/szc_03.cpp (Thread_Create, Thread_Create_With_Affinity,
    # Thread_Create_With_CpuSet) calls pthread_create. glibc >= 2.34 folds the
    # pthread symbols into libc, so Linux links without asking for them;
    # FreeBSD and friends still need the explicit -pthread.
    find_package(Threads REQUIRED)
    target_link_libraries(${XARCHIVE_7ZIP_TARGET}
        INTERFACE
            Threads::Threads
    )
endif()

endif()
