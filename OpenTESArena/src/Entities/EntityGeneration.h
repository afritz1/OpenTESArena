#pragma once

#include "EntityUtils.h"
#include "../WorldMap/ProvinceLibrary.h"

enum class ArenaCityType;
enum class ArenaInteriorType;

struct EntityGenInfo
{
	int playerLevel;
	bool nightLightsAreActive;

	// For loot generation. Defaults to 0 if undefined for the active scene.
	ArenaCityType cityType;
	ArenaInteriorType interiorType;
	int interiorLevelIndex;

	ProvinceDefinitionID provinceDefID;

	EntityGenInfo();

	void init(int playerLevel, bool nightLightsAreActive, ArenaCityType cityType, ArenaInteriorType interiorType,
		int interiorLevelIndex, ProvinceDefinitionID provinceDefID);
};
