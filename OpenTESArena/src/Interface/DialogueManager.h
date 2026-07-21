#pragma once

#include <string>

#include "DialogueFunctions.h"
#include "../Entities/EntityInstance.h"

#include "components/utilities/Buffer.h"

struct DialogueManager
{
	Game *game;

	// Sorted by longest first so longer tokens match before shorter ones.
	Buffer<std::pair<const char*, DialogueFunction>> sortedFunctionMappings;

	// The entity the player is talking to.
	EntityInstanceID entityInstID;
	// @todo various global state like dlgGender that changes based on processing of certain tokens

	void init(Game &game);

	void beginDialogue(EntityInstanceID entityInstID);
	void endDialogue();

	const EntityInstance &getEntityInstance() const;
	ArenaNpcPersonalityType getEntityPersonalityType() const;
	bool hasEntityBeenIntroduced() const;
	WorldDouble2 getEntityPosition() const;
	int getEntityOccupationIndex() const;
	std::string getNearestEquipmentStoreName() const;
	std::string getNearestTavernName() const;
	std::string getNearestTempleName() const;

	const std::string &getTemplateDatEntryValueAtIndex(int entryKey, int index) const;
	const std::string &getRandomTemplateDatEntryValue(int entryKey) const;

	// Scans the text for substitution tokens and returns a fully replaced string for display.
	std::string getSubstitutedText(const char *text, int maxCharsPerLine = 65) const;
};
