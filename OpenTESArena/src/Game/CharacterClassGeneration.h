#ifndef CHARACTER_CLASS_GENERATION_H
#define CHARACTER_CLASS_GENERATION_H

#include <array>
#include <cstdint>

// A record for keeping data from CLASSES.DAT in a nicer format. It is used with
// the character questions for generating a suggested class for the player.

struct CharacterClassGeneration
{
	struct ClassData
	{
		int id;
		bool isSpellcaster, hasCriticalHit, isThief;
	};

	struct ChoiceData { uint8_t a, b, c; };

	static const int ID_MASK;
	static const int SPELLCASTER_MASK;
	static const int CRITICAL_HIT_MASK;
	static const int THIEF_MASK;

	std::array<ClassData, 18> classes;
	std::array<ChoiceData, 66> choices;

	// Gets the class associated with some A/B/C question count. The parameter 
	// is the index where that A/B/C combination lives in the Choices array.
	const ClassData &getClassData(int a, int b, int c) const;
};

#endif
