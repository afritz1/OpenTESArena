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

:: Guardar la ruta de ejecución del script
set "SCRIPT_DIR=%~dp0"

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
set "GIT_INSTALLED=0"
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
    set "GIT_INSTALLED=1"
    
    del git.exe
) else (
    echo Git ya esta instalado.
)

:: Refrescar PATH desde el registro
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PATH') do set "PATH=%%b"

:: Verificar que Git funciona correctamente
echo Verificando que Git esté correctamente configurado...
git --version >nul 2>&1

if %errorlevel% neq 0 (
    echo ERROR: Git no está disponible en el PATH.
    echo Por favor:
    echo 1. Cierra y vuelve a abrir la ventana de comandos
    echo 2. Ejecuta este script nuevamente
    echo O alternativamente:
    echo 1. Reinicia tu computadora
    echo 2. Ejecuta este script como administrador nuevamente
    pause
    exit /b 1
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
        cd /d "%SCRIPT_DIR%"
        pause
        exit /b 1
    )
    
    cd vcpkg
    
    echo Ejecutando bootstrap-vcpkg.bat...
    call bootstrap-vcpkg.bat
    
    if %errorlevel% neq 0 (
        echo Error al ejecutar bootstrap-vcpkg.bat.
        cd /d "%SCRIPT_DIR%"
        pause
        exit /b 1
    )
) else (
    echo vcpkg ya esta instalado, actualizando...
    cd vcpkg
    git pull
)

echo Instalando dependencias con vcpkg...
call vcpkg install sdl2 openal-soft wildmidi --triplet x64-windows > "%SCRIPT_DIR%logs\vcpkg_install.log" 2>&1

if %errorlevel% neq 0 (
    echo Error al instalar las dependencias con vcpkg.
    echo Revisa el log para más detalles: %SCRIPT_DIR%logs\vcpkg_install.log
    type "%SCRIPT_DIR%logs\vcpkg_install.log"
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo Integrando vcpkg con Visual Studio...
call vcpkg integrate install

if %errorlevel% neq 0 (
    echo Error al integrar vcpkg con Visual Studio.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

:: Volver a la carpeta donde se ejecutó el script
cd /d "%SCRIPT_DIR%"


echo.
echo ====================================================
echo Configuración completada exitosamente!
echo.
echo Instrucciones para compilar OpenTESArena:
echo 1. Clona el repositorio: git clone https://github.com/afritz1/OpenTESArena.git
echo 2. Abre el proyecto en Visual Studio 2019
echo 3. Configura el proyecto para usar vcpkg
echo 4. Compila la solución
echo ====================================================
echo.

echo ¿Deseas clonar el repositorio de OpenTESArena ahora? (S/N)
set /p clonar=

if /i "%clonar%"=="S" (
    echo Verificando nuevamente que Git está disponible...
    git --version >nul 2>&1
    if %errorlevel% neq 0 (
        echo ERROR: Git sigue sin estar disponible en el PATH.
        echo Por favor reinicia tu computadora y ejecuta este script nuevamente.
        pause
        exit /b 1
    )

    if exist "OpenTESArena" (
        echo Ya existe una carpeta OpenTESArena. ¿Deseas eliminarla y clonar de nuevo? (S/N)
        set /p eliminar=
        
        if /i "!eliminar!"=="S" (
            echo Eliminando carpeta existente...
            rmdir /s /q OpenTESArena
            
            if %errorlevel% neq 0 (
                echo Error al eliminar la carpeta existente.
                pause
                exit /b 1
            )
        ) else (
            echo No se clonará el repositorio.
            goto fin
        )
    )
    
    echo Clonando repositorio OpenTESArena...
    git clone https://github.com/afritz1/OpenTESArena.git
    
    if %errorlevel% neq 0 (
        echo Error al clonar el repositorio con Git.
        echo Intentando método alternativo con PowerShell...
        
        powershell -Command "Start-Process -Wait -NoNewWindow git -ArgumentList 'clone', 'https://github.com/afritz1/OpenTESArena.git'"
        
        if %errorlevel% neq 0 (
            echo Error persistente al clonar el repositorio.
            echo Intentando descarga directa del ZIP...
            
            echo Descargando archivo ZIP del repositorio...
            powershell -Command "& { $ProgressPreference = 'Continue'; try { Invoke-WebRequest -Uri 'https://github.com/afritz1/OpenTESArena/archive/refs/heads/main.zip' -OutFile 'OpenTESArena.zip'; if ($?) { Write-Host 'Descarga exitosa del ZIP!' } else { Write-Host 'Error al descargar el ZIP.' } } catch { Write-Host 'Excepción en descarga: ' + $_.Exception.Message } }"
            
            if exist "OpenTESArena.zip" (
                echo Descomprimiendo el archivo ZIP...
                powershell -Command "& { Add-Type -AssemblyName System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory('OpenTESArena.zip', '.'); }"
                
                if exist "OpenTESArena-main" (
                    echo Renombrando la carpeta...
                    ren "OpenTESArena-main" "OpenTESArena"
                    del OpenTESArena.zip
                    echo Repositorio descargado y descomprimido exitosamente.
                ) else (
                    echo Error al descomprimir el archivo ZIP.
                    cd /d "%SCRIPT_DIR%"
                    pause
                    exit /b 1
                )
            ) else (
                echo Error al descargar el archivo ZIP. 
                echo Por favor, intenta descargar manualmente el repositorio desde:
                echo https://github.com/afritz1/OpenTESArena/archive/refs/heads/main.zip
                cd /d "%SCRIPT_DIR%"
                pause
                exit /b 1
            )
        )
    )
    
    echo.
    echo Repositorio obtenido exitosamente en la carpeta actual: %SCRIPT_DIR%OpenTESArena
    
    echo.
    echo ====================================================
    echo Compilando OpenTESArena
    echo ====================================================
    
    :: Verificar que tenemos el repositorio
    cd /d "%SCRIPT_DIR%"
    if not exist "OpenTESArena" (
        echo Error: La carpeta OpenTESArena no existe. No se puede continuar con la compilación.
        pause
        exit /b 1
    )
    
    cd OpenTESArena
    
    :: Crear directorio de compilación
    echo Creando directorio de compilación...
    if exist "build" (
        echo El directorio build ya existe. ¿Deseas eliminarlo y crear uno nuevo? (S/N)
        set /p eliminarbuild=
        
        if /i "!eliminarbuild!"=="S" (
            echo Eliminando directorio build existente...
            rmdir /s /q build
            
            if %errorlevel% neq 0 (
                echo Error al eliminar el directorio build.
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
    
    :: Generar archivos de proyecto con CMake
    echo Generando archivos de proyecto con CMake...
    echo Usando: cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=ReleaseGeneric ..
    
    cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=ReleaseGeneric ..
    
    if %errorlevel% neq 0 (
        echo Error al generar los archivos de proyecto con CMake.
        cd /d "%SCRIPT_DIR%"
        pause
        exit /b 1
    )
    
    echo Archivos de proyecto generados correctamente.
    
    :: Preguntar si desea abrir en Visual Studio o compilar directamente
    echo ¿Deseas abrir el proyecto en Visual Studio ahora? (S/N)
    echo Nota: Recomendado para editar y configurar el proyecto.
    set /p abrirVS=
    
    if /i "!abrirVS!"=="S" (
        echo Abriendo proyecto en Visual Studio...
        start OpenTESArena.sln
        
        echo.
        echo Para compilar en Visual Studio:
        echo 1. Asegúrate de que 'otesa' esté seleccionado como proyecto de inicio (clic derecho -^> Establecer como proyecto de inicio)
        echo 2. Selecciona Build -^> Build Solution
        echo 3. Espera a que aparezca el mensaje 'Build: succeeded'
    ) else (
        echo ¿Deseas compilar el proyecto desde la línea de comandos? (S/N)
        set /p compilarCMD=
        
        if /i "!compilarCMD!"=="S" (
            echo Compilando con CMake...
            cmake --build . --config ReleaseGeneric
            
            if %errorlevel% neq 0 (
                echo Error al compilar el proyecto.
                cd /d "%SCRIPT_DIR%"
                pause
                exit /b 1
            )
            
            echo Compilación completada exitosamente.
        )
    )
    
    :: Copiar archivos necesarios para la ejecución
    echo ¿Deseas copiar los archivos de datos y opciones necesarios para ejecutar el juego? (S/N)
    set /p copiardatos=
    
    if /i "!copiardatos!"=="S" (
        echo Copiando archivos necesarios...
        
        :: Determinar la ubicación del ejecutable según el tipo de compilación
        set "EXECUTABLE_DIR=%SCRIPT_DIR%OpenTESArena\build\bin\ReleaseGeneric"
        if not exist "!EXECUTABLE_DIR!" (
            set "EXECUTABLE_DIR=%SCRIPT_DIR%OpenTESArena\build\ReleaseGeneric"
            if not exist "!EXECUTABLE_DIR!" (
                echo No se pudo encontrar la carpeta del ejecutable.
                echo Busca manualmente el archivo ejecutable y copia las carpetas 'data' y 'options' a ese directorio.
            ) else (
                if not exist "!EXECUTABLE_DIR!\data" mkdir "!EXECUTABLE_DIR!\data"
                if not exist "!EXECUTABLE_DIR!\options" mkdir "!EXECUTABLE_DIR!\options"
                
                xcopy /E /Y /I "%SCRIPT_DIR%OpenTESArena\data" "!EXECUTABLE_DIR!\data"
                xcopy /E /Y /I "%SCRIPT_DIR%OpenTESArena\options" "!EXECUTABLE_DIR!\options"
                
                echo Archivos copiados exitosamente a !EXECUTABLE_DIR!
            )
        ) else (
            if not exist "!EXECUTABLE_DIR!\data" mkdir "!EXECUTABLE_DIR!\data"
            if not exist "!EXECUTABLE_DIR!\options" mkdir "!EXECUTABLE_DIR!\options"
            
            xcopy /E /Y /I "%SCRIPT_DIR%OpenTESArena\data" "!EXECUTABLE_DIR!\data"
            xcopy /E /Y /I "%SCRIPT_DIR%OpenTESArena\options" "!EXECUTABLE_DIR!\options"
            
            echo Archivos copiados exitosamente a !EXECUTABLE_DIR!
        )
    )
    
    cd /d "%SCRIPT_DIR%"
    
    echo.
    echo ====================================================
    echo Configuración y compilación completadas!
    echo.
    echo IMPORTANTE: Para ejecutar OpenTESArena:
    echo 1. Necesitas tener instalado The Elder Scrolls: Arena
    echo 2. Configura la ruta en options-default.txt:
    echo    - Edita la línea ArenaPaths= para que apunte a tu carpeta ARENA
    echo    - Ej: ArenaPaths=C:\Program Files (x86)\Steam\steamapps\common\The Elder Scrolls Arena\ARENA
    echo.
    echo 3. Opcional: Para tener música, descarga e instala eawpats en la carpeta data
    echo ====================================================
)

:fin
echo.
echo Configuración finalizada. Presiona cualquier tecla para salir...
pause
exit /b 0