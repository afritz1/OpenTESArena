## OpenTESArena build manual for MSYS2

This document describes how to setup a MYS2 toolchain to build the project in a Windows environment.
Note that this guide is **exclusive for a 64 bits build** and that some steps are optional.

### MSYS2 installation and setup

1. Download and install the last stable version from http://www.msys2.org/.

2. (OPTIONAL) Change language in `.bashrc` to english and restart shell.
It will use Window's by default.

       export LANG=en_US.UTF-8

3. Update MSYS2 default packages to the last version.

       $ pacman -Syu
       $ pacman -Su

   **Note:** _Don't worry if after the first step an error informing that the process could not be exited is shown.
   Just restart the shell and continue._

4. Install `mingw` 64bits complete toolchain.

       $ pacman -S mingw-w64-x86_64-toolchain --noconfirm

5. Install `cmake`.

       $ pacman -S mingw-w64-x86_64-cmake --noconfirm

   **Warning:** _Do not install the package named `cmake`, use the one above._

6. (OPTIONAL) Add `make` alias to `.bashrc`. This is to avoid having to type `mingw32-make` every time.

       alias make="mingw32-make"

### Other requirements

1. (OPTIONAL) Install Git from a package using `pacman`, or just add your current Git to the `PATH`.

       $ pacman -S git --noconfirm

### Building OpenTESArena (64x)

1. Install OpenAL using `pacman`, this will install the last current version (1.18.2).

        $ pacman -S mingw-w64-x86_64-openal --noconfirm

2. Install SDL2 (current version  2.0.8).

        $ pacman -S mingw-w64-x86_64-SDL2 --noconfirm

3. Install WildMIDI manually (current version 0.4.1).

   Since WildMidi is not available though package manager, it requires a manual installation.
   You can just use the command below to automate the whole process.

        $ pacman -S unzip --noconfirm && \
        wget https://github.com/Mindwerks/wildmidi/releases/download/wildmidi-0.4.2/wildmidi-0.4.2-win64.zip && \
        unzip wildmidi-0.4.2-win64.zip && \
        cp wildmidi-0.4.2-win64/mingw/*.h /mingw64/include/ && \
        cp wildmidi-0.4.2-win64/mingw/*.a /mingw64/lib/

   **Note:** _After that, you can safely delete all the WildMidi files except for `libWildMidi.dll`, which is required to run the binary._

4. Clone `OpenTESArena` and move to the directory.

        $ git clone https://github.com/afritz1/OpenTESArena.git
        $ cd OpenTESArena

5. Create `build` directory and move into it.

        $ mkdir build && cd build

6. Create make files with `cmake`.

        $ cmake \
          -D CMAKE_MAKE_PROGRAM="mingw32-make" \
          -D CMAKE_BUILD_TYPE=Release \
          -G "Unix Makefiles" ..

   **Note:** _Both "Unix Makefiles" and "MingGW Makefiles" work.
   However, Unix option is simpler to use since it does not require to remove `sh` from the shell path._

7. Use `make` alias to build.
Process should complete and you'll find the `.exe` file inside the `build` directory.

       $ make

### Deploying and running OpenTESArena

1. Copy the generated `.exe` to a directory of your choice.

2. Follow instruction in https://github.com/afritz1/OpenTESArena#options-files regarding setup of ARENA dir, options-defaults.txt and MIDI accordingly.

   Remember that Midi setup is only required if you want to have sound.

3. Copy `data` and `options` directories and all its contents from source code to the location where you copied the `.exe`.

4. Copy the required libraries to the deployment directory.

   All of them are found inside the `msys64/bin` directory except for `libWildMidi.dll`, which is inside the WildMidi zip:

   * libgcc_s_seh-1.dll
   * libopenal-1.dll
   * libstdc++-6.dll
   * libWildMidi.dll
   * libwinpthread-1.dll
   * SDL2.dll
