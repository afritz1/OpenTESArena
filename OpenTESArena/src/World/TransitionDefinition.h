#ifndef TRANSITION_DEFINITION_H
#define TRANSITION_DEFINITION_H

#include "MapGeneration.h"
#include "TransitionType.h"

// @todo: share with both voxels and entities; probably delete EntityDefinition
// variant of this class.

class TransitionDefinition
{
public:
	struct InteriorEntranceDef
	{
		MapGeneration::InteriorGenInfo interiorGenInfo;

		void init(MapGeneration::InteriorGenInfo &&interiorGenInfo);
	};

	struct LevelChangeDef
	{
		bool isLevelUp;

		void init(bool isLevelUp);
	};
private:
	TransitionType type;
	InteriorEntranceDef interiorEntrance;
	LevelChangeDef levelChange;

	void init(TransitionType type);
public:
	TransitionDefinition();

	void initCityGate();
	void initInteriorEntrance(MapGeneration::InteriorGenInfo &&interiorGenInfo);
	void initInteriorExit();
	void initLevelChange(bool isLevelUp);

	TransitionType getType() const;
	const InteriorEntranceDef &getInteriorEntrance() const;
	const LevelChangeDef &getLevelChange() const;
};

#endif
