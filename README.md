# OpenTESArena
#### An open-source project for "The Elder Scrolls I: Arena"

## Current status

June 11th, 2016:

Builds on Windows 7 with Visual Studio and Linux with GCC 4.8.4.

The basis for OpenAL Soft has been implemented, and MIDI music will be reimplemented soon.

The images [here](https://github.com/afritz1/OpenTESArena/tree/master/samples) are a preview of the graphics engine using some random cities from a test project for demonstration purposes. It is in development and is not currently implemented here.

The game is not yet in a playable state. A couple of the menus work, including most of character creation, though it's only a rough draft. The game world uses an infinite space placeholder while the graphics engine is in development. Pressing Tab in the game world opens the current character sheet. Several other placeholders exist for now as well.

Loading the original files from the Arena floppy disk version is now in the project scope, instead of using extracted and converted files for testing purposes.

## Instructions

The current subset of data files to use are from screen grabs in DOSBox and from extraction utilities, both for testing purposes. This project will eventually read the original Arena data files instead, found on the official Bethesda [website](http://www.elderscrolls.com/arena/) using the "Download the Full Game" link.

- Build the executable on your machine
- Get the small subset of currently used data files from [here](https://www.dropbox.com/s/xc8llh52eahaofs/OpenTESArena_data.zip?dl=0).
- Put the data files in the executable directory.

## Introduction

This is my first big project! I'm already learning a ton.

This project aims to be an open-source reimplementation of the original "The Elder Scrolls I: Arena" game by Bethesda Softworks. It is being written in C++14 and uses SDL2, OpenAL Soft, and OpenCL. It really started out more as an experiment than a remake, but it is steadily inching closer to something akin to the original now.

The concept began after I saw the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). This project is being developed on Windows 7 and Visual Studio 2015 using the VS2013 compiler. Linux is now supported as well.

## Developer Libraries

- [FluidSynth](https://sourceforge.net/projects/fluidsynth/files/) (Windows pre-compiled version 1.1.3 [here](http://slade.mancubus.net/index.php?page=wiki&wikipage=Windows-Compilation))
- [OpenAL Soft 1.17.2](http://kcat.strangesoft.net/openal.html#download)
- OpenCL 1.2 ([AMD](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/), [Nvidia](https://developer.nvidia.com/opencl)) - use cl2.hpp header from OpenCL 2.0
- [SDL2 2.0.4](https://www.libsdl.org/download-2.0.php)
- [SDL2_image 2.0.1](https://www.libsdl.org/projects/SDL_image/)
- [WildMIDI 0.3.9](https://github.com/Mindwerks/wildmidi/releases) (Windows also needs MSVCR80.dll)

## Scope

This project is early in development.

Baseline features to do:
- Character creation
- Graphics engine using OpenCL
- Free-look camera
- Music and sound using OpenAL Soft
- Random test cities and dungeons
- Sprites
- Collision detection
- Weapon attacks (hold right button to swing)

Later features:
- Class traits (spell point multiplier, knight auto-repairing, etc.)
- Click to move and turn
- Game world interface and buttons
- Enemies
- Inventory
- Levels and experience
- Loading/saving
- Map
- Options menu
- Spells
- Wandering people

## Outside Scope (until later)

Some of these features are being left until later either because the original data is not accessible, they're simply too complex to mess with at this stage, or they're new ideas to consider.

Original features:
- Original map data
- Quests
- Wilderness

New features:
- Custom class creation
- Followers
- Left click to attack
- New Imperial race
- New journal tabs
- New stores

## Graphics

The 3D graphics will be done with a GPU ray tracer written in OpenCL, and its code will be added here once I finish other components first. Test versions show it functioning decently on high-end graphics cards, but there are still many optimizations that can be done. 

I puzzled for a long time about whether to use a ray tracer or OpenGL, or even just a software renderer, and I finally decided that this would be a good place to show that real time ray tracing can be done in some games today. Arena looked like a good game to experiment with graphics-wise due to its low geometry count.

## Resources

All of the music and sound files, as well as the vast majority of wall and sprite textures, are available to look at thanks to exporter programs like WinArena and [other utilities](http://www.uesp.net/wiki/Arena:Files#Misc_Utilities). However, this project will still use the original files that came with the floppy disk version of Arena.

Some original files like the character backgrounds are still stored in an unknown format, but they can be obtained simply by screen capturing in DOSBox. If those original files could actually be decoded some day, that would be nice. Check out the image decompression algorithms in WinArena and Daggerfall as a place to start.

The format for the original map data for cities, dungeons, and wilderness is not currently known. This puts a considerable restriction on restoring the game world with its intended content. Main quest dungeons are not randomly-generated, but wilderness dungeons are (to a point).

## Other Resources

Here is the [Unofficial Elder Scrolls website](http://www.uesp.net/wiki/Arena:Arena) for information regarding the original game. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a (humorous) overview of the game's history and gameplay. The Arena manual PDF is available [here](http://www.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities) as well.
