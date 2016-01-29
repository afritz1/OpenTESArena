# OpenTESArena

This project plans to be an open-source remake of the original "The Elder Scrolls I: Arena" game by Bethesda Softworks. It currently uses the MIT license and is planned to be written in C++14/SDL2. This project is very early in development, and there is currently only one programmer (myself) working on it.

Plans were built up around the second half of 2015 and just recently has this project started progressing through the early stages. The concept began after seeing the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). It is currently being developed on Windows, and thanks to the cross-platform nature of SDL and OpenCL, porting to Linux and other operating systems should be relatively easy. A mobile version is not planned at this point, and the graphics engine is one reason why.

## Scope

This game is planned to have several of the features (and none of the bugs) present in the original. New features may be added and old ones left out wherever they make reasonable sense. 

Some baseline features include:
- 3D engine powered by ray tracing via OpenCL
- 3D sound with FMOD Ex
- Random cities and dungeons
- Collision detection
- City folk with simple conversations
- RPG elements (classes, attributes, inventory, spell book, levels, experience)
- Weapon swinging
- Cutscenes

Some more involved features include:
- Creature AI, path finding, spawn rates
- Spell effects and spell making
- Quests, journal
- Crime, guard behavior
- Map
- Infinite wilderness (Arena's world is infinite, while Daggerfall's is just really big)
- Dungeon memory (the original Arena forgets any state in a dungeon, even main quest dungeons, when the player leaves)

Some new features to consider:
- Imperial race, with a new character background and portraits
- Custom class creation, in a style similar to the Daggerfall creator
- Jail time like Daggerfall, instead of guards always killing on sight
- Companions/followers, usually found in places like taverns or temples
- Mod support for adding new kinds of stores, class rule sets (i.e., armor restrictions), etc.
- Saved game information like timestamps, character name, level, etc.
- Unlimited save game slots
- An actual options menu for graphics, audio, key bindings, etc.

Some old features to ignore or replace:
- The game world interface (replaced with a more Daggerfall/Morrowind style with just stat bars and compass)
- Clicking on-screen to move and turn (replaced with WASD controls and mouse-look)
- Dragging mouse around to attack (replaced with just clicking to attack. The attack direction might be determined by mouse or character movement)
- Creatures spawning in ridiculous places (stone golems in an old rickety house??)
- Inventory item limit (only restricted by encumberance, then)

Currently the only (partially) implemented features are the random city generator and the graphics engine, and those were just done for demonstration purposes. The 3D graphics are done with a GPU ray tracer written with the OpenCL framework, and its code will be uploaded once I get everything at least moderately organized for Git. A number of sample images showcasing the current state of the ray tracer are available in the "samples" folder.

The original map data for Arena cities, dungeons, etc. are not currently in an accessible format, so their layout in-game will be limited to random generation for each new character (which may or may not have been a desired trait in the first place). Main quest relevant locations, like Fang Lair, should be recreated by hand if the original map data can't be salvaged, since they are not randomized.

## Data Files

The original art and music may be included with project releases since the game was made freely available by Bethesda upon its 10th anniversary in March 2004. Those content files were extracted from the game data and eventually converted into common formats like PNG, MIDI, and WAV thanks to the WinArena project, and others Arena tools. They will be posted on here or a hosted website at some point.

## Resources

Here is the [Unofficial Elder Scrolls website](http://www.uesp.net/wiki/Arena:Arena) for information regarding the original game. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a (humorous) overview of the game's history and gameplay.
