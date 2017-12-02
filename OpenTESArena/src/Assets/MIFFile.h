#ifndef MIF_FILE_H
#define MIF_FILE_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "../Math/Vector2.h"

// A MIF file contains map information. It defines the dimensions of a particular area 
// and which voxels have which IDs, as well as some other data. It is normally paired with 
// an INF file that tells which textures to use, among other things.

// It is composed of a map header and an array of levels.

class MIFFile
{
public:
	struct Level
	{
		struct Lock
		{
			int x, y, lockLevel;
		};

		struct Trigger
		{
			int x, y, textIndex, soundIndex;
		};

		std::string name, info; // Name of level and associated INF filename.
		int numf; // Number of floor textures.

		// Various data, not always present. FLOR and MAP1 are probably always present.
		// - To do: maybe store MAP2 data with each voxel's extended height?
		std::vector<uint8_t> flat, flor, inns, loot, map1, map2, targ;
		std::vector<MIFFile::Level::Lock> lock;
		std::vector<MIFFile::Level::Trigger> trig;

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
		static int loadTARG(MIFFile::Level &level, const uint8_t *tagStart);
		static int loadTRIG(MIFFile::Level &level, const uint8_t *tagStart);
	};
private:
	int width, depth;
	int startingLevelIndex;
	std::array<Double2, 4> startPoints; // Entrance locations for the level (not always full).
	std::vector<MIFFile::Level> levels;
	// Should a vector of levels be exposed, or does the caller want a nicer format?
	// VoxelGrid? Array of VoxelData?
public:
	MIFFile(const std::string &filename);
	~MIFFile();

	// Identifiers for various chasms in Arena's voxel data.
	static const uint8_t DRY_CHASM;
	static const uint8_t WET_CHASM;
	static const uint8_t LAVA_CHASM;

	// This value is used for transforming .MIF coordinates to voxel coordinates. For example, 
	// if the values in the .MIF files are "centimeters", then dividing by this value converts 
	// them to voxel coordinates (including decimal values; i.e., X=1.5 means the middle of the 
	// voxel at X coordinate 1).
	static const double ARENA_UNITS;

	// Generates the filename for a main quest .MIF file given the XY province coordinates 
	// and the province ID.
	static std::string mainQuestDungeonFilename(int dungeonX, int dungeonY, int provinceID);

	// Gets the dimensions of the map. Width and depth are constant for all levels in a map,
	// and the height depends on MAP2 data in each level (if any -- default otherwise).
	int getWidth() const;
	int getHeight(int levelIndex) const;
	int getDepth() const;

	// Gets the starting level when the player enters the area.
	int getStartingLevelIndex() const;

	// Starting points for the player. The .MIF values require a division by 128 in order
	// to become "voxel units" (including the decimal value).
	const std::array<Double2, 4> &getStartPoints() const;

	// -- temp -- Get the levels associated with the .MIF file (I think we want the data 
	// to be in a nicer format before handing it over to the rest of the program).
	const std::vector<MIFFile::Level> &getLevels() const;
};

#endif
