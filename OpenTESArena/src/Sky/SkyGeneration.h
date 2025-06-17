#ifndef SKY_GENERATION_H
#define SKY_GENERATION_H

#include <cstdint>

#include "../Assets/ArenaTypes.h"
#include "../Weather/WeatherDefinition.h"

#include "components/utilities/Buffer.h"

class BinaryAssetLibrary;
class SkyDefinition;
class SkyInfoDefinition;
class TextureManager;

struct SkyGenerationInteriorInfo
{
	bool outdoorDungeon;

	void init(bool outdoorDungeon);
};

struct SkyGenerationExteriorInfo
{
	ArenaClimateType climateType; // Only cities have climate.
	WeatherDefinition weatherDef;
	int currentDay;
	int starCount;
	uint32_t citySeed;
	uint32_t skySeed;
	bool provinceHasAnimatedLand;

	void init(ArenaClimateType climateType, const WeatherDefinition &weatherDef, int currentDay,
		int starCount, uint32_t citySeed, uint32_t skySeed, bool provinceHasAnimatedLand);
};

namespace SkyGeneration
{
	void generateInteriorSky(const SkyGenerationInteriorInfo &skyGenInfo, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef);
	void generateExteriorSky(const SkyGenerationExteriorInfo &skyGenInfo, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef);
}

#endif
