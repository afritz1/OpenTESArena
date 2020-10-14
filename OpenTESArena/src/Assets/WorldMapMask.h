#ifndef WORLD_MAP_MASK_H
#define WORLD_MAP_MASK_H

#include <cstdint>
#include <vector>

#include "../Math/Rect.h"

// Stores world map mask data from TAMRIEL.MNU. 9 out of 10 of the masks are for provinces, 
// while the last one is for the "Exit" button.

class WorldMapMask
{
private:
	std::vector<uint8_t> mask;
	Rect rect;
public:
	WorldMapMask(std::vector<uint8_t> &&mask, const Rect &rect);

	// This exists so the user of the class can store a std::array of them.
	WorldMapMask() = default;

	// Gets the adjusted width for some rectangle width. This is used in calculating
	// the bitmask byte count and the index in the bitmask.
	static int getAdjustedWidth(int width);

	const Rect &getRect() const;

	// Returns whether the pixel at the given XY coordinate is set. The X and Y values
	// should be absolute mouse coordinates in 320x200 space.
	bool get(int x, int y) const;
};

#endif
