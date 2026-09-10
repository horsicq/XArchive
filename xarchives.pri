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
    $$PWD/core/xarchives.h

SOURCES += \
    $$PWD/core/xarchives.cpp

!contains(XCONFIG, xarchive) {
    XCONFIG += xarchive
    include(xarchive.pri)
}

DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/xarchives.cmake
