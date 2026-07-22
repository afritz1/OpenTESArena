#pragma once

#include <string>

#include "DialogueFunctions.h"
#include "../Entities/EntityInstance.h"

#include "components/utilities/Buffer.h"

// Each entry in the "Where is..." list box.
struct DialogueDirectionsEntry
{
	std::string displayString;
	// @todo callback

	DialogueDirectionsEntry();
};

struct DialogueManager
{
	Game *game;

	// Sorted by longest first so longer tokens match before shorter ones.
	Buffer<std::pair<const char*, DialogueFunction>> sortedFunctionMappings;

	// Copies of dialogue directions for "Where is..." UI. Sometimes more entries (e.g. main quest) are appended afterwards.
	std::vector<DialogueDirectionsEntry> cityDirectionsEntries;
	std::vector<DialogueDirectionsEntry> wildernessDirectionsEntries;

	// The entity the player is talking to.
	EntityInstanceID entityInstID;
	int dialogueGender; // 0: male, 1: female, 2: neutral

	DialogueManager();

	void init(Game &game);

	void beginDialogue(EntityInstanceID entityInstID);
	void endDialogue();

	const EntityInstance &getEntityInstance() const;
	ArenaNpcPersonalityType getEntityPersonalityType() const;
	bool hasEntityBeenIntroduced() const;
	WorldDouble2 getEntityPosition() const;
	int getEntityOccupationIndex() const;
	bool isDialogueGenderValid() const;
	std::string getNearestEquipmentStoreName() const;
	std::string getNearestTavernName() const;
	std::string getNearestTempleName() const;

	const std::string &getTemplateDatEntryValueAtIndex(int entryKey, int index) const;
	const std::string &getRandomTemplateDatEntryValue(int entryKey) const;

	// Scans the text for substitution tokens and returns a fully replaced string for display.
	std::string getSubstitutedText(const char *text, int maxCharsPerLine = 65);
};
