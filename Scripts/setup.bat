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
set "PROJECT_DIR=%~dp0..\"

:: Crear directorio para logs
if not exist "logs" mkdir logs

:: ----------------------------------------
:: Visual Studio 2019
:: ----------------------------------------
echo Verificando instalacion de Visual Studio 2019...
set "vs_path=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe"
if not exist "%vs_path%" (
    echo Visual Studio 2019 no esta instalado.

    echo Descargando instalador de Visual Studio 2019...
    curl -L -o vs_buildtools.exe https://aka.ms/vs/16/release/vs_buildtools.exe

    echo Ejecutando instalador de VS Build Tools...
    start "" vs_buildtools.exe --installPath "%SCRIPT_DIR%BuildTools2019"
) else (
    echo Visual Studio 2019 ya esta instalado.
)

:: ----------------------------------------
:: Verificar CMake
:: ----------------------------------------
echo Verificando CMake...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo CMake no esta instalado.

    echo Descargando instalador de CMake...
    curl -L -o cmake.msi https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-windows-x86_64.msi

    echo Ejecutando instalador de CMake...
    start "" cmake.msi
) else (
    echo CMake ya esta instalado.
)

:: ----------------------------------------
:: Verificar Git
:: ----------------------------------------
echo Verificando Git...
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo Git no esta instalado.

    echo Descargando instalador de Git...
    curl -L -o git.exe https://github.com/git-for-windows/git/releases/download/v2.44.0.windows.1/Git-2.44.0-64-bit.exe

    echo Ejecutando instalador de Git...
    start "" git.exe
) else (
    echo Git ya esta instalado.
)

:: Verificar vcpkg
echo Verificando vcpkg...
if not exist "C:\Tools\vcpkg" (
    echo vcpkg no esta instalado.

    echo Clonando repositorio de vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git C:\Tools\vcpkg
    
    if %errorlevel% neq 0 (
        echo Error al clonar el repositorio de vcpkg.
        cd /d "%SCRIPT_DIR%"
        pause
        exit /b 1
    )
    
    cd C:\Tools\vcpkg
    echo Ejecutando bootstrap-vcpkg.bat...
    call bootstrap-vcpkg.bat
) else (
    echo vcpkg ya esta instalado, actualizando...
    cd C:\Tools\vcpkg
    git pull
)

:: Instalar dependencias con vcpkg
echo Instalando dependencias con vcpkg...
call C:\Tools\vcpkg\vcpkg install sdl2 openal-soft wildmidi --triplet x64-windows > "%SCRIPT_DIR%logs\vcpkg_install.log" 2>&1

if %errorlevel% neq 0 (
    echo Error al instalar las dependencias con vcpkg.
    echo Revisa el log para más detalles: %SCRIPT_DIR%logs\vcpkg_install.log
    type "%SCRIPT_DIR%logs\vcpkg_install.log"
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo Integrando vcpkg con Visual Studio...
call C:\Tools\vcpkg\vcpkg integrate install

if %errorlevel% neq 0 (
    echo Error al integrar vcpkg con Visual Studio.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

:: Volver a la carpeta donde se ejecutó el script

cd /d "%PROJECT_DIR%"

    
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
    
    :: Seleccionar tipo de compilación
    echo.
    echo Selecciona el tipo de compilación:
    echo 1. Debug - Para desarrollo y depuración (más lento, más información de debug)
    echo 2. ReleaseGenericNoLTO - Compilación para lanzamiento sin optimización de enlace
    echo 3. ReleaseGeneric - Compilación genérica optimizada (recomendada para máxima compatibilidad)
    echo 4. ReleaseNative - Optimizada para tu CPU específica (máxima velocidad, menor compatibilidad)
    echo.
    set /p tipo_compilacion=Ingresa el número de tu elección (1-4): 

    set "BUILD_TYPE=ReleaseGeneric"
    if "!tipo_compilacion!"=="1" (
        set "BUILD_TYPE=Debug"
        echo Has seleccionado: Debug
    ) else if "!tipo_compilacion!"=="2" (
        set "BUILD_TYPE=ReleaseGenericNoLTO"
        echo Has seleccionado: ReleaseGenericNoLTO
    ) else if "!tipo_compilacion!"=="3" (
        set "BUILD_TYPE=ReleaseGeneric"
        echo Has seleccionado: ReleaseGeneric
    ) else if "!tipo_compilacion!"=="4" (
        set "BUILD_TYPE=ReleaseNative"
        echo Has seleccionado: ReleaseNative
    ) else (
        echo Opción no válida. Usando ReleaseGeneric por defecto.
    )
    
    :: Preguntar si se desean desactivar las especificaciones del CPU
	echo.
	echo ¿Deseas desactivar optimizaciones específicas del CPU? (AVX2, AVX512, FMA) (S/N)
	set /p desactivarCPU=

	set "CPU_FLAGS="
	if /i "!desactivarCPU!"=="S" (
    		set "CPU_FLAGS=-DUSE_AVX2=OFF -DUSE_AVX512=OFF -DUSE_FMADD=OFF"
    		echo Se desactivarán optimizaciones del CPU: AVX2, AVX512, FMA.
	) else (
    		echo Se mantendrán las optimizaciones del CPU activas.
	)
    
    :: Generar archivos de proyecto con CMake
    echo Generando archivos de proyecto con CMake...
    echo Usando: cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=!BUILD_TYPE! !CPU_FLAGS! ..

    

    cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=!BUILD_TYPE! !CPU_FLAGS! ..

    
    if %errorlevel% neq 0 (
        echo Error al generar los archivos de proyecto con CMake.
        cd /d "%SCRIPT_DIR%"
        pause
        exit /b 1
    )
    
    echo Archivos de proyecto generados correctamente.
    
    :: Preguntar si desea abrir en Visual Studio o compilar directamente
    echo ¿Deseas abrir el proyecto en Visual Studio ahora? (S/N)
    echo Nota: Si eliges "S", el script configurará "otesa" como proyecto de inicio y abrirá Visual Studio.
    set /p abrirVS=
    
    if /i "!abrirVS!"=="S" (
        :: Crear archivo de configuración de StartUp Project para Visual Studio
        echo Configurando 'otesa' como proyecto de inicio...
        
        if not exist ".vs" mkdir ".vs"
        if not exist ".vs\OpenTESArena" mkdir ".vs\OpenTESArena"
        if not exist ".vs\OpenTESArena\v16" mkdir ".vs\OpenTESArena\v16"
        
        :: Crear archivo .suo para Visual Studio 2019 (v16)
        echo ^<?xml version="1.0" encoding="utf-8"?^> > .vs\OpenTESArena\v16\startup.vs.xml
        echo ^<StartUpProject xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^> >> .vs\OpenTESArena\v16\startup.vs.xml
        echo   ^<Project Name="otesa" /^> >> .vs\OpenTESArena\v16\startup.vs.xml
        echo ^</StartUpProject^> >> .vs\OpenTESArena\v16\startup.vs.xml
        
        :: También crear un archivo .suo binario de respaldo con PowerShell
        powershell -Command "& { try { $bytes = [byte[]]@(0xEF, 0xBB, 0xBF, 0x0D, 0x0A, 0x3C, 0x53, 0x74, 0x61, 0x72, 0x74, 0x55, 0x70, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x20, 0x4E, 0x61, 0x6D, 0x65, 0x3D, 0x22, 0x6F, 0x74, 0x65, 0x73, 0x61, 0x22, 0x20, 0x2F, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F, 0x53, 0x74, 0x61, 0x72, 0x74, 0x55, 0x70, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x3E); [System.IO.File]::WriteAllBytes('.vs\OpenTESArena\v16\OpenTESArena.suo', $bytes) } catch { Write-Host 'Error al crear archivo .suo' } }"
        
        echo ¿Deseas que Visual Studio compile automáticamente el proyecto al abrirlo? (S/N)
        set /p autocompile=
        
        if /i "!autocompile!"=="S" (
            :: Abre Visual Studio y ejecuta automáticamente la compilación
            echo Abriendo Visual Studio y compilando automáticamente...
            start "" "%vs_path%" OpenTESArena.sln /Build "!BUILD_TYPE!|x64"
        ) else (
            :: Solo abre Visual Studio con el proyecto configurado
            echo Abriendo Visual Studio con 'otesa' configurado como proyecto de inicio...
            start "" OpenTESArena.sln
        )
        
        echo.
        echo IMPORTANTE: Configuración en Visual Studio
        echo - Se ha configurado 'otesa' como proyecto de inicio automáticamente
        echo - Para compilar manualmente: Selecciona Build -^> Build Solution
        echo - Espera a que aparezca el mensaje 'Build: succeeded'
    ) else (
        echo ¿Deseas compilar el proyecto desde la línea de comandos? (S/N)
        set /p compilarCMD=
        
        if /i "!compilarCMD!"=="S" (
            echo Compilando con CMake...
            cmake --build . --config !BUILD_TYPE!
            
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
    set "EXECUTABLE_DIR=%PROJECT_DIR%build\bin\!BUILD_TYPE!"
    if not exist "!EXECUTABLE_DIR!" (
        set "EXECUTABLE_DIR=%PROJECT_DIR%build\!BUILD_TYPE!"
        if not exist "!EXECUTABLE_DIR!" (
            echo No se pudo encontrar la carpeta del ejecutable.
            echo Busca manualmente el archivo ejecutable y copia las carpetas 'data' y 'options' a ese directorio.
        ) else (
            if not exist "!EXECUTABLE_DIR!\data" mkdir "!EXECUTABLE_DIR!\data"
            if not exist "!EXECUTABLE_DIR!\options" mkdir "!EXECUTABLE_DIR!\options"
            
            xcopy /E /Y /I "%PROJECT_DIR%data" "!EXECUTABLE_DIR!\data"
            xcopy /E /Y /I "%PROJECT_DIR%options" "!EXECUTABLE_DIR!\options"
            
            echo Archivos copiados exitosamente a !EXECUTABLE_DIR!
        )
    ) else (
        if not exist "!EXECUTABLE_DIR!\data" mkdir "!EXECUTABLE_DIR!\data"
        if not exist "!EXECUTABLE_DIR!\options" mkdir "!EXECUTABLE_DIR!\options"
        
        xcopy /E /Y /I "%PROJECT_DIR%data" "!EXECUTABLE_DIR!\data"
        xcopy /E /Y /I "%PROJECT_DIR%options" "!EXECUTABLE_DIR!\options"
        
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