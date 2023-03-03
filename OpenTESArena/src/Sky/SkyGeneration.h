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
		WeatherDefinition weatherDef;
		int currentDay;
		int starCount;
		uint32_t citySeed;
		uint32_t skySeed;
		bool provinceHasAnimatedLand;

		void init(ArenaTypes::ClimateType climateType, const WeatherDefinition &weatherDef, int currentDay,
			int starCount, uint32_t citySeed, uint32_t skySeed, bool provinceHasAnimatedLand);
	};

	void generateInteriorSky(const InteriorSkyGenInfo &skyGenInfo, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef);
	void generateExteriorSky(const ExteriorSkyGenInfo &skyGenInfo,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef);
}

#endif
