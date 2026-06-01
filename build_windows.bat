@echo off
setlocal EnableExtensions

cd /d "%~dp0"

set "BUILD_DIR=build-windows"
set "CONFIG=Release"
set "APP_EXE=%BUILD_DIR%\%CONFIG%\terrain_app.exe"

where cmake >NUL 2>NUL
if errorlevel 1 (
    echo [ERROR] CMake was not found in PATH.
    echo Install CMake and enable "Add CMake to the system PATH".
    exit /b 1
)

echo [INFO] Configuring Visual Studio build...
cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo.
    echo [WARN] Visual Studio 2022 generator failed.
    echo [INFO] Trying MinGW Makefiles...
    cmake -S . -B "%BUILD_DIR%" -G "MinGW Makefiles"
    if errorlevel 1 (
        echo.
        echo [ERROR] Could not configure the project.
        echo Install Visual Studio 2022 C++ tools or MinGW-w64, then try again.
        exit /b 1
    )

    echo [INFO] Building MinGW target...
    cmake --build "%BUILD_DIR%"
    if errorlevel 1 exit /b %errorlevel%

    set "APP_EXE=%BUILD_DIR%\terrain_app.exe"
) else (
    echo [INFO] Building Visual Studio target...
    cmake --build "%BUILD_DIR%" --config "%CONFIG%"
    if errorlevel 1 exit /b %errorlevel%
)

echo.
echo [OK] Built: %APP_EXE%
echo.
echo Run server:
echo   %APP_EXE% --server
echo.
echo Run client:
echo   %APP_EXE% --client commands.txt --shutdown
echo.
echo Run seminar sample:
echo   %APP_EXE% --config files\seminar_config.txt files\seminar1_commands.txt

endlocal
