#ifndef SKY_GENERATION_H
#define SKY_GENERATION_H

#include <cstdint>

#include "../Assets/ArenaTypes.h"
#include "../Weather/WeatherDefinition.h"

#include "components/utilities/Buffer.h"

class BinaryAssetLibrary;
class Random;
class SkyDefinition;
class SkyInfoDefinition;
class TextureManager;

namespace SkyGeneration
{
	struct InteriorSkyGenInfo
	{
		bool outdoorDungeon;

		void init(bool outdoorDungeon);
	};

	struct ExteriorSkyGenInfo
	{
		ArenaTypes::ClimateType climateType; // Only cities have climate.
		int currentDay;
		int starCount;
		uint32_t citySeed;
		uint32_t skySeed;
		bool provinceHasAnimatedLand;
		Random *randomPtr;

		void init(ArenaTypes::ClimateType climateType, int currentDay, int starCount, uint32_t citySeed,
			uint32_t skySeed, bool provinceHasAnimatedLand, Random &random);
	};

	void generateInteriorSky(const InteriorSkyGenInfo &skyGenInfo, TextureManager &textureManager, SkyDefinition *outSkyDef);
	void generateInteriorSkyInfo(SkyInfoDefinition *outSkyInfoDef);

	void generateExteriorSky(const ExteriorSkyGenInfo &skyGenInfo, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef);
	void generateExteriorSkyInfo(SkyInfoDefinition *outSkyInfoDef);
}

#endif
