#pragma once

#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/PrimaryAttribute.h"

struct CharacterCreationState
{
	static constexpr int MAX_NAME_LENGTH = 25;

	char name[MAX_NAME_LENGTH + 1];
	CharacterClassDefinitionID charClassDefID;
	int raceIndex;
	int portraitIndex;
	bool male;
	PrimaryAttributes attributes;
	DerivedAttributes derivedAttributes;
	int maxHealth, maxStamina, maxSpellPoints;
	int gold;
	int bonusPoints;
	int changedPoints[8]; // For primary attributes

	CharacterCreationState();

	void setName(const char *name);
	void populateBaseAttributes();
	void clearChangedPoints();

	void clear();
};
