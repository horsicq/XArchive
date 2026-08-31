HEADERS += \
    $$PWD/Algos/xdearkdecoder.h \
    $$PWD/Algos/xdearkengine_p.h \
    $$PWD/Algos/xdearkmoduleregistry_p.h \
    $$PWD/Algos/xdearkengine_api_p.h \
    $$PWD/Algos/xdearkengine_config_p.h \
    $$PWD/Algos/xdearkengine_cp932data_p.h \
    $$PWD/Algos/xdearkengine_dskdcmps_p.h \
    $$PWD/Algos/xdearkengine_fmtutil_p.h \
    $$PWD/Algos/xdearkengine_fmtutilarchive_p.h \
    $$PWD/Algos/xdearkengine_lzhuf_p.h \
    $$PWD/Algos/xdearkengine_miniz_p.h \
    $$PWD/Algos/xdearkengine_minizapi_p.h \
    $$PWD/Algos/xdearkengine_modules_p.h \
    $$PWD/Algos/xdearkengine_ozunreduce_p.h \
    $$PWD/Algos/xdearkengine_private_p.h \
    $$PWD/Algos/xdearkengine_types_p.h \
    $$PWD/Algos/xdearkengine_version_p.h \
    $$PWD/Algos/xdearkstuffit13_p.h \
    $$PWD/Algos/xdearkstuffit13tables_p.h

SOURCES += \
    $$PWD/Algos/xdearkdecoder.cpp \
    $$PWD/Algos/xdearkengine_p.cpp \
    $$PWD/Algos/xdearkengine_api_p.cpp \
    $$PWD/Algos/xdearkengine_bitmap_p.cpp \
    $$PWD/Algos/xdearkengine_buffer_p.cpp \
    $$PWD/Algos/xdearkengine_char_p.cpp \
    $$PWD/Algos/xdearkengine_data_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_advfile_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_archive_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_compress_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_exe_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_fax_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_huffman_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_iff_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_lzah_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_lzh_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_lzw_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_miniz_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_rle_p.cpp \
    $$PWD/Algos/xdearkengine_fmtutil_zip_p.cpp \
    $$PWD/Algos/xdearkengine_font_p.cpp \
    $$PWD/Algos/xdearkengine_png_p.cpp \
    $$PWD/Algos/xdearkengine_registry_p.cpp \
    $$PWD/Algos/xdearkengine_tar_p.cpp \
    $$PWD/Algos/xdearkengine_ucstring_p.cpp \
    $$PWD/Algos/xdearkengine_unix_p.cpp \
    $$PWD/Algos/xdearkengine_util2_p.cpp \
    $$PWD/Algos/xdearkengine_util_p.cpp \
    $$PWD/Algos/xdearkengine_win_p.cpp \
    $$PWD/Algos/xdearkengine_zip_p.cpp \
    $$PWD/Algos/xdearkmodule_arc_p.cpp \
    $$PWD/Algos/xdearkmodule_dskexp_p.cpp \
    $$PWD/Algos/xdearkmodule_fat_p.cpp \
    $$PWD/Algos/xdearkmodule_grasp_p.cpp \
    $$PWD/Algos/xdearkmodule_installshld_p.cpp \
    $$PWD/Algos/xdearkmodule_kdc_p.cpp \
    $$PWD/Algos/xdearkmodule_lbr_p.cpp \
    $$PWD/Algos/xdearkmodule_lha_p.cpp \
    $$PWD/Algos/xdearkmodule_misc3_p.cpp \
    $$PWD/Algos/xdearkmodule_misc_p.cpp \
    $$PWD/Algos/xdearkmodule_mscompress_p.cpp \
    $$PWD/Algos/xdearkmodule_nufx_p.cpp \
    $$PWD/Algos/xdearkmodule_os2ea_p.cpp \
    $$PWD/Algos/xdearkmodule_os2pack_p.cpp \
    $$PWD/Algos/xdearkmodule_packit_p.cpp \
    $$PWD/Algos/xdearkmodule_reko_p.cpp \
    $$PWD/Algos/xdearkmodule_rsc_p.cpp \
    $$PWD/Algos/xdearkmodule_stuffit_p.cpp \
    $$PWD/Algos/xdearkstuffit13_p.cpp

DISTFILES += \
    $$PWD/Algos/xdearkdecoder.LICENSE \
    $$PWD/Algos/xdearkdecoder.LICENSE.compcol \
    $$PWD/Algos/xdearkdecoder.LICENSE.miniz \
    $$PWD/Algos/xdearkdecoder.NOTICE.md \
    $$PWD/Algos/xdearkdecoder.NOTICE.dskdcmps.txt \
    $$PWD/Algos/xdearkdecoder.NOTICE.lzhuf.txt \
    $$PWD/Algos/xdearkdecoder.NOTICE.miniz.txt

!msvc: QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-missing-field-initializers
