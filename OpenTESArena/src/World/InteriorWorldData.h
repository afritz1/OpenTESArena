#ifndef INTERIOR_WORLD_DATA_H
#define INTERIOR_WORLD_DATA_H

#include <cstdint>

#include "InteriorLevelData.h"
#include "VoxelDefinition.h"
#include "WorldData.h"
#include "../Assets/INFFile.h"

class ExeData;
class MIFFile;

class InteriorWorldData : public WorldData
{
private:
	std::vector<InteriorLevelData> levels;
	VoxelDefinition::WallData::MenuType interiorType;
	int levelIndex;

	InteriorWorldData();
public:
	InteriorWorldData(InteriorWorldData&&) = default;
	virtual ~InteriorWorldData();

	InteriorWorldData &operator=(InteriorWorldData&&) = default;

	static InteriorWorldData loadInterior(VoxelDefinition::WallData::MenuType interiorType,
		const MIFFile &mif, const ExeData &exeData);
	static InteriorWorldData loadDungeon(uint32_t seed, WEInt widthChunks, SNInt depthChunks,
		bool isArtifactDungeon, VoxelDefinition::WallData::MenuType interiorType,
		const ExeData &exeData);

	// Gets the currently selected level's index.
	int getLevelIndex() const;

	// Gets the number of levels in the interior.
	int getLevelCount() const;

	// Gets the type of the interior (mostly needed for checking if it's a palace).
	VoxelDefinition::WallData::MenuType getInteriorType() const;

	// Always interior for interior world data.
	virtual WorldType getBaseWorldType() const override;
	virtual WorldType getActiveWorldType() const override;

	virtual LevelData &getActiveLevel() override;
	virtual const LevelData &getActiveLevel() const override;

	// Sets which level is considered the active one.
	void setLevelIndex(int levelIndex);
};

#endif
