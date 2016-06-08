# OpenTESArena
#### An open-source project for "The Elder Scrolls I: Arena"

### Current status

June 8th, 2016:

Builds on Windows 7 with Visual Studio and Linux Mint with GCC 4.8.4. I'll likely start using CMake sometime soon.

The images [here](https://github.com/afritz1/OpenTESArena/tree/master/samples) are a preview of the graphics engine using some random cities from a test project for demonstration purposes. It is in development and is not currently implemented here.

Rough draft skeletons of the intro cinematic, main menu, and character creation are in. The player can choose their gender, class, name, race, and character portrait. Class questions and attribute selection are not implemented yet.

The game world is a placeholder infinite space while the graphics engine is in development. No gameplay or scene data is available yet. The UI stat bars (health, stamina, etc.) are also placeholders. The pause menu also uses a placeholder.

Options for resolution, field of view, look sensitivity, and sound are loaded from a text file on startup. The options menu in-game is currently not implemented.

## Instructions

- Build the executable on your machine
- Get the small subset of currently used data files from [here](https://www.dropbox.com/s/xc8llh52eahaofs/OpenTESArena_data.zip?dl=0).
- More data files will be added as program functionality grows.

## Introduction

This is my first big project! I'm already learning a ton.

This project aims to be an open-source reimplementation of the original "The Elder Scrolls I: Arena" game by Bethesda Softworks. It is being written in C++14 using the SDL2, FMOD Ex, and OpenCL 2.0 libraries, and currently uses the MIT license. It really started out more as an experiment than a remake, but it is steadily inching closer to something akin to the original.

The concept began after I saw the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). This project is being developed on Windows 7 and Visual Studio 2015 using the VS2013 compiler, and thanks to the cross-platform nature of SDL2 and OpenCL, support for Linux is already in sight.

## Developer Libraries

- [FMOD Ex](http://www.fmod.org/download-previous-products/#FMODExAPIDownloads) (switching to OpenAL-Soft?)
- OpenCL 2.0 ([AMD](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/), [Nvidia](https://developer.nvidia.com/opencl)) - use "cl2.hpp" instead of "cl.hpp"
- [SDL2 2.0.4](https://www.libsdl.org/download-2.0.php)
- [SDL2_image 2.0.1](https://www.libsdl.org/projects/SDL_image/)

## Scope

This project is early in development.

Baseline features:
- Character creation
- Graphics engine using OpenCL
- Free-look camera
- Music and 3D sound with FMOD Ex (or OpenAL-Soft?)
- Random test cities and dungeons
- Sprites
- Collision detection
- Weapon attacks (hold right button to swing)

Later features:
- Class traits (spell point multiplier, knight auto-repairing, etc.)
- Click to move and turn
- Game world interface and buttons
- Enemies
- Inventory (no item limit!)
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

The 3D graphics will be done with a GPU ray tracer written in OpenCL, and its code will be added here once I finish other components first. Test versions of it show it functioning decently on high-end graphics cards, but there are still many optimizations that can be done. I puzzled for a long time about whether to use a ray tracer or OpenGL, or even just a software renderer, and I finally decided that this would be a good place to show that real time ray tracing can be done in some games today. Arena looked like a good game to experiment with graphics-wise due to its low geometry count.

There are some neat effects like ambient occlusion and adaptive anti-aliasing I experimented with that I would like to implement, but they're still pretty naive at this point and will probably only be usable on fast graphics cards.

## Resources

All of the music and sound files, as well as the vast majority of wall and sprite textures, are available to use thanks to exporter programs like WinArena, [other utilities](http://www.uesp.net/wiki/Arena:Files#Misc_Utilities), and tools like GIMP and Audacity. The textures are all being converted to PNG for this project since it's a well-known lossless format and some game world textures (like ghosts) may eventually have transparency.

The original art and music should be able to be included with project releases since the game was made freely available by Bethesda upon its 10th anniversary in March 2004. I don't anticipate any licensing issues (or cease and desist letters) because the game is free and this project is non-profit.

The MIDI music will be the standard music released with the project because it's only a couple of kilobytes, but other formats should be usable at some point like Ogg.

Some original files like the character backgrounds are still stored in an unknown format, but they have been obtained simply by screen capturing in DOSBox. If those original files could actually be decoded some day, that would be nice. Check out the image decompression algorithms in WinArena and for Daggerfall as a place to start.

The original map data for cities, dungeons, and wilderness is not currently available. This puts a considerable restriction on having all places use their intended layout and creatures without placing them by hand (ugh). Main quest dungeons are not randomly-generated

## Other Resources

Here is the [Unofficial Elder Scrolls website](http://www.uesp.net/wiki/Arena:Arena) for information regarding the original game. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a (humorous) overview of the game's history and gameplay. The Arena manual PDF is available [here](http://www.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities) as well.
