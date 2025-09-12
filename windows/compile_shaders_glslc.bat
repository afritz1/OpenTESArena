@echo off
setlocal enabledelayedexpansion

set SHADERS_DIRECTORY=..\data\shaders\

for %%f in ("%SHADERS_DIRECTORY%*.vert" "%SHADERS_DIRECTORY%*.frag" "%SHADERS_DIRECTORY%*.comp") do (
    set "cmd[%i%]=glslc.exe "%%f" -o "%SHADERS_DIRECTORY%%%~nf.spv""
    echo !cmd[%i%]!
    call !cmd[%i%]!
)

echo.
endlocal
pause
