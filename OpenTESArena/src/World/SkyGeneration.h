#ifndef SKY_GENERATION_H
#define SKY_GENERATION_H

#include <cstdint>

#include "components/utilities/Buffer.h"

class BinaryAssetLibrary;
class SkyDefinition;
class SkyInfoDefinition;
class TextureManager;

enum class ClimateType;
enum class WeatherType;

namespace SkyGeneration
{
	struct InteriorSkyGenInfo
	{
		bool outdoorDungeon;

		void init(bool outdoorDungeon);
	};

	struct ExteriorSkyGenInfo
	{
		ClimateType climateType; // Only cities have climate.
		WeatherType weatherType;
		int currentDay;
		int starCount;
		uint32_t citySeed;
		uint32_t skySeed;
		bool provinceHasAnimatedLand;

		void init(ClimateType climateType, WeatherType weatherType, int currentDay, int starCount,
			uint32_t citySeed, uint32_t skySeed, bool provinceHasAnimatedLand);
	};

	void generateInteriorSky(const InteriorSkyGenInfo &skyGenInfo, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef);
	void generateExteriorSky(const ExteriorSkyGenInfo &skyGenInfo,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef);
}

#endif
