include_directories(${CMAKE_CURRENT_LIST_DIR})

if (NOT DEFINED XARCHIVES_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/xarchives.cmake)
    set(XARCHIVECONSOLE_SOURCES ${XARCHIVECONSOLE_SOURCES} ${XARCHIVES_SOURCES})
endif()

# XArchiveConsole calls XFormats directly (createClass/getPrefFileType/
# getFileInfo/getEntropy), so it composes the full format layer rather than
# relying on a consumer to have added it.
if (NOT DEFINED XFORMATS_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../Formats/xformats.cmake)
    set(XARCHIVECONSOLE_SOURCES ${XARCHIVECONSOLE_SOURCES} ${XFORMATS_SOURCES})
endif()

# The --struct / --showstructs viewers render XFHEADER data through the
# XFModel/XFTreeModel family.
if (NOT DEFINED XFMODEL_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../Formats/xfmodel.cmake)
    set(XARCHIVECONSOLE_SOURCES ${XARCHIVECONSOLE_SOURCES} ${XFMODEL_SOURCES})
endif()

set(XARCHIVECONSOLE_SOURCES
    ${XARCHIVECONSOLE_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/xarchiveconsole.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xarchiveconsole.h
)
