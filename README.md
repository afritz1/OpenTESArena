# OpenTESArena

[![GitHub release](https://img.shields.io/github/release/afritz1/OpenTESArena/all.svg)](https://github.com/afritz1/OpenTESArena/releases/latest)
[![MIT License](https://img.shields.io/badge/license-MIT-green)](LICENSE.txt) 
[![Discord](https://img.shields.io/discord/395739926831824908.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/DgHe2jG)
[![YouTube Channel Views](https://img.shields.io/youtube/channel/views/UCJpmkgtHRIxR7aOpi909GKw)](https://www.youtube.com/channel/UCJpmkgtHRIxR7aOpi909GKw)

This open-source project aims to be a modern engine re-implementation for "The Elder Scrolls: Arena" from 1994 by Bethesda Softworks. It is written in C++17 and uses SDL2, WildMIDI for music, and OpenAL Soft for sound and mixing. There is support for Windows, Linux, and macOS.

## Current status [![Build Status](https://travis-ci.org/afritz1/OpenTESArena.svg?branch=master)](https://travis-ci.org/afritz1/OpenTESArena)

No actual gameplay yet, but all locations and interiors can be accessed for testing. Citizens wander around cities and the wilderness but cannot be interacted with. Fast traveling works and you can go to any city or dungeon on the world map. Collision detection is barebones (just enough for playtesting) and needs work. Character creation works but character questions and player attributes are not implemented. Some of the in-game interface icons work; for example, left clicking the map icon goes to the automap, and right clicking it goes to the world map.

Controls:
- WASD - move and turn. Hold LCtrl to strafe.
- Esc - pause menu
- Tab - character sheet
- F - draw/sheathe weapon
- G - hold and click voxel to destroy
- L - logbook
- M - world map
- N - automap
- V - status
- F2 - player position
- F4 - debug profiler
- PrintScreen - screenshot

<br/>

![Preview](Preview.PNG)
<br/>

## Project Details

Inspired by [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/), this started out as a simplistic ray tracing tech demo in early 2016, and is now steadily inching closer to something akin to the original game. I am using a clean-room approach for understanding Arena's algorithms and data structures, the details of which can be found in the [wiki](https://github.com/afritz1/OpenTESArena/wiki). It is a behavioral approximation project, not about matching machine instructions, and quality-of-life changes are made where they make sense.

There are two versions of Arena: the floppy disk version (which Bethesda released for free) and the CD version. The user must acquire their own copy of Arena because OpenTESArena is a standalone engine and does not contain content.

Check out [CONTRIBUTING.md](CONTRIBUTING.md) for more details on how to assist with development.

## Installation

If you would like music played in-game, see **Music setup** below after installing. The engine uses `ArenaPath` and `MidiConfig` from the options file to find where the game files and MIDI config are.

### Windows
1. Get the most recent build from the [releases](https://github.com/afritz1/OpenTESArena/releases) tab.
1. Get the Arena data files from one of:
   1. [Download the Full Game](http://static.elderscrolls.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip) from the Bethesda website (floppy disk version)
   1. Bethesda Launcher: `C:/Program Files (x86)/Bethesda.net Launcher/Games/The Elder Scrolls Arena` (floppy disk version)
   1. [GOG](https://www.gog.com/wishlist/games/the_elder_scrolls_arena) (CD version)
1. Extract Arena106Setup.zip and run Arena106.exe.
1. Pick a destination folder to install into. This can be anywhere on your hard drive or in the OpenTESArena `data` folder.
1. Open `options-default.txt` in the `options` folder and change `ArenaPath` to where you put the `ARENA`/`ARENACD` folder.
1. Run OpenTESArena.exe.

If you see an error about missing MSVCP141.dll or similar, download and run the [Visual C++ Redistributable Installer](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads) for Windows:
- 64-bit (recommended): vc_redist.x64.exe
- 32-bit: vc_redist.x86.exe

If you see a warning about `alcOpenDevice()` failing, or there is no sound, download the [OpenAL 1.1 Windows Installer](https://www.openal.org/downloads/) and run oalinst.exe.

### macOS
#### Get the data files for *The Elder Scrolls: Arena*
1. [Download the Full Game](http://static.elderscrolls.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip) from the Bethesda website.
1. Unzip `Arena106Setup.zip`
1. `Arena106.exe` is a self-extracting RAR file. Use a tool such as [The Unarchiver](https://theunarchiver.com) to extract it into a folder of data files.

#### Install OpenTESArena
1. Download the most recent build from the [releases](https://github.com/afritz1/OpenTESArena/releases) tab.
1. Open the `.dmg` and copy OpenTESArena to the `Applications` folder or another location
1. Right-click on the app and choose "Show Package Contents"
1. Navigate to `Contents/Resources/data` and copy in the files for Arena that you extracted earlier
1. Return to the `Applications` folder or wherever you have the app installed and open OpenTESArena. If you have Gatekeeper turned on (the default for macOS), you will need to do the following:
   1. Right-click on the app and choose "Open"
   1. In the warning that appears saying that it is from an unidentified developer, choose "Open"
   1. The app will start. In the future, you can just double-click on the app without having to go through these steps.

### Linux (Ubuntu 16.04)
Substitute `<version>` with the current version number (`0.#.0`), and `<arch>` with the desired architecture (`32` or `64`).
```bash
sudo apt-get install wget unzip rar
wget https://cdnstatic.bethsoft.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip
wget https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-<version>/opentesarena-<version>-Linux<arch>.tar.gz
tar xvzf opentesarena-<version>-Linux<arch>.tar.gz
cd opentesarena-<version>-Linux<arch>/data
unzip ../../Arena106Setup.zip
rar x Arena106.exe
cd ..
./run.sh
```

### Options files
`options-default.txt` comes with releases and stores default settings. `options-changes.txt` is generated in your user prefs folder and stores user-specific settings, and you can either create it yourself or let the program create it. For now, you can change things like `ArenaPath` in `options-default.txt`, but in the future, a wizard will take care of this instead. The prefs folders are:
- Windows: `<username>/AppData/Roaming/OpenTESArena/options/`
- Linux: `~/.config/OpenTESArena/options/`
- macOS: `~/Library/Preferences/OpenTESArena/options/`

### Music setup
Arena uses MIDI files for music, so the user must have MIDI sound patches in order to have music play in-game.

The easiest way is to download one of the eawpats packages ([zip](https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-0.1.0/eawpats.zip), [tar.gz](https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-0.1.0/eawpats.tar.gz)) and place the extracted eawpats folder into the OpenTESArena `data` folder.

If you would like to use a different sound patches library like OPL3, edit `MidiConfig` in the options file to point to the MIDI `.cfg` file for that library.

## Building from source

### Project dependencies
- [CMake](https://cmake.org/download/)
- [OpenAL Soft 1.18.2](https://openal-soft.org/#download)
- [SDL 2.0.4](https://www.libsdl.org/download-2.0.php)
- [WildMIDI 0.4.3](https://github.com/Mindwerks/wildmidi/releases) (optional; required for music)

### Building the executable
- Navigate to the root of the repository
- Use CMake to generate your project file (Visual Studio .sln, Unix Makefile, etc.). In a Unix terminal, the commands might look like:
    ```bash
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=<?> ..
    make -j8
    ```
    where `CMAKE_BUILD_TYPE` is one of `Debug`|`DebugFast`|`ReleaseGeneric`|`ReleaseNative`. For maximum optimizations, `ReleaseNative` should be used.
- Other parameters for CMake may be necessary depending on the IDE you are using.

### Running the executable
- Verify that the `data` and `options` folders are in the same folder as the executable. If not, then copy them from the project's root folder (this should be fixed in the future with a post-build command).
- Make sure that `MidiConfig` and `ArenaPath` in the options file point to valid locations on your computer (i.e., `data/eawpats/timidity.cfg` and `data/ARENA` respectively).

If you struggle, here are some more detailed guides:
- [Building with Visual Studio (Windows)](docs/setup_windows.md)  
- [Building with MSYS2 (Windows)](docs/setup_windows_msys2.md)

If there is a bug or technical problem in the program, check out the issues tab!

## Resources

The [Unofficial Elder Scrolls Pages](http://en.uesp.net/wiki/Arena:Arena) are a great resource for information all about Arena. [Various tools](http://en.uesp.net/wiki/Arena:Files#Misc_Utilities) like WinArena and BSATool allow for browsing Arena's content, and there is a very detailed [manual](http://en.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities) as well. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a humorous overview of the game's history and gameplay.
