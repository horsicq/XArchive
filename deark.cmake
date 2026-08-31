# Project-owned C++ engine plus the explicitly selected private legacy decoder
# classes and implementation units. No C translation units are part of this
# target; the former xdearkcodec source tree is not part of the project.
set(XARCHIVE_DEARK_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmoduleregistry_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_api_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_bitmap_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_buffer_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_char_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_data_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_advfile_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_archive_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_compress_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_exe_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_fax_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_huffman_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_iff_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_lzah_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_lzh_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_lzw_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_miniz_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_rle_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_zip_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_font_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_png_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_registry_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_tar_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_ucstring_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_unix_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_util2_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_util_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_win_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_zip_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_arc_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_dskexp_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_fat_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_grasp_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_installshld_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_kdc_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_lbr_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_lha_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_misc3_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_misc_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_mscompress_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_nufx_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_os2ea_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_os2pack_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_packit_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_reko_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_rsc_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkmodule_stuffit_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkstuffit13_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_api_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_config_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_cp932data_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_dskdcmps_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutil_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_fmtutilarchive_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_lzhuf_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_miniz_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_minizapi_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_modules_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_ozunreduce_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_private_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_types_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkengine_version_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkstuffit13_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xdearkstuffit13tables_p.h)

if(NOT MSVC)
    set_source_files_properties(${XARCHIVE_DEARK_SOURCES} PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-missing-field-initializers")
endif()
