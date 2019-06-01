# OpenTESArena build manual for Visual Studio

This document describes how to setup a build environment for building with Visual Studio

## Setup build environment

1. Install [Visual Studio Community 2019](https://www.visualstudio.com/downloads/) with the C++ for games workload
2. Install [cmake](https://cmake.org/download/) and [git](https://git-scm.com/download)

## Building

1. Create `build` directory
2. Create solution files with `cmake`:

  ```PowerShell
  cmake ..
  ```
3. Open the file OpenTESArena.sln and in Visual Studio, do the following: from the menu: Build -> build Solution

4.   The compilation is done when you can read something like the following in the bottom text output window:  
  ```========== Build: succeeded ==========```
