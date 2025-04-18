#ifndef CHARACTER_CLASS_GENERATION_H
#define CHARACTER_CLASS_GENERATION_H

#include <cstdint>

struct CharacterClassGenerationClass
{
	int id;
	bool isSpellcaster, hasCriticalHit, isThief;

	CharacterClassGenerationClass();
};

struct CharacterClassGenerationChoice
{
	uint8_t a, b, c;

	CharacterClassGenerationChoice();
};

// A record for keeping data from CLASSES.DAT in a nicer format. It is used with
// the character questions for generating a suggested class for the player.
struct CharacterClassGeneration
{
	static constexpr int ID_MASK = 0x1F;
	static constexpr int SPELLCASTER_MASK = 0x20;
	static constexpr int CRITICAL_HIT_MASK = 0x40;
	static constexpr int THIEF_MASK = 0x80;

	CharacterClassGenerationClass classes[18];
	CharacterClassGenerationChoice choices[66];

	// Gets the class associated with some A/B/C question count. The parameter 
	// is the index where that A/B/C combination lives in the Choices array.
	int getClassIndex(int a, int b, int c) const;
};

#endif
