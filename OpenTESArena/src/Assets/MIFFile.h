#ifndef MIF_FILE_H
#define MIF_FILE_H

#include <cstdint>
#include <string>
#include <vector>

// A MIF file contains map information. It defines the dimensions of a particular area 
// and which voxels have which IDs, as well as some other data. It is normally paired with 
// an INF file that tells which textures to use, among other things.

// It seems to be composed of a map header and an array of levels.

class MIFFile
{
private:
	struct Level
	{
		uint16_t size; // Size of the compressed level in bytes.
		std::string name, info; // Name of level, and associated INF filename.
		uint8_t numf; // Unknown, possibly an ID for something. Usually 9, 7, or 6.

		// Various data, not always present. "flor" and "map1" are probably always present.
		std::vector<uint8_t> targ, trig, lock, flor, map1, map2, loot;
	};

	int width, depth;
public:
	MIFFile(const std::string &filename);
	~MIFFile();

	// Gets the dimensions of the map. They are constant for all levels in a map.
	int getWidth() const;
	int getDepth() const;

	// Eventually...
	// - 2D voxel data per floor (flor, map1, map2), per level.
	// - Name and INF of a level.
	// - ID of a level (numf?)
	// - Target(?), Triggers(?), Lock (locked doors?), Loot (loot piles? Quest items?).
};

#endif
