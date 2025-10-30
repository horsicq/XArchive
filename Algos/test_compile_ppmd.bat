@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d c:\tmp_build\qt5\_mylibs\XArchive\Algos
cl /nologo /c /std:c++17 /EHsc /I"C:\Qt\5.15.2\msvc2019_64\include" /I"C:\Qt\5.15.2\msvc2019_64\include\QtCore" /I"C:\tmp_build\qt5\_mylibs\Formats" /I"C:\tmp_build\qt5\_mylibs\XArchive" /I".." /I"." /DQT_NO_DEBUG /DQT_CORE_LIB xppmdmodel.cpp
