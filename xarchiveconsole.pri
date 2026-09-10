INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/archives
INCLUDEPATH += $$PWD/compressors
INCLUDEPATH += $$PWD/core
INCLUDEPATH += $$PWD/diskimages
INCLUDEPATH += $$PWD/documents
INCLUDEPATH += $$PWD/filesystems
INCLUDEPATH += $$PWD/games
INCLUDEPATH += $$PWD/installers
INCLUDEPATH += $$PWD/packages
INCLUDEPATH += $$PWD/sfx
INCLUDEPATH += $$PWD/tar
INCLUDEPATH += $$PWD/transport
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/core/xarchiveconsole.h \
    $$PWD/core/xlegacyconsole.h

SOURCES += \
    $$PWD/core/xarchiveconsole.cpp \
    $$PWD/core/xlegacyconsole.cpp

!contains(XCONFIG, xarchives) {
    XCONFIG += xarchives
    include($$PWD/xarchives.pri)
}

# XArchiveConsole calls XFormats directly and renders --struct/--showstructs
# through the XFModel family.
!contains(XCONFIG, xformats) {
    XCONFIG += xformats
    include($$PWD/../Formats/xformats.pri)
}

!contains(XCONFIG, xfmodel) {
    XCONFIG += xfmodel
    include($$PWD/../Formats/xfmodel.pri)
}

DISTFILES += \
    $$PWD/xarchiveconsole.cmake
