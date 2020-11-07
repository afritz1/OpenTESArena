#ifndef SKY_GENERATION_H
#define SKY_GENERATION_H

#include <cstdint>

#include "../Media/Color.h"

#include "components/utilities/Buffer.h"

class BinaryAssetLibrary;
class SkyDefinition;
class SkyInstance;
class SkyObjectDefinition;
class SkyObjectInstance;

enum class ClimateType;
enum class WeatherType;

namespace SkyGeneration
{
	struct SkyGenInfo
	{
		WeatherType weatherType;
		ClimateType climateType; // Only cities have climate.
		int currentDay;
		int starCount;
		uint32_t citySeed;
		uint32_t skySeed;
		bool provinceHasAnimatedLand;

		void init(WeatherType weatherType, ClimateType climateType, int currentDay, int starCount,
			uint32_t citySeed, uint32_t skySeed, bool provinceHasAnimatedLand);
	};

	// @todo: this should take SkyDefinition* and SkyInfoDefinition* instead.
	void generateSky(const SkyGenInfo &skyGenInfo, const BinaryAssetLibrary &binaryAssetLibrary,
		SkyDefinition *outSkyDef, SkyInstance *outSkyInst);

	Buffer<Color> makeInteriorSkyColors(bool isOutdoorDungeon); // @todo: give palette also
	Buffer<Color> makeExteriorSkyColors(WeatherType weatherType);
}

#endif
