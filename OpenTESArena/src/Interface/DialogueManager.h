#pragma once

#include <string>

#include "../Entities/EntityInstance.h"

struct DialogueManager
{
	Game *game;
	EntityInstanceID entityInstID; // The entity the player is talking to.
	// @todo various global state like dlgGender that changes based on processing of certain tokens

	void init(Game &game);

	void beginDialogue(EntityInstanceID entityInstID);
	void endDialogue();

	const EntityInstance &getEntityInstance() const;
	ArenaNpcPersonalityType getEntityPersonalityType() const;
	bool hasEntityBeenIntroduced() const;

	const std::string &getTemplateDatEntryValueAtIndex(int entryKey, int index) const;
	const std::string &getRandomTemplateDatEntryValue(int entryKey) const;

	// Scans the text for substitution tokens and returns a fully replaced string for display.
	std::string getSubstitutedText(const char *text, int maxCharsPerLine = 65) const;
};
