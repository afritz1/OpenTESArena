#ifndef WORLD_TYPE_H
#define WORLD_TYPE_H

// A unique identifier for each kind of place the player is in. These values assist the 
// program with level data -- i.e., generating a ceiling depending on whether a place is 
// indoors or outdoors, etc.. This enum might be reworked/removed at some point.
enum class WorldType
{
	City,
	Interior,
	Wilderness
};

#endif
