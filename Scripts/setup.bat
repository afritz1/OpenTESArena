@echo off
setlocal enabledelayedexpansion

echo ==================================
echo Configuracion de OpenTESArena
echo ==================================
echo.

:: Verificar privilegios de administrador
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Este script requiere privilegios de administrador.
    echo Por favor, ejecuta este script como administrador.
    pause
    exit /b 1
)

:: Crear directorio para logs
if not exist "logs" mkdir logs

:: Verificar Visual Studio
echo Verificando instalacion de Visual Studio 2019...
set "vs_path=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe"
if not exist "%vs_path%" (
    echo Visual Studio 2019 Community no esta instalado.
    
    echo Descargando el instalador de Visual Studio 2019...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://aka.ms/vs/16/release/vs_community.exe' -OutFile 'vs_community.exe'}"
    
    if %errorlevel% neq 0 (
        echo Error al descargar Visual Studio 2019.
        pause
        exit /b 1
    )
    
    echo Instalando Visual Studio 2019 con componentes necesarios...
    start /wait vs_community.exe --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --includeRecommended --wait --passive
    
    if %errorlevel% neq 0 (
        echo Error al instalar Visual Studio 2019.
        echo Verifica el log de errores en: %temp%\dd_vs_community_*.log
        pause
        exit /b 1
    )
    
    del vs_community.exe
) else (
    echo Visual Studio 2019 Community ya esta instalado.
)

:: Verificar CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo CMake no esta instalado. Instalando...
    
    echo Descargando el instalador de CMake...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0-windows-x86_64.msi' -OutFile 'cmake.msi'}"
    
    if %errorlevel% neq 0 (
        echo Error al descargar CMake.
        pause
        exit /b 1
    )
    
    echo Instalando CMake...
    start /wait msiexec /i cmake.msi /quiet /qn /norestart
    
    if %errorlevel% neq 0 (
        echo Error al instalar CMake.
        pause
        exit /b 1
    )
    
    setx PATH "%PATH%;C:\Program Files\CMake\bin" /M
    set "PATH=%PATH%;C:\Program Files\CMake\bin"
    
    del cmake.msi
) else (
    echo CMake ya esta instalado.
)

:: Verificar Git
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo Git no esta instalado. Instalando...
    
    echo Descargando el instalador de Git...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/git-for-windows/git/releases/download/v2.37.0.windows.1/Git-2.37.0-64-bit.exe' -OutFile 'git.exe'}"
    
    if %errorlevel% neq 0 (
        echo Error al descargar Git.
        pause
        exit /b 1
    )
    
    echo Instalando Git...
    start /wait git.exe /VERYSILENT /NORESTART
    
    if %errorlevel% neq 0 (
        echo Error al instalar Git.
        pause
        exit /b 1
    )
    
    setx PATH "%PATH%;C:\Program Files\Git\cmd" /M
    set "PATH=%PATH%;C:\Program Files\Git\cmd"
    
    del git.exe
) else (
    echo Git ya esta instalado.
)

:: Configurar vcpkg
echo Configurando vcpkg en C:\Tools\vcpkg...

if not exist "C:\Tools" mkdir "C:\Tools"
cd "C:\Tools"

if not exist "C:\Tools\vcpkg" (
    echo Clonando repositorio de vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git vcpkg
    
    if %errorlevel% neq 0 (
        echo Error al clonar el repositorio de vcpkg.
        cd /d "%~dp0"
        pause
        exit /b 1
    )
    
    cd vcpkg
    
    echo Ejecutando bootstrap-vcpkg.bat...
    call bootstrap-vcpkg.bat
    
    if %errorlevel% neq 0 (
        echo Error al ejecutar bootstrap-vcpkg.bat.
        cd /d "%~dp0"
        pause
        exit /b 1
    )
) else (
    echo vcpkg ya esta instalado, actualizando...
    cd vcpkg
    git pull
)

echo Instalando dependencias con vcpkg...
call vcpkg install sdl2 openal-soft wildmidi --triplet x64-windows > "%~dp0logs\vcpkg_install.log" 2>&1

if %errorlevel% neq 0 (
    echo Error al instalar las dependencias con vcpkg.
    echo Revisa el log para más detalles: %~dp0logs\vcpkg_install.log
    type "%~dp0logs\vcpkg_install.log"
    cd /d "%~dp0"
    pause
    exit /b 1
)

echo Integrando vcpkg con Visual Studio...
call vcpkg integrate install

if %errorlevel% neq 0 (
    echo Error al integrar vcpkg con Visual Studio.
    cd /d "%~dp0"
    pause
    exit /b 1
)

cd /d "%~dp0"

echo.
echo ====================================================
echo Configuración completada exitosamente!
echo.
echo Instrucciones para compilar OpenTESArena:
echo 1. Clona el repositorio: git clone https://github.com/OpenTESArena/OpenTESArena.git
echo 2. Abre el proyecto en Visual Studio 2019
echo 3. Configura el proyecto para usar vcpkg
echo 4. Compila la solución
echo ====================================================
echo.

echo ¿Deseas clonar el repositorio de OpenTESArena ahora? (S/N)
set /p clonar=

if /i "%clonar%"=="S" (
    echo Clonando repositorio OpenTESArena...
    git clone https://github.com/OpenTESArena/OpenTESArena.git
    
    if %errorlevel% neq 0 (
        echo Error al clonar el repositorio de OpenTESArena.
        pause
        exit /b 1
    )
    
    echo.
    echo Repositorio clonado exitosamente.
    echo Ahora puedes abrir el proyecto en Visual Studio 2019.
)

echo.
echo Configuración finalizada. Presiona cualquier tecla para salir...
pause
exit /b 0