# OpenTESArena

This project aims to be an open-source remake of the original "The Elder Scrolls I: Arena" game by Bethesda Softworks. It currently uses the MIT license (which may change) and is written in C++14/SDL2. This project is early in development, and there is currently only one programmer (myself) working on it.

Plans were built up around the second half of 2015 and just recently has this project started progressing through the early stages. The concept began after seeing the success of other open-source projects like [OpenXcom](http://openxcom.org/) and [OpenMW](http://openmw.org/en/). It is currently being developed on Windows + Visual Studio 2013, and thanks to the cross-platform nature of SDL and OpenCL, porting to Linux and other operating systems should be relatively easy. A mobile version is not planned at this point, and the graphics engine is one reason why.

## Scope

This game is planned to have several of the features present in the original while avoiding bugs at the same time. New features may be added and old ones left out wherever they make reasonable sense.

Some baseline features include:
- 3D engine powered by ray tracing via OpenCL
- 3D sound with FMOD Ex
- Random voxel cities and dungeons
- Collision detection
- City folk with simple conversations
- RPG elements (classes, attributes, inventory, spell book, levels, experience)
- Weapon swinging
- Cutscenes

Some more involved features include:
- Character/creature AI, path finding, spawn rates
- Spell effects and spell making
- Quests
- Loading and saving
- Crime, guard behavior
- Map
- Wilderness (no one knows if Arena's world really is infinite. Daggerfall's world is just really big)
- Dungeon memory (Arena resets a dungeon, even main quest dungeons, when the player leaves)

Some new features to possibly consider:
- Imperial race, with a new character background and portraits (the Breton bodies can be reused)
- Custom class creation using a simplified model of the Daggerfall class creator
- Jail time like Daggerfall, instead of guards always killing on sight
- Companions/followers (found in places like taverns and temples)
- Journal tabs (current/completed quests, player statistics, etc.)
- Mod support for new kinds of stores, class rule sets, etc.
- Saved game information like timestamps, character name, level, etc.
- Unlimited save game slots
- Options menu for graphics, audio, key bindings, etc.

Some old features to ignore or replace:
- The game world interface (replaced with a more Daggerfall/Morrowind style with just stat bars and compass. The buttons would be accessible by hotkeys)
- Clicking on-screen to move and turn (replaced with WASD controls and mouse-look)
- Dragging mouse around to attack (replaced with just clicking to attack. The attack direction might be determined by mouse or character movement)
- Creatures spawning in ridiculous places (stone golems in an old rickety house??)
- Inventory item limit (only restricted by weight then)

The 3D graphics are done with a GPU ray tracer written in OpenCL, and its code will be added here once I finish other components first. It functions decently and runs smoothly on high-end computers, but there are still many optimizations that can be done. I puzzled for a long time about whether to use a ray tracer or OpenGL, or even just a software renderer, and I finally decided that this would be a good place to show that real time ray tracing can be done in some games today. A number of sample images showcasing the current state of the ray tracer are available in the "samples" folder.

The spell casting system will likely need some redesigning to balance it out with the non-magic classes, since spell casters can have a game-breaking advantage. Maybe all classes would have spell points? But then who gets to wear plate? Do spells have an individual cooldown time, a global cooldown time, or can everyone cast spells like a machine gun? I think the original system was designed to be played with a party of adventurers like with D&D, not for solo characters. More discussion should happen on this topic first.

## Data Files

The original art and music may be included with project releases since the game was made freely available by Bethesda upon its 10th anniversary in March 2004. I don't anticipate any licensing issues because the game is free and this project is non-profit. The content files were extracted from the game data and eventually converted into common formats like MP3, PNG, and WAV thanks to [various Arena utilities](http://www.uesp.net/wiki/Arena:Files#Misc_Utilities) and other tools like GIMP and Audacity. The data will be posted on here or a hosted website (Dropbox?) at some point.

The MIDI music will be the standard music released with the project because it's only a couple of kilobytes, but other formats could be used instead (like Ogg), and an option for the music format would eventually be in the options menu.

Some original files like the character backgrounds are still stored in an unknown format, but they have been obtained simply by screen capturing in DOSBox. If those original files could be decoded some day, that would be nice.

The original map data for Arena cities, dungeons, etc. are not currently in an accessible format, so their layout in-game will be limited to one-time random generation for now (which may or may not have been a desired trait in the first place). Main quest relevant locations, like Fang Lair, should be recreated by hand if the original map data can't be salvaged, since they are not randomized. Maybe there could be a map editor for creating main quest dungeons and city/wilderness chunks, but that's asking a bit much at this point.

## Resources

Here is the [Unofficial Elder Scrolls website](http://www.uesp.net/wiki/Arena:Arena) for information regarding the original game. I also recommend the [Lazy Game Review](https://www.youtube.com/watch?v=5MW5SxKMrtE) on YouTube for a (humorous) overview of the game's history and gameplay.
