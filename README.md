# OpenTESArena
#### An open-source project for "The Elder Scrolls I: Arena"

## Current status

August 6th, 2016:

This project is early in development.

The game world is currently a barebones test city with some textured blocks, stone arches, and day/night cycles. No jumping, collision, or sprites yet (they're only in my test project). A few of the menus work, including most of character creation.

Here are some hotkeys in the game world:
- Tab - character sheet and inventory
- M - world map (click on provinces for province maps)
- L - logbook
- N - automap

I'm currently trying to catch back up to the state of the world shown in these [preview images](https://github.com/afritz1/OpenTESArena/tree/master/samples), since the project is now aiming to use original data exclusively instead of tool-extracted images from applications like WinArena. The graphics engine is in development and is only partially implemented here.

## Project Details

This open-source project aims to be a modern reimplementation of "The Elder Scrolls I: Arena" by Bethesda Softworks. It is being written in C++11 and uses SDL2 for cross-platform video, OpenAL Soft and WildMIDI for sound, and OpenCL for 3D rendering. There is currently support for Windows and Linux.

The concept began after I saw the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). It really started out more as an experiment than a remake, but now the project is steadily inching closer to something akin to the original.

It's named OpenTESArena so there's less confusion with the Quake III-based [OpenArena](https://github.com/OpenArena).

Open a pull request if you'd like to contribute.

## Instructions

Get the latest `data` and `options` folders [here](https://www.dropbox.com/s/xc8llh52eahaofs/OpenTESArena_data.zip?dl=0) (last updated August 6th).

#### Installing the Arena game data (Windows, Linux + WINE):
- [Download the Full Game](http://static.elderscrolls.com/elderscrolls.com/assets/files/tes/extras/Arena106Setup.zip) from the Bethesda website.
- Extract Arena106Setup.zip.
- Run Arena106.exe.
- Pick a destination folder anywhere.
- Install.

#### Obtaining a MIDI sound patches library:
- Suggested: [eawpats12_full.tar.gz](http://distfiles.gentoo.org/distfiles/eawpats12_full.tar.gz) (I use [7-Zip](http://www.7-zip.org/) for extracting it on Windows).
- Copy `timidity.cfg` from `eawpats\winconfig` on Windows or `eawpats\linuxconfig` on Linux up one folder into `eawpats`.
- The lines with `dir` in `timidity.cfg` may need to be ignored for the right directories to be used (for example, `dir c:\timidity` becomes `#dir c:\timidity` and `dir c:\eawpats` becomes `#dir c:\eawpats`).

#### Obtaining the developer libraries:
- [OpenAL Soft 1.17.2](http://kcat.strangesoft.net/openal.html#download)
- OpenCL 1.2 ([AMD](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/), [Nvidia](https://developer.nvidia.com/opencl)) - use cl2.hpp header from OpenCL 2.0
- [SDL2 2.0.4](https://www.libsdl.org/download-2.0.php)
- [SDL2_image 2.0.1](https://www.libsdl.org/projects/SDL_image/) (this dependency will be removed eventually)
- [WildMIDI 0.4.0](https://github.com/Mindwerks/wildmidi/releases)

#### Building the executable:
- Build the components static library (i.e., `components.lib`) separately using the files in the `components` folder.
- If necessary, edit the Visual Studio project's include and library directories to fit your computer.
- Link to the developer libraries and components library and build the executable.

#### Running the executable:
- Put the `data` and `options` folders, as well as any dependencies (SDL2.dll, wildmidi_dynamic.dll, etc.), in the executable directory.
- Verify that `Soundfont` and `DataPath` in `options\options.txt` point to valid locations on your computer (i.e., `data\eawpats\timidity.cfg` and `data\ARENA` respectively).

If there is a bug or technical problem in the program, check out the issues tab!

## Scope

Current priority:
- Load character faces (.CIF, to replace PNGs)
- Load font files (.DAT, to replace PNGs)
- Load frames from videos (.FLC/.CEL, to replace PNGs)
- Load original textures (.CFA, .CIF, .DFA, .IMG, etc.)
- Sprites

Next priority:
- Character creation questions
- Collision detection
- Game interface/buttons (some hotkeys already work)
- Options menu
- Weapon attacks (hold RMB to swing)

Later:
- Automap
- Class rules and traits
- Click to move and turn
- Enemies
- Inventory and containers
- Levels and experience
- Load artifact data (to replace artifacts.txt)
- Load class data (to replace classes.txt)
- Load locations data (to replace locations.txt)
- More sprite detail (various clothes/hair/skin colors)
- Random test cities and dungeons

Outside scope (until later):
- (new) Custom class creation
- (new) Followers
- (new) Imperial race
- (new) Journal tabs
- (new) Left click to attack
- Loading/saving
- Main and side quests
- Modding
- (new) New kinds of stores
- Original city/dungeon data
- Spells
- Wandering people
- Wilderness

## Graphics

The 3D graphics are being done with a ray tracer I am writing in OpenCL. Test versions show it functioning decently on high-end graphics cards, but there are still many optimizations that can be done. As it is fairly experimental, I can't guarantee that the graphics engine will run well either on low-end computers or in CPU mode unless the render resolution is low. I suggest using a modern GPU.

I puzzled for a long time about whether to use a ray tracer or OpenGL, or even just a software renderer, and I finally decided that this would be a good place to show that real time ray tracing can be done in some games today. Arena looked like a good game to experiment with graphics-wise due to its low geometry count.

## Resources

All of the music and sound files, as well as the vast majority of wall and sprite textures, are available to look at thanks to exporter programs like WinArena and [other utilities](http://www.uesp.net/wiki/Arena:Files#Misc_Utilities). However, this project will still use the original files that came with the floppy disk version of Arena.

Here is the [Unofficial Elder Scrolls website](http://www.uesp.net/wiki/Arena:Arena) for information regarding the original game. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a (humorous) overview of the game's history and gameplay. The Arena manual PDF is available [here](http://www.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities) as well.
