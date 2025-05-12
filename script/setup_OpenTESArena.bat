@echo off
setlocal enabledelayedexpansion

:: ==========================================
:: OpenTESArena Setup Script
:: ==========================================
echo ==================================
echo OpenTESArena Setup
echo ==================================
echo.

:: ==========================================
:: Check Administrator Privileges
:: ==========================================
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo This script requires administrator privileges.
    echo Please run this script as administrator.
    pause
    exit /b 1
)

:: ==========================================
:: Initialize Paths and Directories
:: ==========================================
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%~dp0..\"
if not exist "logs" mkdir logs

:: ==========================================
:: Visual Studio 2019 Installation
:: ==========================================
echo Checking Visual Studio 2019 installation...
set "vs_path=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe"
if not exist "%vs_path%" (
    echo Visual Studio 2019 is not installed.
    echo Downloading Visual Studio 2019 installer...
    curl -L -o vs_buildtools.exe https://aka.ms/vs/16/release/vs_buildtools.exe
    echo Running VS Build Tools installer...
    start "" vs_buildtools.exe --installPath "%SCRIPT_DIR%BuildTools2019"
) else (
    echo Visual Studio 2019 is already installed.
)

:: ==========================================
:: CMake Installation
:: ==========================================
echo Checking CMake...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo CMake is not installed.
    echo Downloading CMake installer...
    curl -L -o cmake.msi https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-windows-x86_64.msi
    echo Running CMake installer...
    start "" cmake.msi
) else (
    echo CMake is already installed.
)

:: ==========================================
:: Git Installation
:: ==========================================
echo Checking Git...
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo Git is not installed.
    echo Downloading Git installer...
    curl -L -o git.exe https://github.com/git-for-windows/git/releases/download/v2.44.0.windows.1/Git-2.44.0-64-bit.exe
    echo Running Git installer...
    start "" git.exe
) else (
    echo Git is already installed.
)

:: ==========================================
:: vcpkg Setup
:: ==========================================
echo Checking vcpkg...
if not exist "C:\Tools\vcpkg" (
    echo vcpkg is not installed.
    echo Cloning vcpkg repository...
    git clone https://github.com/Microsoft/vcpkg.git C:\Tools\vcpkg
    
    if %errorlevel% neq 0 (
        echo Error cloning vcpkg repository.
        cd /d "%SCRIPT_DIR%"
        pause
        exit /b 1
    )
    
    cd C:\Tools\vcpkg
    echo Running bootstrap-vcpkg.bat...
    call bootstrap-vcpkg.bat
) else (
    echo vcpkg is already installed, updating...
    cd C:\Tools\vcpkg
    git pull
)

:: ==========================================
:: Install Dependencies
:: ==========================================
echo Installing dependencies with vcpkg...
call C:\Tools\vcpkg\vcpkg install sdl2 openal-soft wildmidi --triplet x64-windows > "%SCRIPT_DIR%logs\vcpkg_install.log" 2>&1

if %errorlevel% neq 0 (
    echo Error installing dependencies with vcpkg.
    echo Check the log for more details: %SCRIPT_DIR%logs\vcpkg_install.log
    type "%SCRIPT_DIR%logs\vcpkg_install.log"
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo Integrating vcpkg with Visual Studio...
call C:\Tools\vcpkg\vcpkg integrate install

if %errorlevel% neq 0 (
    echo Error integrating vcpkg with Visual Studio.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

:: ==========================================
:: Build Directory Setup
:: ==========================================
cd /d "%PROJECT_DIR%"
echo Creating build directory...
if exist "build" (
    echo Build directory already exists. Do you want to delete it and create a new one? (Y/N)
    
    if /i "!deletebuild!"=="Y" (
        echo Deleting existing build directory...
        rmdir /s /q build
        
        if %errorlevel% neq 0 (
            echo Error deleting build directory.
            cd /d "%SCRIPT_DIR%"
            pause
            exit /b 1
        )
        
        mkdir build
    )
) else (
    mkdir build
)

cd build

:: ==========================================
:: Build Type Selection
:: ==========================================
echo.
echo Select build type:
echo 1. Debug - For development and debugging (slower, more debug information)
echo 2. ReleaseGenericNoLTO - Release build without Link Time Optimization
echo 3. ReleaseGeneric - Generic optimized build (recommended for maximum compatibility)
echo 4. ReleaseNative - Optimized for your specific CPU (maximum speed, less compatibility)
echo.
set /p compilation_type=Enter your choice (1-4): 

set "BUILD_TYPE=ReleaseGeneric"
if "!compilation_type!"=="1" (
    set "BUILD_TYPE=Debug"
    echo You selected: Debug
) else if "!compilation_type!"=="2" (
    set "BUILD_TYPE=ReleaseGenericNoLTO"
    echo You selected: ReleaseGenericNoLTO
) else if "!compilation_type!"=="3" (
    set "BUILD_TYPE=ReleaseGeneric"
    echo You selected: ReleaseGeneric
) else if "!compilation_type!"=="4" (
    set "BUILD_TYPE=ReleaseNative"
    echo You selected: ReleaseNative
) else (
    echo Invalid option. Using ReleaseGeneric as default.
)

:: ==========================================
:: CPU Feature Detection
:: ==========================================
echo.
echo Do you want to disable CPU-specific optimizations? (AVX2, AVX512, FMA) (Y/N)
set /p disableCPU=

set "CPU_FLAGS="
if /i "!disableCPU!"=="Y" (
    set "CPU_FLAGS=-DUSE_AVX2=OFF -DUSE_AVX512=OFF -DUSE_FMADD=OFF"
    echo CPU optimizations will be disabled: AVX2, AVX512, FMA.
) else (
    echo CPU optimizations will remain active.
)

:: ==========================================
:: CMake Configuration
:: ==========================================
echo Generating project files with CMake...
echo Using: cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=!BUILD_TYPE! !CPU_FLAGS! ..

cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=!BUILD_TYPE! !CPU_FLAGS! ..

if %errorlevel% neq 0 (
    echo Error generating project files with CMake.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo Project files generated successfully.

:: ==========================================
:: Build Process
:: ==========================================
echo Do you want to build the project from the command line? (Y/N)
set /p compileCMD=

if /i "!compileCMD!"=="Y" (
    echo Building with CMake...
    cmake --build . --config !BUILD_TYPE!
    
    if %errorlevel% neq 0 (
        echo Error building the project.
        cd /d "%SCRIPT_DIR%"
        pause
        exit /b 1
    )
    
    echo Build completed successfully.
)

:: ==========================================
:: Arena Path Configuration
:: ==========================================
echo.
echo Please enter the path to your The Elder Scrolls: Arena installation:
echo Example: C:\Program Files (x86)\Steam\steamapps\common\The Elder Scrolls Arena\ARENA
set /p arena_path=

:: Update options-default.txt with the correct paths
echo Updating options-default.txt with correct paths...
powershell -Command "(Get-Content '!EXECUTABLE_DIR!\options\options-default.txt') -replace 'ArenaPaths=.*', 'ArenaPaths=!arena_path!' -replace 'MidiConfig=.*', 'MidiConfig=data/eawpats/timidity.cfg' | Set-Content '!EXECUTABLE_DIR!\options\options-default.txt'"

echo Options file updated successfully with your Arena path.

cd /d "%SCRIPT_DIR%"

:: ==========================================
:: Final Instructions
:: ==========================================
echo.
echo ====================================================
echo Setup and build completed!
echo.
echo IMPORTANT: To run OpenTESArena:
echo 1. You need to have The Elder Scrolls: Arena installed
echo 2. Configure the path in options-default.txt:
echo    - Edit the ArenaPaths= line to point to your ARENA folder
echo    - Example: ArenaPaths=C:\Program Files (x86)\Steam\steamapps\common\The Elder Scrolls Arena\ARENA
echo.
echo 3. Optional: For music, download and install eawpats in the data folder
echo ====================================================

:fin
echo.
echo Setup finished. Press any key to exit...
pause
exit /b 0