#ifndef EXTERIOR_LEVEL_DATA_H
#define EXTERIOR_LEVEL_DATA_H

#include "LevelData.h"

class ExteriorLevelData : public LevelData
{
private:
	ExteriorLevelData(int gridWidth, int gridHeight, int gridDepth, const std::string &infName,
		const std::string &name);
public:
	ExteriorLevelData(ExteriorLevelData&&) = default;
	virtual ~ExteriorLevelData();

	// Premade exterior level with a pre-defined .INF file. Only used by center province.
	static ExteriorLevelData loadPremadeCity(const MIFFile::Level &level,
		const std::string &infName, int gridWidth, int gridDepth, const ExeData &exeData);

	// Exterior level with a pre-defined .INF file (for randomly generated cities). This loads
	// the skeleton of the level (city walls, etc.), and fills in the rest by loading the
	// required .MIF chunks.
	static ExteriorLevelData loadCity(const MIFFile::Level &level, uint32_t citySeed, int cityDim,
		const std::vector<uint8_t> &reservedBlocks, const Int2 &startPosition,
		const std::string &infName, int gridWidth, int gridDepth, const ExeData &exeData);

	// Wilderness with a pre-defined .INF file. This loads the skeleton of the wilderness
	// and fills in the rest by loading the required .RMD chunks.
	static ExteriorLevelData loadWilderness(int rmdTR, int rmdTL, int rmdBR, int rmdBL,
		const std::string &infName, const ExeData &exeData);

	// Exteriors are never outdoor dungeons (always false).
	virtual bool isOutdoorDungeon() const override;
};

#endif
