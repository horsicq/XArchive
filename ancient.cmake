# Project-owned private implementations of the six legacy codecs exposed by
# XAncientDecoder. The former Ancient class tree is not part of this target.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(XARCHIVE_ANCIENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientdecoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientdecoder.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientcodecbase_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientcodecbase_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientdmsdecoder_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientdmsdecoder_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientfreezedecoder_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientfreezedecoder_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientpowerpackerdecoder_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientpowerpackerdecoder_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientunixpackdecoder_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientunixpackdecoder_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientrncdecoder_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientrncdecoder_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancienttpwmdecoder_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancienttpwmdecoder_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientdynamichuffman_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancienthuffman_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientinputstream_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientinputstream_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientoutputstream_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientoutputstream_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientvlc_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientxpk_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientbuffer_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientbuffer_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientcrc16_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientcrc16_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientcommon_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientcommon_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientmemorybuffer_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientmemorybuffer_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientoverflow_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientstaticbuffer_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientstaticbuffer_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientsubbuffer_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientsubbuffer_p.h
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientvectorbuffer_p.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algos/xancientvectorbuffer_p.h)
