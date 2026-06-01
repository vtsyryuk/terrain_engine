@echo off
setlocal

set BUILD_DIR=build-windows

cmake -S . -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b %errorlevel%

cmake --build %BUILD_DIR% --config Release
if errorlevel 1 exit /b %errorlevel%

echo.
echo Built: %BUILD_DIR%\Release\terrain_app.exe
echo Run server: %BUILD_DIR%\Release\terrain_app.exe --server
echo Run client: %BUILD_DIR%\Release\terrain_app.exe --client commands.txt --shutdown
