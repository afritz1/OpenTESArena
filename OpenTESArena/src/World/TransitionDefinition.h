#ifndef TRANSITION_DEFINITION_H
#define TRANSITION_DEFINITION_H

#include "MapGeneration.h"
#include "TransitionType.h"

struct InteriorEntranceTransitionDefinition
{
	MapGeneration::InteriorGenInfo interiorGenInfo;

	void init(MapGeneration::InteriorGenInfo &&interiorGenInfo);
};

struct InteriorLevelChangeTransitionDefinition
{
	bool isLevelUp;

	void init(bool isLevelUp);
};

// @todo: share with both voxels and entities; probably delete EntityDefinition variant of this class.
struct TransitionDefinition
{
	TransitionType type;
	InteriorEntranceTransitionDefinition interiorEntrance;
	InteriorLevelChangeTransitionDefinition interiorLevelChange;

	TransitionDefinition();

	void initCityGate();
	void initInteriorEntrance(MapGeneration::InteriorGenInfo &&interiorGenInfo);
	void initInteriorExit();
	void initInteriorLevelChange(bool isLevelUp);
};

#endif
