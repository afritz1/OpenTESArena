#ifndef MIF_FILE_H
#define MIF_FILE_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "ArenaTypes.h"
#include "../Math/Vector2.h"
#include "../Voxels/VoxelUtils.h"

#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView2D.h"

// A .MIF file contains a map header and an array of levels. It defines the dimensions of
// a particular area and which voxels have which IDs, as well as some other data. It is normally
// paired with an .INF file that tells which textures to use, among other things.
class MIFLevel
{
private:
	std::string name, info; // Name of level and associated .INF filename.
	int numf; // Number of floor textures.

	// Various data, not always present. FLOR and MAP1 are probably always present.
	Buffer2D<ArenaTypes::VoxelID> flor, map1, map2;
	std::vector<uint8_t> flat, inns, loot, stor;
	std::vector<ArenaTypes::MIFTarget> targ;
	std::vector<ArenaTypes::MIFLock> lock;
	std::vector<ArenaTypes::MIFTrigger> trig;
public:
	MIFLevel();

	// Primary method for decoding .MIF level tag data. This method calls all the lower-
	// level loading methods for each tag as needed. The return value is the offset from 
	// the current LEVL tag to where the next LEVL tag would be.
	int load(const uint8_t *levelStart, WEInt levelWidth, SNInt levelDepth);

	// Loading methods for .MIF level tags that use level dimensions. The return value is
	// the offset from the current tag to where the next tag would be.
	static int loadFLOR(MIFLevel &level, const uint8_t *tagStart, WEInt levelWidth, SNInt levelDepth);
	static int loadMAP1(MIFLevel &level, const uint8_t *tagStart, WEInt levelWidth, SNInt levelDepth);
	static int loadMAP2(MIFLevel &level, const uint8_t *tagStart, WEInt levelWidth, SNInt levelDepth);

	// Loading methods for each .MIF level tag that don't need level dimensions.
	static int loadFLAT(MIFLevel &level, const uint8_t *tagStart);
	static int loadINFO(MIFLevel &level, const uint8_t *tagStart);
	static int loadINNS(MIFLevel &level, const uint8_t *tagStart);
	static int loadLOCK(MIFLevel &level, const uint8_t *tagStart);
	static int loadLOOT(MIFLevel &level, const uint8_t *tagStart);
	static int loadNAME(MIFLevel &level, const uint8_t *tagStart);
	static int loadNUMF(MIFLevel &level, const uint8_t *tagStart);
	static int loadSTOR(MIFLevel &level, const uint8_t *tagStart);
	static int loadTARG(MIFLevel &level, const uint8_t *tagStart);
	static int loadTRIG(MIFLevel &level, const uint8_t *tagStart);

	const std::string &getName() const;
	const std::string &getInfo() const;
	int getNumf() const;

	BufferView2D<const ArenaTypes::VoxelID> getFLOR() const;
	BufferView2D<const ArenaTypes::VoxelID> getMAP1() const;
	BufferView2D<const ArenaTypes::VoxelID> getMAP2() const;

	BufferView<const uint8_t> getFLAT() const;
	BufferView<const uint8_t> getINNS() const;
	BufferView<const uint8_t> getLOOT() const;
	BufferView<const uint8_t> getSTOR() const;

	BufferView<const ArenaTypes::MIFTarget> getTARG() const;
	BufferView<const ArenaTypes::MIFLock> getLOCK() const;
	BufferView<const ArenaTypes::MIFTrigger> getTRIG() const;
};

class MIFFile
{
private:
	std::string filename;
	WEInt width;
	SNInt depth;
	int startingLevelIndex;
	std::array<OriginalInt2, 4> startPoints; // Entrance locations for the level (not always full).
	std::vector<MIFLevel> levels;
public:
	bool init(const char *filename);

	const std::string &getFilename() const;

	// Gets the dimensions of all levels in the map.
	WEInt getWidth() const;
	SNInt getDepth() const;

	// Gets the starting level when the player enters the area.
	int getStartingLevelIndex() const;

	// Starting points for the player in special 'centimeter-like' units.
	int getStartPointCount() const;
	const OriginalInt2 &getStartPoint(int index) const;

	// Get the levels associated with the .MIF file.
	int getLevelCount() const;
	const MIFLevel &getLevel(int index) const;
};

#endif
