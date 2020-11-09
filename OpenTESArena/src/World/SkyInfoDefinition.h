#ifndef SKY_INFO_DEFINITION_H
#define SKY_INFO_DEFINITION_H

#include <vector>

#include "AirObjectDefinition.h"
#include "LandObjectDefinition.h"
#include "MoonObjectDefinition.h"
#include "SkyDefinition.h"
#include "StarObjectDefinition.h"
#include "SunObjectDefinition.h"

// Contains object definitions pointed to by a sky definition.

class SkyInfoDefinition
{
private:
	std::vector<LandObjectDefinition> lands;
	std::vector<AirObjectDefinition> airs;
	std::vector<StarObjectDefinition> stars;
	std::vector<SunObjectDefinition> suns;
	std::vector<MoonObjectDefinition> moons;
public:
	const LandObjectDefinition &getLand(SkyDefinition::LandDefID id) const;
	const AirObjectDefinition &getAir(SkyDefinition::AirDefID id) const;
	const StarObjectDefinition &getStar(SkyDefinition::StarDefID id) const;
	const SunObjectDefinition &getSun(SkyDefinition::SunDefID id) const;
	const MoonObjectDefinition &getMoon(SkyDefinition::MoonDefID id) const;

	SkyDefinition::LandDefID addLand(LandObjectDefinition &&def);
	SkyDefinition::AirDefID addAir(AirObjectDefinition &&def);
	SkyDefinition::StarDefID addStar(StarObjectDefinition &&def);
	SkyDefinition::SunDefID addSun(SunObjectDefinition &&def);
	SkyDefinition::MoonDefID addMoon(MoonObjectDefinition &&def);
};

#endif
