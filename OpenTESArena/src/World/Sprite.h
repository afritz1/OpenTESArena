#ifndef SPRITE_H
#define SPRITE_H

#include <vector>

#include "../Math/Float3.h"
#include "../Math/Int3.h"

// This class is a bit experimental. It would be redundant for a non-player entity to
// have a sprite as a member. 

// What about "SpriteData"? It would be a class with dimension adjustments (for cacti, 
// etc.) and texture IDs. Maybe that would be a static class instead.

class Sprite
{
private:
	Float3d point, direction; // Point is at the base of the sprite.
	double width, height;
	// Texture ID...?
public:
	Sprite(const Float3d &point, const Float3d &direction, double width, double height);
	~Sprite();
};

#endif
