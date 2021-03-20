#ifndef DOOR_DEFINITION_H
#define DOOR_DEFINITION_H

#include "../Assets/ArenaTypes.h"
#include "../Media/DoorSoundDefinition.h"

class DoorDefinition
{
private:
	ArenaTypes::DoorType type;
	DoorSoundDefinition doorSoundDef;
public:
	DoorDefinition();

	void init(ArenaTypes::DoorType type, DoorSoundDefinition &&doorSoundDef);

	ArenaTypes::DoorType getType() const;
	const DoorSoundDefinition &getDoorSoundDef() const;
};

#endif
