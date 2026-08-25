@echo off
setlocal enabledelayedexpansion
cls

:: Navigate to script directory
cd /d "%~dp0"

:: -------------------------------------------------------------------
:: 1. CMAKE AUTO-DETECTION & PORTABLE INSTALLATION
:: -------------------------------------------------------------------
where cmake >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set "CMAKE_BIN=cmake"
    goto msvc_preflight
)

echo [INFO] CMake was not found on your system.
echo [INFO] Setting up a portable local copy of CMake...
echo.

set "PORTABLE_DIR=%~dp0.tools"
set "CMAKE_DIR=%PORTABLE_DIR%\cmake-3.29.0-windows-x86_64"
set "CMAKE_ZIP=%PORTABLE_DIR%\cmake.zip"
set "CMAKE_BIN=%CMAKE_DIR%\bin\cmake.exe"

if exist "%CMAKE_BIN%" (
    echo [INFO] Using existing portable CMake from .tools\
    echo.
    goto msvc_preflight
)

if not exist "%PORTABLE_DIR%" mkdir "%PORTABLE_DIR%"

echo [1/2] Downloading portable CMake...
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/Kitware/CMake/releases/download/v3.29.0/cmake-3.29.0-windows-x86_64.zip' -OutFile '%CMAKE_ZIP%'"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to download CMake. Please check your internet connection.
    pause
    exit /b %ERRORLEVEL%
)

echo [2/2] Extracting CMake...
powershell -Command "Expand-Archive -Path '%CMAKE_ZIP%' -DestinationPath '%PORTABLE_DIR%' -Force"
if exist "%CMAKE_ZIP%" del /f /q "%CMAKE_ZIP%"

if not exist "%CMAKE_BIN%" (
    echo [ERROR] Extraction failed or binary missing.
    pause
    exit /b 1
)

echo [SUCCESS] Portable CMake ready.
echo.

:: -------------------------------------------------------------------
:: 2. MSVC TOOLCHAIN PREFLIGHT
:: -------------------------------------------------------------------
:msvc_preflight
where cl >nul 2>nul
if not errorlevel 1 goto msvc_ready

set "VSWHERE="
for %%V in ("%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe") do if not defined VSWHERE if exist "%%~V" set "VSWHERE=%%~V"

if not defined VSWHERE (
    echo [ERROR] Visual Studio installer discovery tool was not found.
    echo Install Visual Studio or Build Tools with the C++ workload.
    pause
    exit /b 1
)

set "VSINSTALL="
for /f "usebackq delims=" %%V in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do if not defined VSINSTALL set "VSINSTALL=%%V"

if not defined VSINSTALL (
    echo [ERROR] No Visual Studio installation with MSVC C++ tools was found.
    echo Install the Desktop development with C++ workload and Windows SDK.
    pause
    exit /b 1
)

if not exist "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" (
    echo [ERROR] Visual Studio developer command script was not found:
    echo         %VSINSTALL%\Common7\Tools\VsDevCmd.bat
    pause
    exit /b 1
)

echo [INFO] Initializing MSVC from %VSINSTALL%
call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize the MSVC developer environment.
    pause
    exit /b 1
)

where cl >nul 2>nul
if errorlevel 1 (
    echo [ERROR] MSVC was not found.
    echo Install Visual Studio 2022 or Build Tools with:
    echo   Desktop development with C++
    echo   Windows 10/11 SDK
    pause
    exit /b 1
)

:msvc_ready
echo [INFO] MSVC toolchain detected.
echo.

:: -------------------------------------------------------------------
:: 3. BUILD TYPE SELECTION
:: -------------------------------------------------------------------
if /I "%~1"=="debug" set "BUILD_TYPE=Debug"
if /I "%~1"=="release" set "BUILD_TYPE=Release"

if defined BUILD_TYPE goto build_type_selected

:choose_build
echo Select Build Type:
echo [0] Debug
echo [1] Release
echo.
set /p build_choice="Enter choice (0 or 1): "

if "%build_choice%"=="0" set "BUILD_TYPE=Debug"
if "%build_choice%"=="1" set "BUILD_TYPE=Release"
if not defined BUILD_TYPE (
    cls
    echo Invalid choice. Please enter 0 or 1.
    echo.
    goto choose_build
)

:build_type_selected
set "BUILD_DIR=build\%BUILD_TYPE%"

if exist "%BUILD_DIR%\CMakeCache.txt" del /f /q "%BUILD_DIR%\CMakeCache.txt"
if exist "%BUILD_DIR%\CMakeFiles" rmdir /s /q "%BUILD_DIR%\CMakeFiles"

:: -------------------------------------------------------------------
:: 4. CONFIGURE & BUILD
:: -------------------------------------------------------------------
:: Configure CMake out-of-source
"%CMAKE_BIN%" -S . -B "%BUILD_DIR%"
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

:: Build project target
"%CMAKE_BIN%" --build "%BUILD_DIR%" --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b %ERRORLEVEL%
)

:: Clear screen before running executable
cls

:: -------------------------------------------------------------------
:: 5. EXECUTE BINARY
:: -------------------------------------------------------------------
if exist "%BUILD_DIR%\%BUILD_TYPE%\QuantumSimulationStudio.exe" (
    "%BUILD_DIR%\%BUILD_TYPE%\QuantumSimulationStudio.exe"
) else if exist "%BUILD_DIR%\QuantumSimulationStudio.exe" (
    "%BUILD_DIR%\QuantumSimulationStudio.exe"
) else (
    echo [ERROR] Executable not found.
)

pause