# OpenTESArena

This open-source project aims to be a modern engine re-implementation for "The Elder Scrolls: Arena" by Bethesda Softworks. It is written in C++11 and uses SDL2 for cross-platform video, WildMIDI for music, and OpenAL Soft for sound and mixing. There is currently support for Windows and Linux.

- Version: 0.4.0
- License: MIT
- IRC: #opentesarena on [webchat.freenode.net](https://webchat.freenode.net/)

## Current status [![Build Status](https://travis-ci.org/afritz1/OpenTESArena.svg?branch=master)](https://travis-ci.org/afritz1/OpenTESArena)

This project is early in development.

No jumping or collision yet. A few of the menus work, including some of character creation, and some of the game interface icons have basic behavior, too. For example, left clicking the map icon goes to the automap, and right clicking it goes to the world map.

Here are some keys in the game world:
- WASD - move and turn. Hold LCtrl to strafe with A and D.
- Esc - pause menu
- Tab - character sheet
- L - logbook
- M - world map (click on provinces for province maps)
- N - automap
- F4 - toggle debug text

<br/>

![Preview](Preview.PNG)
<br/>

## Project Details

The concept began after I saw the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). It really started out more as an experiment than a remake (and it still is quite an experiment), but now the project is steadily inching closer to something akin to the original.

I've been experimenting with various methods of rendering looking for the right one. Versions 0.2.0 and before use 3D ray casting with OpenCL on the GPU, and it allows for some really nice-looking dynamic shadows. I'm trying out a software ray casting method instead to see if it would be a better fit. Of course there's always OpenGL, but this project (and Arena's low geometry count) affords me the opportunity to experiment. I want to see how the software renderer turns out before I think about reintroducing a graphics card API.

Note that there are two versions of Arena: the floppy disk version and the CD version. Bethesda released the floppy disk version  [here](http://www.elderscrolls.com/arena/) for free, and this project is being designed for use with that. The user must still acquire their own copy of Arena, though.

Check out [CONTRIBUTING.md](CONTRIBUTING.md) for more details on how to assist with development.

## Installation

The engine uses `Soundfont` and `ArenaPath` in `options/options.txt` to find where the MIDI config and game files are.

#### Windows
- Get the most recent build from the [releases](https://github.com/afritz1/OpenTESArena/releases) tab.
- [Download the Full Game](http://static.elderscrolls.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip) from the Bethesda website.
- Extract Arena106Setup.zip and run Arena106.exe.
- Pick a destination folder anywhere and install.
- Point `ArenaPath` in `options/options.txt` to the `ARENA` folder.
- If you receive an error about "alcOpenDevice", you will need to download and run the OpenAL 1.1 Windows Installer from [here](https://www.openal.org/downloads/).

#### Linux (Ubuntu 16.04)
Substitute `<version>` with the current version number (i.e., `0.4.0`), and `<arch>` with the desired architecture (`32` or `64`).
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

#### Obtaining a MIDI sound patches library (for music):
- The easiest way is to download one of the eawpats packages ([zip](https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-0.1.0/eawpats.zip), [tar.gz](https://github.com/afritz1/OpenTESArena/releases/download/opentesarena-0.1.0/eawpats.tar.gz)) and place the extracted eawpats folder into your `data` folder.
- If you would like to use a different sound patches library, simply edit `Soundfont` in `options/options.txt` to point to another existing MIDI `.cfg` file.

## Building from source

#### Project dependencies:
- [CMake](https://cmake.org/download/)
- [OpenAL Soft 1.17.2](http://kcat.strangesoft.net/openal.html#download)
- [SDL 2.0.4](https://www.libsdl.org/download-2.0.php)
- [WildMIDI 0.4.0](https://github.com/Mindwerks/wildmidi/releases)

#### Building the executable:
- Create a `build` folder in the top-level directory.
- Use CMake to generate your project files in `build`, then compile the executable.

#### Running the executable:
- Verify that the `data` and `options` folders are in the same folder as the executable, and that `Soundfont` and `ArenaPath` in `options/options.txt` point to valid locations on your computer (i.e., `data/eawpats/timidity.cfg` and `data/ARENA` respectively).

If there is a bug or technical problem in the program, check out the issues tab!

## Resources

The [Unofficial Elder Scrolls Pages](http://en.uesp.net/wiki/Arena:Arena) are a great resource for finding information all about Arena. There are [various tools](http://en.uesp.net/wiki/Arena:Files#Misc_Utilities) available such as WinArena for browsing Arena's content, and there is a very detailed manual as well, so you'll probably want to take a look at a copy from [here](http://en.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities). I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a humorous overview of the game's history and gameplay. Here's the YouTube [channel](https://www.youtube.com/channel/UCJpmkgtHRIxR7aOpi909GKw) where I post project updates.
