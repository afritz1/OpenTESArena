#ifndef INTERIOR_LEVEL_DATA_H
#define INTERIOR_LEVEL_DATA_H

#include "LevelData.h"

class InteriorLevelData : public LevelData
{
private:
	// All interiors have the same grid height.
	static const int GRID_HEIGHT;

	std::unordered_map<Int2, LevelData::TextTrigger> textTriggers;
	std::unordered_map<Int2, std::string> soundTriggers;

	// Exteriors have dynamic sky palettes, so sky color can only be stored by interiors (for the
	// purposes of background fill, fog, etc.).
	uint32_t skyColor;

	bool outdoorDungeon;

	InteriorLevelData(int gridWidth, int gridDepth, const std::string &infName,
		const std::string &name);

	void readTriggers(const std::vector<ArenaTypes::MIFTrigger> &triggers, const INFFile &inf,
		int width, int depth);
public:
	InteriorLevelData(InteriorLevelData&&) = default;
	virtual ~InteriorLevelData();

	// Interior level. The .INF is obtained from the level's info member.
	static InteriorLevelData loadInterior(const MIFFile::Level &level, int gridWidth,
		int gridDepth, const ExeData &exeData);

	// Dungeon level. Each chunk is determined by an "inner seed" which depends on the
	// dungeon level count being calculated beforehand.
	static InteriorLevelData loadDungeon(ArenaRandom &random,
		const std::vector<MIFFile::Level> &levels, int levelUpBlock, const int *levelDownBlock,
		int widthChunks, int depthChunks, const std::string &infName, int gridWidth, int gridDepth,
		const ExeData &exeData);

	// Returns a pointer to some trigger text if the given voxel has a text trigger, or
	// null if it doesn't. Also returns a pointer to one-shot text triggers that have 
	// been activated previously (use another function to check activation).
	LevelData::TextTrigger *getTextTrigger(const Int2 &voxel);

	// Returns a pointer to a sound filename if the given voxel has a sound trigger, or
	// null if it doesn't.
	const std::string *getSoundTrigger(const Int2 &voxel) const;

	// Some interiors are considered "outdoor dungeons", which have a different sky color
	// and day/night behavior.
	virtual bool isOutdoorDungeon() const override;

	// Calls the base level data method then does some interior-specific work.
	virtual void setActive(bool nightLightsAreActive, const WorldData &worldData,
		const Location &location, const MiscAssets &miscAssets, TextureManager &textureManager,
		Renderer &renderer) override;
};

#endif
