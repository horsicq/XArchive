INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/src

win32-g++ {
    LIBS += $$PWD/libs/libppmd-win-$${QT_ARCH}.a
}
win32-msvc* {
    LIBS += $$PWD/libs/ppmd-win-$${QT_ARCH}.lib
}
unix:!macx {
    LIBS += $$PWD/libs/libppmd-unix-$${QT_ARCH}.a
}
unix:macx {
    LIBS += $$PWD/libs/libppmd-macos-$${QT_ARCH}.a
}
