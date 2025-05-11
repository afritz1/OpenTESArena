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
    set /p eliminarbuild=
    
    if /i "!eliminarbuild!"=="Y" (
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
set /p tipo_compilacion=Enter your choice (1-4): 

set "BUILD_TYPE=ReleaseGeneric"
if "!tipo_compilacion!"=="1" (
    set "BUILD_TYPE=Debug"
    echo You selected: Debug
) else if "!tipo_compilacion!"=="2" (
    set "BUILD_TYPE=ReleaseGenericNoLTO"
    echo You selected: ReleaseGenericNoLTO
) else if "!tipo_compilacion!"=="3" (
    set "BUILD_TYPE=ReleaseGeneric"
    echo You selected: ReleaseGeneric
) else if "!tipo_compilacion!"=="4" (
    set "BUILD_TYPE=ReleaseNative"
    echo You selected: ReleaseNative
) else (
    echo Invalid option. Using ReleaseGeneric as default.
)

:: ==========================================
:: CPU Feature Detection
:: ==========================================
echo.
echo Do you want to detect your CPU capabilities or enable all CPU features? (1/2)
echo 1. Detect CPU capabilities (Recommended)
echo 2. Enable all CPU features (May cause illegal instruction errors)
set /p cpu_option=

set "CPU_FLAGS="
if "!cpu_option!"=="1" (
    echo Detecting CPU capabilities...
    
    :: Get CPU information using wmic
    echo Getting CPU information...
    wmic cpu get Name,ProcessorId /format:list > "%TEMP%\cpu_info.txt"
    
    :: Read CPU information and set flags
    set "CPU_FLAGS="
    for /f "tokens=*" %%a in ('type "%TEMP%\cpu_info.txt"') do (
        set "line=%%a"
        
        :: Check for SSE4.1
        echo !line! | findstr /i "SSE4.1" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_SSE4_1=OFF"
        
        :: Check for SSE4.2
        echo !line! | findstr /i "SSE4.2" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_SSE4_2=OFF"
        
        :: Check for AVX
        echo !line! | findstr /i "AVX" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_AVX=OFF"
        
        :: Check for AVX2
        echo !line! | findstr /i "AVX2" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_AVX2=OFF"
        
        :: Check for AVX512
        echo !line! | findstr /i "AVX512" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_AVX512=OFF"
        
        :: Check for LZCNT
        echo !line! | findstr /i "LZCNT" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_LZCNT=OFF"
        
        :: Check for TZCNT
        echo !line! | findstr /i "TZCNT" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_TZCNT=OFF"
        
        :: Check for F16C
        echo !line! | findstr /i "F16C" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_F16C=OFF"
        
        :: Check for FMA
        echo !line! | findstr /i "FMA" >nul
        if errorlevel 1 set "CPU_FLAGS=!CPU_FLAGS! -DUSE_FMADD=OFF"
    )
    
    :: Clean up temporary file
    del "%TEMP%\cpu_info.txt" >nul 2>&1
    
    if "!CPU_FLAGS!"=="" (
        echo Warning: Could not detect CPU features. Disabling all CPU-specific optimizations.
        set "CPU_FLAGS=-DUSE_SSE4_1=OFF -DUSE_SSE4_2=OFF -DUSE_AVX=OFF -DUSE_AVX2=OFF -DUSE_AVX512=OFF -DUSE_LZCNT=OFF -DUSE_TZCNT=OFF -DUSE_F16C=OFF -DUSE_FMADD=OFF"
    ) else (
        echo CPU features detected and configured accordingly.
    )
) else if "!cpu_option!"=="2" (
    echo All CPU features will be enabled. Warning: This may cause illegal instruction errors.
    set "CPU_FLAGS="
) else (
    echo Invalid option. Disabling all CPU-specific optimizations for safety.
    set "CPU_FLAGS=-DUSE_SSE4_1=OFF -DUSE_SSE4_2=OFF -DUSE_AVX=OFF -DUSE_AVX2=OFF -DUSE_AVX512=OFF -DUSE_LZCNT=OFF -DUSE_TZCNT=OFF -DUSE_F16C=OFF -DUSE_FMADD=OFF"
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
:: Visual Studio Integration
:: ==========================================
echo Do you want to open the project in Visual Studio now? (Y/N)
echo Note: If you choose "Y", the script will set "otesa" as the startup project and open Visual Studio.
set /p abrirVS=

if /i "!abrirVS!"=="Y" (
    :: Create StartUp Project configuration file for Visual Studio
    echo Setting 'otesa' as startup project...
    
    if not exist ".vs" mkdir ".vs"
    if not exist ".vs\OpenTESArena" mkdir ".vs\OpenTESArena"
    if not exist ".vs\OpenTESArena\v16" mkdir ".vs\OpenTESArena\v16"
    
    :: Create .suo file for Visual Studio 2019 (v16)
    echo ^<?xml version="1.0" encoding="utf-8"?^> > .vs\OpenTESArena\v16\startup.vs.xml
    echo ^<StartUpProject xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^> >> .vs\OpenTESArena\v16\startup.vs.xml
    echo   ^<Project Name="otesa" /^> >> .vs\OpenTESArena\v16\startup.vs.xml
    echo ^</StartUpProject^> >> .vs\OpenTESArena\v16\startup.vs.xml
    
    :: Also create a backup binary .suo file with PowerShell
    powershell -Command "& { try { $bytes = [byte[]]@(0xEF, 0xBB, 0xBF, 0x0D, 0x0A, 0x3C, 0x53, 0x74, 0x61, 0x72, 0x74, 0x55, 0x70, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x20, 0x4E, 0x61, 0x6D, 0x65, 0x3D, 0x22, 0x6F, 0x74, 0x65, 0x73, 0x61, 0x22, 0x20, 0x2F, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F, 0x53, 0x74, 0x61, 0x72, 0x74, 0x55, 0x70, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x3E); [System.IO.File]::WriteAllBytes('.vs\OpenTESArena\v16\OpenTESArena.suo', $bytes) } catch { Write-Host 'Error creating .suo file' } }"
    
    echo Do you want Visual Studio to automatically build the project when opening? (Y/N)
    set /p autocompile=
    
    if /i "!autocompile!"=="Y" (
        :: Open Visual Studio and automatically run the build
        echo Opening Visual Studio and building automatically...
        start "" "%vs_path%" OpenTESArena.sln /Build "!BUILD_TYPE!|x64"
    ) else (
        :: Just open Visual Studio with the configured project
        echo Opening Visual Studio with 'otesa' set as startup project...
        start "" OpenTESArena.sln
    )
    
    echo.
    echo IMPORTANT: Visual Studio Configuration
    echo - 'otesa' has been automatically set as the startup project
    echo - To build manually: Select Build -^> Build Solution
    echo - Wait for the 'Build: succeeded' message to appear
) else (
    echo Do you want to build the project from the command line? (Y/N)
    set /p compilarCMD=
    
    if /i "!compilarCMD!"=="Y" (
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
)

:: ==========================================
:: Data Files Setup
:: ==========================================
echo Do you want to copy the data and options files needed to run the game? (Y/N)
set /p copiardatos=

if /i "!copiardatos!"=="Y" (
    echo Copying necessary files...
    
    :: Determine the location of the executable based on the build type
    set "EXECUTABLE_DIR=%PROJECT_DIR%build\bin\!BUILD_TYPE!"
    if not exist "!EXECUTABLE_DIR!" (
        set "EXECUTABLE_DIR=%PROJECT_DIR%build\!BUILD_TYPE!"
        if not exist "!EXECUTABLE_DIR!" (
            echo Could not find the executable folder.
            echo Manually search for the executable file and copy the 'data' and 'options' folders to that directory.
        ) else (
            if not exist "!EXECUTABLE_DIR!\data" mkdir "!EXECUTABLE_DIR!\data"
            if not exist "!EXECUTABLE_DIR!\options" mkdir "!EXECUTABLE_DIR!\options"
            
            xcopy /E /Y /I "%PROJECT_DIR%data" "!EXECUTABLE_DIR!\data"
            xcopy /E /Y /I "%PROJECT_DIR%options" "!EXECUTABLE_DIR!\options"
            
            echo Files successfully copied to !EXECUTABLE_DIR!
            
            :: Ask for Arena installation path
            echo.
            echo Please enter the path to your The Elder Scrolls: Arena installation:
            echo Example: C:\Program Files (x86)\Steam\steamapps\common\The Elder Scrolls Arena\ARENA
            set /p arena_path=
            
            :: Update options-default.txt with the correct paths
            echo Updating options-default.txt with correct paths...
            powershell -Command "(Get-Content '!EXECUTABLE_DIR!\options\options-default.txt') -replace 'ArenaPaths=.*', 'ArenaPaths=!arena_path!' -replace 'MidiConfig=.*', 'MidiConfig=data/eawpats/timidity.cfg' | Set-Content '!EXECUTABLE_DIR!\options\options-default.txt'"
            
            echo Options file updated successfully with your Arena path.
        )
    ) else (
        if not exist "!EXECUTABLE_DIR!\data" mkdir "!EXECUTABLE_DIR!\data"
        if not exist "!EXECUTABLE_DIR!\options" mkdir "!EXECUTABLE_DIR!\options"
        
        xcopy /E /Y /I "%PROJECT_DIR%data" "!EXECUTABLE_DIR!\data"
        xcopy /E /Y /I "%PROJECT_DIR%options" "!EXECUTABLE_DIR!\options"
        
        echo Files successfully copied to !EXECUTABLE_DIR!
        
        :: Ask for Arena installation path
        echo.
        echo Please enter the path to your The Elder Scrolls: Arena installation:
        echo Example: C:\Program Files (x86)\Steam\steamapps\common\The Elder Scrolls Arena\ARENA
        set /p arena_path=
        
        :: Update options-default.txt with the correct paths
        echo Updating options-default.txt with correct paths...
        powershell -Command "(Get-Content '!EXECUTABLE_DIR!\options\options-default.txt') -replace 'ArenaPaths=.*', 'ArenaPaths=!arena_path!' -replace 'MidiConfig=.*', 'MidiConfig=data/eawpats/timidity.cfg' | Set-Content '!EXECUTABLE_DIR!\options\options-default.txt'"
        
        echo Options file updated successfully with your Arena path.
    )
)

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