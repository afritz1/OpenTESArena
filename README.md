# OpenTESArena

This open-source project aims to be a modern engine reimplementation for "The Elder Scrolls: Arena" by Bethesda Softworks. It is written in C++11 and uses SDL2 for cross-platform video, OpenAL Soft and WildMIDI for sound, and OpenCL for 3D rendering. There is currently support for Windows and Linux.

- Version: 0.1.0
- License: MIT
- IRC: #opentesarena on irc.freenode.net

## Current status [![Build Status](https://travis-ci.org/afritz1/OpenTESArena.svg?branch=master)](https://travis-ci.org/afritz1/OpenTESArena)

This project is early in development.

The game world is currently a barebones test city with some buildings, sprites, and a day/night cycle. No jumping or collision yet. A few of the menus work, including some of character creation, and some of the game interface icons have basic behavior, too. For example, left clicking the map icon goes to the automap, and right clicking it goes to the world map.

Here are some hotkeys in the game world:
- Esc - pause menu
- Tab - character sheet and inventory
- M - world map (click on provinces for province maps)
- L - logbook
- N - automap

<br/>
![Preview](Preview.PNG)
<br/>

## Project Details

The concept began after I saw the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). It really started out more as an experiment than a remake (and it still is quite an experiment), but now the project is steadily inching closer to something akin to the original.

The graphics are being rendered with a ray tracer I'm writing in OpenCL. I puzzled for a long time about what 3D rendering method to use, and I decided that this would be a good place to demonstrate real time GPU ray tracing. Arena looked like a good game to experiment with graphics-wise due to its low geometry count.

Note that there are two versions of Arena: the floppy disk version and the CD version. Bethesda released the floppy disk version  [here](http://www.elderscrolls.com/arena/) for free, and this project is being designed for use with that. The user must still acquire their own copy of Arena, though.

This project shares a similar name with the Quake III-based [OpenArena](https://github.com/OpenArena), though the two projects are unrelated.

Check out the Projects tab to see what's currently on the to-do list. Open a pull request if you'd like to contribute.

## Installation

The most recent builds can be found in the [releases](https://github.com/afritz1/OpenTESArena/releases) tab. The engine uses `Soundfont` and `ArenaPath` in `options/options.txt` to find where the MIDI config and game files are.

#### Installing the Arena game data (Windows, Linux + WINE):
- [Download the Full Game](http://static.elderscrolls.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip) from the Bethesda website.
- Extract Arena106Setup.zip.
- Run Arena106.exe.
- Pick a destination folder anywhere.
- Install.
- Point `ArenaPath` in `options/options.txt` to the `ARENA` folder.

#### Obtaining a MIDI sound patches library:
- The easiest way is to download one of the eawpats packages from the [releases](https://github.com/afritz1/OpenTESArena/releases) tab and place the extracted eawpats folder into your `data` folder.
- If you would like to use a different sound patches library, simply edit `Soundfont` in `options/options.txt` to point to another existing MIDI `.cfg` file.

## Building from source

#### Project dependencies:
- [CMake](https://cmake.org/download/)
- [OpenAL Soft 1.17.2](http://kcat.strangesoft.net/openal.html#download)
- OpenCL 1.2 ([AMD](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/), [Nvidia](https://developer.nvidia.com/opencl)) - use [cl2.hpp](https://github.com/KhronosGroup/OpenCL-CLHPP/releases/download/v2.0.10/cl2.hpp) header from OpenCL 2.0.
- [SDL 2.0.4](https://www.libsdl.org/download-2.0.php)
- [WildMIDI 0.4.0](https://github.com/Mindwerks/wildmidi/releases)

#### Building the executable:
- Use CMake to generate your project files and then build the executable.

#### Running the executable:
- You will need to copy the `data` and `options` folders to the project directory to allow the executable to run within your IDE (this should be automated sometime).
- Verify that `Soundfont` and `ArenaPath` in `options/options.txt` point to valid locations on your computer (i.e., `data/eawpats/timidity.cfg` and `data/ARENA` respectively).

If there is a bug or technical problem in the program, check out the issues tab!

## Scope

Current priority:
- Lights (street lamps, etc.)
- Options menu

Next priority:
- Character creation questions
- Game interface button behavior
- Weapon attacks (hold RMB to swing)

Later:
- Adaptive super-sampling
- Automap
- Class rules and traits
- Clicking in game world
- Collision detection
- Doors and secret walls
- Enemies
- Inventory and containers
- Levels and experience
- Load artifact data
- Load class data (to replace classes.txt)
- Load locations data
- Load unpacked Arena executable data
- Modern (Daggerfall-like) interface option with free-look camera
- Random test cities and dungeons
- Redesigned water and lava (to replace vanilla screen animations)
- Reflections in windows and water

Outside scope (until much later):
- (new) Custom class creation
- (new) Followers
- (new) Imperial race
- (new) Journal tabs
- Loading/saving redesign
- Main and side quests
- Modding
- (new) New kinds of stores
- Original city/dungeon data
- Pickpocketing
- Scripting
- Services (bartender, priest, shopkeeper, wizard)
- Spells
- Sprite variation (clothes/hair/skin colors)
- (new) UI scale
- Vulkan (to replace OpenCL)
- Wandering people
- Wilderness (seed-based chunk generation)

## Resources

All of the music and sound files, as well as the vast majority of wall and sprite textures, are available to look at thanks to exporter programs like WinArena and [other utilities](http://www.uesp.net/wiki/Arena:Files#Misc_Utilities). However, this project will still use the original files that came with the floppy disk version of Arena.

Here is the [Unofficial Elder Scrolls website](http://www.uesp.net/wiki/Arena:Arena) for information regarding the original game. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a humorous overview of the game's history and gameplay. The Arena manual PDF is available [here](http://www.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities) as well.
