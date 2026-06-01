@echo off
setlocal EnableExtensions

cd /d "%~dp0"

set "APP=%CD%\build-windows\gauss_with_clusters.exe"
set "PIPE=\\.\pipe\TerrainPipe"

if not exist "%APP%" (
    echo [INFO] Executable not found, building first...
    call build_windows.bat
    if errorlevel 1 exit /b %errorlevel%
)

echo [INFO] Starting one shared server...
start "terrain server" "%APP%" --server --config seminar_config.txt --pipe "%PIPE%"

timeout /T 1 /NOBREAK >NUL

echo [INFO] Starting seminar clients in parallel against one server...
start "client seminar1" "%APP%" --client seminar1_commands.txt --config seminar_config.txt --pipe "%PIPE%"
start "client seminar2" "%APP%" --client seminar2_commands.txt --config seminar_config.txt --pipe "%PIPE%"
start "client seminar3" "%APP%" --client seminar3_commands.txt --config seminar_config.txt --pipe "%PIPE%"

echo [INFO] Waiting for clients to finish...
timeout /T 5 /NOBREAK >NUL

echo [INFO] Stopping shared server...
"%APP%" --shutdown-only --pipe "%PIPE%"

echo.
echo [OK] One server handled seminar1, seminar2 and seminar3 clients.
echo Outputs are in:
echo   output\

endlocal
