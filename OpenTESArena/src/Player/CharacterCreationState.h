#ifndef CHARACTER_CREATION_STATE_H
#define CHARACTER_CREATION_STATE_H

#include "../Stats/PrimaryAttribute.h"

struct CharacterCreationState
{
	static constexpr int MAX_NAME_LENGTH = 25;
	static constexpr int NO_INDEX = -1;

	char name[MAX_NAME_LENGTH + 1];
	int classDefID;
	int raceIndex;
	int portraitIndex;
	bool male;
	PrimaryAttributes attributes;
	DerivedAttributes derivedAttributes;
	int bonusPoints;
	int changedPoints[8]; // For primary attributes

	CharacterCreationState();

	void setName(const char *name);
	void populateBaseAttributes();
	void clearChangedPoints();

	void clear();
};

#endif
