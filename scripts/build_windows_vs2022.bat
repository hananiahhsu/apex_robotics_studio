@echo off
setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0\..
for %%I in ("%ROOT_DIR%") do set ROOT_DIR=%%~fI
pushd "%ROOT_DIR%"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cmake was not found in PATH.
    popd
    exit /b 1
)

cmake --preset windows-vs2022-release
if errorlevel 1 (
    popd
    exit /b 1
)

cmake --build --preset build-windows-vs2022-release
if errorlevel 1 (
    popd
    exit /b 1
)

ctest --preset test-windows-vs2022-release
if errorlevel 1 (
    popd
    exit /b 1
)

cmake --install out\build\windows-vs2022-release --config Release
if errorlevel 1 (
    popd
    exit /b 1
)

echo.
echo Build completed successfully.
echo Executable: %ROOT_DIR%\out\build\windows-vs2022-release\Release\apex_robotics_studio.exe
popd
exit /b 0
