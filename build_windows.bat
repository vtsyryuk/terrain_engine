@echo off
setlocal EnableExtensions

cd /d "%~dp0"

set "SRC=gauss_with_clusters.cpp"
set "BUILD_DIR=build-windows"
set "APP_EXE=%BUILD_DIR%\gauss_with_clusters.exe"

if not exist "%SRC%" (
    echo [ERROR] Cannot find %SRC%
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "CXX="
where gcc >NUL 2>NUL
if not errorlevel 1 set "CXX=gcc"

if "%CXX%"=="" (
    where g++ >NUL 2>NUL
    if not errorlevel 1 set "CXX=g++"
)

if "%CXX%"=="" (
    echo [ERROR] GCC was not found in PATH.
    echo Install MinGW-w64 or the Code::Blocks package with MinGW.
    echo Then add the compiler bin folder to PATH, for example:
    echo   C:\Program Files\CodeBlocks\MinGW\bin
    exit /b 1
)

echo [INFO] Compiler: %CXX%
echo [INFO] Building standalone %SRC%...

if /I "%CXX%"=="gcc" (
    gcc -x c++ -std=c++17 -Wall -Wextra -pedantic "%SRC%" -lstdc++ -o "%APP_EXE%"
) else (
    g++ -std=c++17 -Wall -Wextra -pedantic "%SRC%" -o "%APP_EXE%"
)

if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b %errorlevel%
)

echo.
echo [OK] Built: %APP_EXE%
echo.
echo Run PDF field1 sample:
echo   "%APP_EXE%" files\field1_commands.txt files\seminar_config.txt
echo.
echo Run the default sample:
echo   "%APP_EXE%" files\seminar1_commands.txt files\seminar_config.txt
echo.
echo Run seminar 2:
echo   "%APP_EXE%" files\seminar2_commands.txt files\seminar_config.txt
echo.
echo Run seminar 3:
echo   "%APP_EXE%" files\seminar3_commands.txt files\seminar_config.txt
echo.
echo Output files will be written to the output\ folder.

endlocal
