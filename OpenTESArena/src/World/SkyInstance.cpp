#include "AirObjectDefinition.h"
#include "LandObjectDefinition.h"
#include "MoonObjectDefinition.h"
#include "SkyDefinition.h"
#include "SkyInfoDefinition.h"
#include "SkyInstance.h"
#include "StarObjectDefinition.h"
#include "SunObjectDefinition.h"

#include "components/debug/Debug.h"

void SkyInstance::init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition)
{
	// Spawn all sky objects from the ready-to-bake format.
	for (int i = 0; i < skyDefinition.getLandPlacementDefCount(); i++)
	{
		const SkyDefinition::LandPlacementDef &placementDef = skyDefinition.getLandPlacementDef(i);
		const SkyDefinition::LandDefID defID = placementDef.id;
		const LandObjectDefinition &objectDef = skyInfoDefinition.getLand(defID);

		for (const Radians position : placementDef.positions)
		{
			// Convert radians to direction.
			DebugNotImplemented();
		}
	}

	for (int i = 0; i < skyDefinition.getAirPlacementDefCount(); i++)
	{
		const SkyDefinition::AirPlacementDef &placementDef = skyDefinition.getAirPlacementDef(i);
		const SkyDefinition::AirDefID defID = placementDef.id;
		const AirObjectDefinition &objectDef = skyInfoDefinition.getAir(defID);

		for (const std::pair<Radians, Radians> &position : placementDef.positions)
		{
			// Convert X and Y radians to direction.
			DebugNotImplemented();
		}
	}

	for (int i = 0; i < skyDefinition.getStarPlacementDefCount(); i++)
	{
		const SkyDefinition::StarPlacementDef &placementDef = skyDefinition.getStarPlacementDef(i);
		const SkyDefinition::StarDefID defID = placementDef.id;
		const StarObjectDefinition &objectDef = skyInfoDefinition.getStar(defID);

		for (const Double3 &position : placementDef.positions)
		{
			// Use star direction directly.
			DebugNotImplemented();
		}
	}

	for (int i = 0; i < skyDefinition.getSunPlacementDefCount(); i++)
	{
		const SkyDefinition::SunPlacementDef &placementDef = skyDefinition.getSunPlacementDef(i);
		const SkyDefinition::SunDefID defID = placementDef.id;
		const SunObjectDefinition &objectDef = skyInfoDefinition.getSun(defID);

		for (const double position : placementDef.positions)
		{
			// Convert starting sun latitude to direction.
			DebugNotImplemented();
		}
	}

	for (int i = 0; i < skyDefinition.getMoonPlacementDefCount(); i++)
	{
		const SkyDefinition::MoonPlacementDef &placementDef = skyDefinition.getMoonPlacementDef(i);
		const SkyDefinition::MoonDefID defID = placementDef.id;
		const MoonObjectDefinition &objectDef = skyInfoDefinition.getMoon(defID);

		for (const SkyDefinition::MoonPlacementDef::Position &position : placementDef.positions)
		{
			// Convert moon position to direction.
			DebugNotImplemented();
		}
	}
}
