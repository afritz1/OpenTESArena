@echo off
setlocal enabledelayedexpansion

echo ==================================
echo OpenTESArena Setup
echo ==================================
echo.

:: Check for administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo This script requires administrator privileges.
    echo Please run this script as administrator.
    pause
    exit /b 1
)

:: Save the script execution path
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%~dp0..\"

:: Create logs directory
if not exist "logs" mkdir logs

:: ----------------------------------------
:: Visual Studio 2019
:: ----------------------------------------
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

:: ----------------------------------------
:: Check CMake
:: ----------------------------------------
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

:: ----------------------------------------
:: Check Git
:: ----------------------------------------
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

:: Check vcpkg
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

:: Install dependencies with vcpkg
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

:: Return to the folder where the script was executed

cd /d "%PROJECT_DIR%"

    
    :: Create build directory
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
    
    :: Select build type
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
    
    :: Ask if CPU-specific optimizations should be disabled
	echo.
	echo Do you want to disable CPU-specific optimizations? (AVX2, AVX512, FMA) (Y/N)
	set /p desactivarCPU=

	set "CPU_FLAGS="
	if /i "!desactivarCPU!"=="Y" (
    		set "CPU_FLAGS=-DUSE_AVX2=OFF -DUSE_AVX512=OFF -DUSE_FMADD=OFF"
    		echo CPU optimizations will be disabled: AVX2, AVX512, FMA.
	) else (
    		echo CPU optimizations will remain active.
	)
    
    :: Generate project files with CMake
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
    
    :: Ask if you want to open in Visual Studio or build directly
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
    
:: Copy files needed for execution
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
        )
    ) else (
        if not exist "!EXECUTABLE_DIR!\data" mkdir "!EXECUTABLE_DIR!\data"
        if not exist "!EXECUTABLE_DIR!\options" mkdir "!EXECUTABLE_DIR!\options"
        
        xcopy /E /Y /I "%PROJECT_DIR%data" "!EXECUTABLE_DIR!\data"
        xcopy /E /Y /I "%PROJECT_DIR%options" "!EXECUTABLE_DIR!\options"
        
        echo Files successfully copied to !EXECUTABLE_DIR!
    )
)
    
    cd /d "%SCRIPT_DIR%"
    
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
)

:fin
echo.
echo Setup finished. Press any key to exit...
pause
exit /b 0