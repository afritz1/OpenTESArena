## Building OpenTESArena with Visual Studio + vcpkg

### Setting up build environment
1. Install the latest [Visual Studio Community](https://www.visualstudio.com/downloads/) with the C++ for Games workload
2. Install [cmake](https://cmake.org/download/) and [git](https://git-scm.com/download)

#### Installing vcpkg
The easiest way to build the dependencies is with [vcpkg](https://github.com/Microsoft/vcpkg).

1. Clone [vcpkg](https://github.com/Microsoft/vcpkg) to `C:/Tools/vcpkg/`:
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
  .\vcpkg install sdl2 openal-soft wildmidi --triplet x64-windows
  ```

### Building
1. Create a `build` directory and navigate into it
2. Create .sln solution files with `cmake`:
  ```PowerShell
  cmake -DCMAKE_TOOLCHAIN_FILE=C:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake ..
  ```
  - **Warning**: Jolt Physics enables CPU features which may cause illegal instruction errors. You can set these `OFF` in CMake ([more information](https://github.com/jrouwe/JoltPhysics/blob/20eedf47c4bf064e740c9de2f638a8c1d57ce2ed/Build/README.md#illegal-instruction-error))
    ```bash
    -DUSE_SSE4_1=OFF -DUSE_SSE4_2=OFF -DUSE_AVX=OFF -DUSE_AVX2=OFF -DUSE_AVX512=OFF -DUSE_LZCNT=OFF -DUSE_TZCNT=OFF -DUSE_F16C=OFF -DUSE_FMADD=OFF
    ```
3. Open the file OpenTESArena.sln in Visual Studio
4. Make sure `otesa` in the solution hierarchy is set as the startup project
5. Select Build -> Build Solution
6. Compilation is done when you can read something like the following in the bottom text output window:
  ```
  ========== Build: succeeded ==========
  ```
