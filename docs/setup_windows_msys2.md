## OpenTESArena build manual for MSYS2

This document describes how to setup a MYS2 toolchain to build the project in a Windows environment.
Note that this guide is **exclusive for a 64 bits build** and that some steps are optional.

### MSYS2 installation and setup

1. Download and install the last stable version from http://www.msys2.org/.

2. (OPTIONAL) Change language in `.bashrc` to english and restart shell.
It will use Windows' by default.

       export LANG=en_US.UTF-8

3. Update MSYS2's default packages to the latest version.

       $ pacman -Syu
       $ pacman -Su

   **Note:** _Don't worry if after the first step an error informing that the process could not be exited is shown.
   Just restart the shell and continue._

4. Install `mingw` 64bit complete toolchain.

       $ pacman -S mingw-w64-x86_64-toolchain --noconfirm

5. Install `cmake`.

       $ pacman -S mingw-w64-x86_64-cmake --noconfirm

   **Warning:** _Do not install the package named `cmake`, use the one above._

6. (OPTIONAL) Add a `make` alias to `.bashrc`. This is to avoid having to type `mingw32-make` every time.

       alias make="mingw32-make"

### Other requirements

1. (OPTIONAL) Install Git using `pacman`, or just add your current Git to the `PATH`.

       $ pacman -S git --noconfirm

### Building OpenTESArena (x64)

1. Install OpenAL using `pacman`.

        $ pacman -S mingw-w64-x86_64-openal --noconfirm

2. Install SDL2.

        $ pacman -S mingw-w64-x86_64-SDL2 --noconfirm

3. Install WildMIDI manually.

   Since WildMidi is not available though the package manager, it requires a manual installation.
   You can just use the command below to automate the whole process.

        $ pacman -S unzip --noconfirm && \
        wget https://github.com/Mindwerks/wildmidi/releases/download/wildmidi-0.4.2/wildmidi-0.4.2-win64.zip && \
        unzip wildmidi-0.4.2-win64.zip && \
        cp wildmidi-0.4.2-win64/mingw/*.h /mingw64/include/ && \
        cp wildmidi-0.4.2-win64/mingw/*.a /mingw64/lib/

   **Note:** _After that, you can safely delete all the WildMidi files except for `libWildMidi.dll`, which is required to run the binary._

4. Clone `OpenTESArena` and move to the newly-created directory.

        $ git clone https://github.com/afritz1/OpenTESArena.git
        $ cd OpenTESArena

5. Create a `build` directory and move into it.

        $ mkdir build && cd build

6. Create make files with `cmake`.

        $ cmake \
          -D CMAKE_MAKE_PROGRAM="mingw32-make" \
          -D CMAKE_BUILD_TYPE=Release \
          -G "Unix Makefiles" ..

   **Note:** _Both "Unix Makefiles" and "MingGW Makefiles" work.
   However, the Unix option is simpler to use since it does not require `sh` to be removed from the shell path._

7. Use the `make` alias to build.
When the process completes, you'll find the executable inside the `build` directory.

       $ make

### Deploying and running OpenTESArena

1. Copy the generated executable to a directory of your choice.

2. Follow the instructions in https://github.com/afritz1/OpenTESArena#options-files regarding the setup of the ARENA dir, options-defaults.txt, and MIDI accordingly.

   Remember that the Midi setup is only required if you want to have sound.

3. Copy the `data` and `options` directories and all their contents from source code folder to the location where you copied the executable.

4. Copy the required libraries to the deployment directory.

   All of them are found inside the `msys64/bin` directory except for `libWildMidi.dll`, which is inside the WildMidi zip:

   * libgcc_s_seh-1.dll
   * libopenal-1.dll
   * libstdc++-6.dll
   * libWildMidi.dll
   * libwinpthread-1.dll
   * SDL2.dll
