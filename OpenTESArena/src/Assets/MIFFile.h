#ifndef MIF_FILE_H
#define MIF_FILE_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "ArenaTypes.h"
#include "../Math/Vector2.h"
#include "../World/VoxelUtils.h"

// A MIF file contains map information. It defines the dimensions of a particular area 
// and which voxels have which IDs, as well as some other data. It is normally paired with 
// an INF file that tells which textures to use, among other things.

// It is composed of a map header and an array of levels.

class MIFFile
{
public:
	struct Level
	{
		std::string name, info; // Name of level and associated INF filename.
		int numf; // Number of floor textures.

		// Various data, not always present. FLOR and MAP1 are probably always present.
		// - @todo: maybe store MAP2 data with each voxel's extended height?
		std::vector<uint16_t> flor, map1, map2;
		std::vector<uint8_t> flat, inns, loot, stor;
		std::vector<ArenaTypes::MIFTarget> targ;
		std::vector<ArenaTypes::MIFLock> lock;
		std::vector<ArenaTypes::MIFTrigger> trig;

		Level();

		// Primary method for decoding .MIF level tag data. This method calls all the lower-
		// level loading methods for each tag as needed. The return value is the offset from 
		// the current LEVL tag to where the next LEVL tag would be.
		int load(const uint8_t *levelStart);

		// Gets the height of the level in voxels. This value depends on extended blocks
		// in the MAP2 data, otherwise it drops back to a default value.
		int getHeight() const;

		// Loading methods for each .MIF level tag (FLOR, MAP1, etc.), called by Level::load(). 
		// The return value is the offset from the current tag to where the next tag would be.
		static int loadFLAT(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadFLOR(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadINFO(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadINNS(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadLOCK(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadLOOT(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadMAP1(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadMAP2(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadNAME(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadNUMF(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadSTOR(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadTARG(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadTRIG(MIFFile::Level &level, const uint8_t *tagStart);
	};
private:
	WEInt width;
	SNInt depth;
	int startingLevelIndex;
	std::array<Double2, 4> startPoints; // Entrance locations for the level (not always full).
	std::vector<MIFFile::Level> levels;
	std::string name;
	// Should a vector of levels be exposed, or does the caller want a nicer format?
	// VoxelGrid? Array of VoxelData?
public:
	bool init(const char *filename);

	// Gets the dimensions of the map. Width and depth are constant for all levels in a map,
	// and the height depends on MAP2 data in each level (if any -- default otherwise).
	WEInt getWidth() const;
	int getHeight(int levelIndex) const;
	SNInt getDepth() const;

	// Gets the starting level when the player enters the area.
	int getStartingLevelIndex() const;

	// Gets the name of the .MIF file.
	const std::string &getName() const;

	// Starting points for the player. The .MIF values require a division by 128 in order
	// to become "voxel units" (including the decimal value).
	const std::array<Double2, 4> &getStartPoints() const;

	// -- temp -- Get the levels associated with the .MIF file (I think we want the data 
	// to be in a nicer format before handing it over to the rest of the program).
	const std::vector<MIFFile::Level> &getLevels() const;
};

#endif
