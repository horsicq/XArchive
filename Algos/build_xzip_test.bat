@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Building XZip Test with Custom PPMd
echo ========================================
echo.

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to initialize MSVC environment
    exit /b 1
)

REM Set Qt paths
set QTDIR=C:\Qt\5.15.2\msvc2019_64
set PATH=%QTDIR%\bin;%PATH%

REM Create temporary build directory
set BUILD_DIR=%TEMP%\xzip_ppmd_build_%RANDOM%
mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Build directory: %BUILD_DIR%
echo.

REM Run CMake
echo Configuring with CMake...
cmake -G "NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH=%QTDIR% ^
    -S "C:\tmp_build\qt5\_mylibs\Formats\tests" ^
    -B . ^
    -DTEST_TARGET=xzip

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..
    rmdir /s /q "%BUILD_DIR%"
    exit /b 1
)

echo.
echo Building project...
nmake

if errorlevel 1 (
    echo ERROR: Build failed
    cd ..
    rmdir /s /q "%BUILD_DIR%"
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Executable: %BUILD_DIR%\FormatsTestsXZip.exe
echo ========================================
echo.

REM Keep build directory for inspection
echo Build directory preserved: %BUILD_DIR%
pause
