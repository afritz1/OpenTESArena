#ifndef SKY_INFO_DEFINITION_H
#define SKY_INFO_DEFINITION_H

#include <vector>

#include "SkyAirDefinition.h"
#include "SkyDefinition.h"
#include "SkyLandDefinition.h"
#include "SkyMoonDefinition.h"
#include "SkyStarDefinition.h"
#include "SkySunDefinition.h"

// Contains object definitions pointed to by a sky definition.

class SkyInfoDefinition
{
private:
	std::vector<SkyLandDefinition> lands;
	std::vector<SkyAirDefinition> airs;
	std::vector<SkyStarDefinition> stars;
	std::vector<SkySunDefinition> suns;
	std::vector<SkyMoonDefinition> moons;
public:
	const SkyLandDefinition &getLand(SkyDefinition::LandDefID id) const;
	const SkyAirDefinition &getAir(SkyDefinition::AirDefID id) const;
	const SkyStarDefinition &getStar(SkyDefinition::StarDefID id) const;
	const SkySunDefinition &getSun(SkyDefinition::SunDefID id) const;
	const SkyMoonDefinition &getMoon(SkyDefinition::MoonDefID id) const;

	SkyDefinition::LandDefID addLand(SkyLandDefinition &&def);
	SkyDefinition::AirDefID addAir(SkyAirDefinition &&def);
	SkyDefinition::StarDefID addStar(SkyStarDefinition &&def);
	SkyDefinition::SunDefID addSun(SkySunDefinition &&def);
	SkyDefinition::MoonDefID addMoon(SkyMoonDefinition &&def);
};

#endif
