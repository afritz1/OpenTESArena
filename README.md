# OpenTESArena

This open-source project aims to be a modern engine re-implementation for "The Elder Scrolls: Arena" by Bethesda Softworks. It is written in C++17 and uses SDL2 for cross-platform video, WildMIDI for music, and OpenAL Soft for sound and mixing. There is currently support for Windows, Linux, and macOS.

- Version: 0.11.0
- License: MIT
- Discord: https://discord.gg/DgHe2jG

## Current status [![Build Status](https://travis-ci.org/afritz1/OpenTESArena.svg?branch=master)](https://travis-ci.org/afritz1/OpenTESArena)

No actual gameplay yet, but all cities, main quest dungeons, random dungeons, interior locations, and wilderness can be accessed from test options on the main menu.

Doors and transition blocks can be interacted with, and you can enter and exit buildings.

Fast traveling works and you can go to any city or main quest dungeon on the world map.

Collision detection is partially implemented (you can't jump or fall yet).

A few menus work, including some of character creation, and some of the game interface icons work, too. For example, left clicking the map icon goes to the automap, and right clicking it goes to the world map.

Here are some keys in the game world:
- WASD - move and turn. Hold LCtrl to strafe with A and D.
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

The concept began after I saw the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). It really started out more as an experiment than a remake, but now the project is steadily inching closer to something akin to the original.

Note that there are two versions of Arena: the floppy disk version (which Bethesda released for free) and the CD version. The user must acquire their own copy of Arena because OpenTESArena is just an engine and does not contain any content.

OpenTESArena is licensed under the MIT license. See [LICENSE.txt](LICENSE.txt) for details.

Check out [CONTRIBUTING.md](CONTRIBUTING.md) for more details on how to assist with development.

## Installation

If you would like music played in-game, see **Music setup** below. The engine uses `ArenaPath` and `MidiConfig` from the options file to find where the game files and MIDI config are.

### Windows
- Get the most recent build from the [releases](https://github.com/afritz1/OpenTESArena/releases) tab.
- [Download the Full Game](http://static.elderscrolls.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip) from the Bethesda website.
- Extract Arena106Setup.zip and run Arena106.exe.
- Pick a destination folder anywhere and install.
- Point `ArenaPath` in the options file to the `ARENA` folder.
- Run OpenTESArena.exe.

If you receive an error about missing MSVCP141.dll, you will need to download and run the Visual C++ 2017 Redistributable installer from [here](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads) for your desired target (vc_redist.x64.exe for 64-bit, vc_redist.x86.exe for 32-bit). Similarly, MSVCP140.dll for Visual C++ 2015 is available [here](https://www.microsoft.com/en-us/download/details.aspx?id=48145).

If you see a warning about `alcOpenDevice()` failing, or there is no sound, you will need to download the OpenAL 1.1 Windows Installer from [here](https://www.openal.org/downloads/) and run oalinst.exe.

### macOS
#### Get the data files for *The Elder Scrolls: Arena*
- [Download the Full Game](http://static.elderscrolls.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip) from the Bethesda website.
- Unzip `Arena106Setup.zip`
- `Arena106.exe` is a self-extracting RAR file. Use a tool such as [The Unarchiver](https://theunarchiver.com) to extract it into a folder of data files.

#### Install OpenTESArena
- Download the most recent build from the [releases](https://github.com/afritz1/OpenTESArena/releases) tab.
- Open the `.dmg` and copy OpenTESArena to the `Applications` folder or another location
- Right-click on the app and choose "Show Package Contents"
- Navigate to `Contents/Resources/data` and copy in the files for Arena that you extracted earlier
- Return to the `Applications` folder or wherever you have the app installed and open OpenTESArena. If you have Gatekeeper turned on (the default for macOS), you will need to do the following:
  - Right-click on the app and choose "Open"
  - In the warning that appears saying that it is from an unidentified developer, choose "Open"
  - The app will start. In the future, you can just double-click on the app without having to go through these steps.

### Linux (Ubuntu 16.04)
Substitute `<version>` with the current version number (`0.#.0`), and `<arch>` with the desired architecture (`32` or `64`).
```bash
sudo apt-get install wget unzip rar
wget https://cdnstatic.bethsoft.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip
wget https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-<version>/opentesarena-<version>-Linux<arch>.tar.gz
tar xvzf opentesarena-<version>-Linux<arch>.tar.gz
cd opentesarena-<version>/data
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

The easiest way is to download one of the eawpats packages ([zip](https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-0.1.0/eawpats.zip), [tar.gz](https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-0.1.0/eawpats.tar.gz)) and place the extracted eawpats folder into your `data` folder.

If you would like to use a different sound patches library (like OPL3), simply edit `MidiConfig` in the options file to point to another MIDI `.cfg` file.

## Building from source

### Project dependencies
- [CMake](https://cmake.org/download/)
- [OpenAL Soft 1.18.2](http://kcat.strangesoft.net/openal.html#download)
- [SDL 2.0.4](https://www.libsdl.org/download-2.0.php)
- [WildMIDI 0.4.3](https://github.com/Mindwerks/wildmidi/releases) (optional; required for music)

### Building the executable
- Create a `build` folder in the project's top-level directory.
- Use CMake to generate your project files in `build`. In a Unix terminal, the command might look like:
```bash
cd build
cmake ..
make -j4
```
- Other user-specific parameters may be necessary for CMake depending on your IDE.

### Running the executable
- Verify that the `data` and `options` folders are in the same folder as the executable.
- Make sure that `MidiConfig` and `ArenaPath` in the options file point to valid locations on your computer (i.e., `data/eawpats/timidity.cfg` and `data/ARENA` respectively).

If you struggle, here are some more detailed guides:
- [Building with Visual Studio (Windows)](docs/setup_windows.md)  
- [Building with MSYS2 (Windows)](docs/setup_windows_msys2.md)

If there is a bug or technical problem in the program, check out the issues tab!

## Resources

The [Unofficial Elder Scrolls Pages](http://en.uesp.net/wiki/Arena:Arena) are a great resource for finding information all about Arena. There are [various tools](http://en.uesp.net/wiki/Arena:Files#Misc_Utilities) available such as WinArena and BSATool for browsing Arena's content, and there is a very detailed manual as well, so you'll probably want to take a look at a copy from [here](http://en.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities). I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a humorous overview of the game's history and gameplay. 

YouTube channel: https://www.youtube.com/channel/UCJpmkgtHRIxR7aOpi909GKw
