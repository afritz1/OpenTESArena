# OpenTESArena build manual for Visual Studio

This document describes how to setup a build environment for building with Visual Studio.

## Setup build environment

1. Install [Visual Studio Community 2019](https://www.visualstudio.com/downloads/) with the C++ for games workload
2. Install [cmake](https://cmake.org/download/) and [git](https://git-scm.com/download)

### Installing vcpkg
The easiest way to build the dependencies is with [vcpkg](https://github.com/Microsoft/vcpkg)

1. Clone [vcpkg](https://github.com/Microsoft/vcpkg) to ```C:/Tools/vcpkg/```:
  ```PowerShell
  cd C:\Tools\vcpkg\
  git clone https://github.com/Microsoft/vcpkg.git .
  ```
2. Run the bootstrapper in the root folder:
  ```PowerShell
  .\bootstrap-vcpkg.bat
  ```

3. Install the dependencies:
  ```PowerShell
  vcpkg install sdl2 openal-soft wildmidi --triplet x64-windows
  ```

## Building

1. Create a `build` directory and move into it
2. Create solution files with `cmake`:

  ```PowerShell
  cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake ..
  ```
3. Open the file OpenTESArena.sln and in Visual Studio, do the following from the menu: Build -> build Solution

4.   The compilation is done when you can read something like the following in the bottom text output window:  
  ```========== Build: succeeded ==========```
