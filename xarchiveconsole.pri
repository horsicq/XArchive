INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xarchiveconsole.h

SOURCES += \
    $$PWD/xarchiveconsole.cpp

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
