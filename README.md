# OpenTESArena
#### An open-source project for "The Elder Scrolls I: Arena"

## Current status

July 16th, 2016:

The game world is currently a barebones test city with some textured blocks, stone arches, and day/night cycles. No jumping, collision, or sprites yet (they're only in the test project). A few of the menus work, including most of character creation.

Here are some hotkeys in the game world:
- Tab - character sheet and inventory
- M - world map (click on provinces for province maps)
- L - logbook
- N - automap

OpenAL Soft and WildMIDI have been implemented, and MIDI music is now supported again. The user must provide their own patches, though (see instructions).

I'm currently trying to catch back up to the state of the world shown in these [preview images](https://github.com/afritz1/OpenTESArena/tree/master/samples) since the project is now aiming to use original data exclusively instead of tool-extracted images from applications like WinArena. The renderer is in development and is not fully implemented here. The test project that the images are from was also using some naive code for rendering sprites, so I am in the process of redesigning it for use here.

## Instructions

- Install the Arena data from the Bethesda [website](http://www.elderscrolls.com/arena/) ("Download the Full Game" link) in a folder somewhere.
- Get a sound patches library like [eawpats](https://slackbuilds.org/repository/13.37/audio/eawpats/) for the MIDI configuration (the .tar.gz under "Source Downloads". For decompression on Windows, 7-zip is an option).
- Point your compiler to the developer libraries and build the executable.
- Put the `data` and `options` folders from [here](https://www.dropbox.com/s/xc8llh52eahaofs/OpenTESArena_data.zip?dl=0) (updated July 16th) in the executable directory.
- Make sure `options.txt` points to a soundfont file (i.e., data/eawpats/timidity.cfg, see below) and Arena path (i.e., data/ARENA).

If using Visual Studio, edit the project's include and library directories to fit your computer. I haven't gotten CMake to work well enough with Visual Studio yet.

Copy the `timidity.cfg` file into the `eawpats` folder from `eawpats/winconfig` for Windows or `eawpats/linuxconfig` for Linux. You might need to comment out the lines with `dir` for it to use the folders in `eawpats`.

The components static library (i.e., `components.lib`) is built using a separate project with the source files in the `components` folder. Link it to the main project once created.

## Developer Libraries

- ~~[FluidSynth](https://sourceforge.net/projects/fluidsynth/files/) (Windows pre-compiled version 1.1.3 [here](http://slade.mancubus.net/index.php?page=wiki&wikipage=Windows-Compilation))~~
- [OpenAL Soft 1.17.2](http://kcat.strangesoft.net/openal.html#download)
- OpenCL 1.2 ([AMD](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/), [Nvidia](https://developer.nvidia.com/opencl)) - use cl2.hpp header from OpenCL 2.0
- [SDL2 2.0.4](https://www.libsdl.org/download-2.0.php)
- [SDL2_image 2.0.1](https://www.libsdl.org/projects/SDL_image/) (possibly phasing this out? It's only for PNGs)
- [WildMIDI 0.4](https://github.com/Mindwerks/wildmidi/releases)

## Introduction

This is my first big project! I'm already learning a ton.

This project aims to be an open-source reimplementation of the original "The Elder Scrolls I: Arena" game by Bethesda Softworks. It is being written in C++11 and uses SDL2 for cross-platform video, OpenAL Soft and WildMIDI for sound, and OpenCL for 3D rendering. There is currently support for Windows and Linux.

The concept began after I saw the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). It really started out more as an experiment than a remake, but now the project is steadily inching closer to something akin to the original.

It's named OpenTESArena so there's less confusion with the Quake III-based [OpenArena](https://github.com/OpenArena).

## Scope

This project is early in development.

Baseline features to do:
- Load original assets
- Character creation
- Game world interface and buttons (some hotkeys already work)
- Graphics engine using OpenCL
- Free-look camera
- Music and sound using OpenAL Soft
- Random test cities and dungeons
- Sprites
- Collision detection
- Weapon attacks (hold right mouse button to swing)

Later features:
- Automap
- Character faces (currently just DOSBox captures)
- Class traits (spell point multiplier, knight auto-repairing, etc.)
- Click to move and turn
- Enemies
- Inventory
- Weapon and armor overlays
- Levels and experience
- Options menu (just `options.txt` for now)

## Outside Scope (until later)

Some of these features are being left until later either because the original data is not accessible, they're simply too complex to mess with at this stage, or they're new ideas to consider.

Original features:
- City and dungeon data
- Loading/saving
- Quests
- Spells
- Wandering people
- Wilderness

New features:
- Custom class creation
- Followers
- Left click to attack
- New Imperial race
- New journal tabs
- New stores

## Graphics

The 3D graphics are being done with a ray tracer written in OpenCL. Test versions show it functioning decently on high-end graphics cards, but there are still many optimizations that can be done. As it is fairly experimental, I can't guarantee that the renderer will run well either on low-end computers or in CPU mode. I suggest using a modern GPU.

I puzzled for a long time about whether to use a ray tracer or OpenGL, or even just a software renderer, and I finally decided that this would be a good place to show that real time ray tracing can be done in some games today. Arena looked like a good game to experiment with graphics-wise due to its low geometry count.

I'm also considering replacing all of the geometry with rectangles instead of triangles. This would theoretically speed up rendering by 2x, but it would make integrating any kind of traditional rasterization (like OpenGL) more involved if I ever considered adding that option in the future.

## Resources

All of the music and sound files, as well as the vast majority of wall and sprite textures, are available to look at thanks to exporter programs like WinArena and [other utilities](http://www.uesp.net/wiki/Arena:Files#Misc_Utilities). However, this project will still use the original files that came with the floppy disk version of Arena.

Here is the [Unofficial Elder Scrolls website](http://www.uesp.net/wiki/Arena:Arena) for information regarding the original game. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a (humorous) overview of the game's history and gameplay. The Arena manual PDF is available [here](http://www.uesp.net/wiki/Arena:Files#Official_Patches_and_Utilities) as well.
