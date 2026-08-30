INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/Algos
DEPENDPATH += $$PWD/Algos
INCLUDEPATH += $$PWD/Algos/include
DEPENDPATH += $$PWD/Algos/include

# The ZIP + decompression core (XZip family, XArchive base, XCompress/
# XDecompress, ALL Algos decoders, and xbinary/xoptions/xpng/xjavaclass/
# xandroidbinary) lives in xzip.pri so it can be used WITHOUT the USE_ARCHIVE
# define; this file adds the remaining archive formats on top.
!contains(XCONFIG, xzip) {
    XCONFIG += xzip
    include($$PWD/xzip.pri)
}

include($$PWD/ancient.pri)
include($$PWD/deark.pri)
include($$PWD/libdsk.pri)

HEADERS += \
    $$PWD/x_ar.h \
    $$PWD/xancient.h \
    $$PWD/xlegacyencoded.h \
    $$PWD/xdearkarchive.h \
    $$PWD/xlibdskarchive.h \
    $$PWD/xcompactproarchive.h \
    $$PWD/xdiskdoublerarchive.h \
    $$PWD/Algos/xmaclegacydecoders.h \
    $$PWD/Algos/xpaxdecoder.h \
    $$PWD/Algos/xvisedeflatedecoder.h \
    $$PWD/xpyinstallercarchive.h \
    $$PWD/xwisesfxarchive.h \
    $$PWD/xlegacystorearchive.h \
    $$PWD/xconcatziparchive.h \
    $$PWD/xdiskjugglerarchive.h \
    $$PWD/xapplesingle.h \
    $$PWD/x2img.h \
    $$PWD/xpyz.h \
    $$PWD/xlzxarchive.h \
    $$PWD/xmacbinary.h \
    $$PWD/xresourcefork.h \
    $$PWD/xlbr.h \
    $$PWD/xseaarc.h \
    $$PWD/xexternalarchive.h \
    $$PWD/xfreearc.h \
    $$PWD/xzpaq.h \
    $$PWD/xbcm.h \
    $$PWD/xlpaq8.h \
    $$PWD/xpea.h \
    $$PWD/xarj.h \
    $$PWD/xace.h \
    $$PWD/xfilteredarchive.h \
    $$PWD/xcab.h \
    $$PWD/xcfbf.h \
    $$PWD/xcpio.h \
    $$PWD/games/xpak.h \
    $$PWD/games/xwad.h \
    $$PWD/games/xgrp.h \
    $$PWD/games/xhog.h \
    $$PWD/games/xwolfvswap.h \
    $$PWD/games/xwintermutedcp.h \
    $$PWD/games/xt64.h \
    $$PWD/games/xgamestorearchive_p.h \
    $$PWD/games/xckp.h \
    $$PWD/games/xedp.h \
    $$PWD/games/xmpq.h \
    $$PWD/games/xbigf.h \
    $$PWD/games/xparsecarchive.h \
    $$PWD/games/xpmm.h \
    $$PWD/xwarc.h \
    $$PWD/xmtree.h \
    $$PWD/xuu.h \
    $$PWD/xdeb.h \
    $$PWD/xdos16.h \
    $$PWD/xgzip.h \
    $$PWD/xiso9660.h \
    $$PWD/xudf.h \
    $$PWD/xwim.h \
    $$PWD/xrpm.h \
    $$PWD/xkwaj.h \
    $$PWD/xasar.h \
    $$PWD/xxar.h \
    $$PWD/xzoo.h \
    $$PWD/xstk.h \
    $$PWD/xlha.h \
    $$PWD/xsar.h \
    $$PWD/xarx.h \
    $$PWD/xmachofat.h \
    $$PWD/xrar.h \
    $$PWD/xsevenzip.h \
    $$PWD/xsquashfs.h \
    $$PWD/xtar.h \
    $$PWD/xtarcompressed.h \
    $$PWD/xtar_gz.h \
    $$PWD/xtar_bzip2.h \
    $$PWD/xtar_lzip.h \
    $$PWD/xtar_lzma.h \
    $$PWD/xtar_lzop.h \
    $$PWD/xtar_xz.h \
    $$PWD/xtar_zstd.h \
    $$PWD/xtar_lz4.h \
    $$PWD/xtar_compress.h \
    $$PWD/xnpm.h \
    $$PWD/xszdd.h \
    $$PWD/xbzip2.h \
    $$PWD/xbrotli.h \
    $$PWD/xlzip.h \
    $$PWD/xxz.h \
    $$PWD/xzlib.h \
    $$PWD/xzstd.h \
    $$PWD/xlz4.h \
    $$PWD/xlz5.h \
    $$PWD/xlizard.h \
    $$PWD/xlzma.h \
    $$PWD/xlzo.h \
    $$PWD/xcompressz.h \
    $$PWD/xminidump.h \
    $$PWD/xdmg.h

SOURCES += \
    $$PWD/x_ar.cpp \
    $$PWD/xancient.cpp \
    $$PWD/xlegacyencoded.cpp \
    $$PWD/xdearkarchive.cpp \
    $$PWD/xlibdskarchive.cpp \
    $$PWD/xcompactproarchive.cpp \
    $$PWD/xdiskdoublerarchive.cpp \
    $$PWD/Algos/xmaclegacydecoders.cpp \
    $$PWD/Algos/xpaxdecoder.cpp \
    $$PWD/Algos/xvisedeflatedecoder.cpp \
    $$PWD/xpyinstallercarchive.cpp \
    $$PWD/xwisesfxarchive.cpp \
    $$PWD/xlegacystorearchive.cpp \
    $$PWD/xconcatziparchive.cpp \
    $$PWD/xdiskjugglerarchive.cpp \
    $$PWD/xapplesingle.cpp \
    $$PWD/x2img.cpp \
    $$PWD/xpyz.cpp \
    $$PWD/xlzxarchive.cpp \
    $$PWD/xmacbinary.cpp \
    $$PWD/xresourcefork.cpp \
    $$PWD/xlbr.cpp \
    $$PWD/xseaarc.cpp \
    $$PWD/xexternalarchive.cpp \
    $$PWD/xfreearc.cpp \
    $$PWD/xzpaq.cpp \
    $$PWD/xbcm.cpp \
    $$PWD/xlpaq8.cpp \
    $$PWD/xpea.cpp \
    $$PWD/xarj.cpp \
    $$PWD/xace.cpp \
    $$PWD/xfilteredarchive.cpp \
    $$PWD/xcab.cpp \
    $$PWD/xcfbf.cpp \
    $$PWD/xcpio.cpp \
    $$PWD/games/xpak.cpp \
    $$PWD/games/xwad.cpp \
    $$PWD/games/xgrp.cpp \
    $$PWD/games/xhog.cpp \
    $$PWD/games/xwolfvswap.cpp \
    $$PWD/games/xwintermutedcp.cpp \
    $$PWD/games/xt64.cpp \
    $$PWD/games/xgamestorearchive_p.cpp \
    $$PWD/games/xckp.cpp \
    $$PWD/games/xedp.cpp \
    $$PWD/games/xmpq.cpp \
    $$PWD/games/xbigf.cpp \
    $$PWD/games/xparsecarchive.cpp \
    $$PWD/games/xpmm.cpp \
    $$PWD/xwarc.cpp \
    $$PWD/xmtree.cpp \
    $$PWD/xuu.cpp \
    $$PWD/xdeb.cpp \
    $$PWD/xdos16.cpp \
    $$PWD/xgzip.cpp \
    $$PWD/xiso9660.cpp \
    $$PWD/xudf.cpp \
    $$PWD/xwim.cpp \
    $$PWD/xrpm.cpp \
    $$PWD/xkwaj.cpp \
    $$PWD/xasar.cpp \
    $$PWD/xxar.cpp \
    $$PWD/xzoo.cpp \
    $$PWD/xstk.cpp \
    $$PWD/xlha.cpp \
    $$PWD/xsar.cpp \
    $$PWD/xarx.cpp \
    $$PWD/xmachofat.cpp \
    $$PWD/xrar.cpp \
    $$PWD/xsevenzip.cpp \
    $$PWD/xsquashfs.cpp \
    $$PWD/xtar.cpp \
    $$PWD/xtarcompressed.cpp \
    $$PWD/xtar_gz.cpp \
    $$PWD/xtar_bzip2.cpp \
    $$PWD/xtar_lzip.cpp \
    $$PWD/xtar_lzma.cpp \
    $$PWD/xtar_lzop.cpp \
    $$PWD/xtar_xz.cpp \
    $$PWD/xtar_zstd.cpp \
    $$PWD/xtar_lz4.cpp \
    $$PWD/xtar_compress.cpp \
    $$PWD/xnpm.cpp \
    $$PWD/xszdd.cpp \
    $$PWD/xbzip2.cpp \
    $$PWD/xbrotli.cpp \
    $$PWD/xlzip.cpp \
    $$PWD/xxz.cpp \
    $$PWD/xzlib.cpp \
    $$PWD/xzstd.cpp \
    $$PWD/xlz4.cpp \
    $$PWD/xlz5.cpp \
    $$PWD/xlizard.cpp \
    $$PWD/xlzma.cpp \
    $$PWD/xlzo.cpp \
    $$PWD/xcompressz.cpp \
    $$PWD/xminidump.cpp \
    $$PWD/xdmg.cpp

!contains(XCONFIG, xmach) {
    XCONFIG += xmach
    include($$PWD/../Formats/exec/xmach.pri) # MACHFAT archive contains Mach-O
}

DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/xarchive.cmake
