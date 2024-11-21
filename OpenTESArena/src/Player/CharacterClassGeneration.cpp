#include <algorithm>
#include <string>

#include "CharacterClassGeneration.h"

#include "components/debug/Debug.h"

CharacterClassGeneration::ClassData::ClassData()
{
	this->id = 0;
	this->isSpellcaster = false;
	this->hasCriticalHit = false;
	this->isThief = false;
}

CharacterClassGeneration::ChoiceData::ChoiceData()
{
	this->a = 0;
	this->b = 0;
	this->c = 0;
}

const CharacterClassGeneration::ClassData &CharacterClassGeneration::getClassData(
	int a, int b, int c) const
{
	// A maximum of ten answers for any category is the limit.
	DebugAssert((a >= 0) && (a <= 10));
	DebugAssert((b >= 0) && (b <= 10));
	DebugAssert((c >= 0) && (c <= 10));

	// Find the index of the given A/B/C counts in the choices array.
	const int choiceID = [this, a, b, c]()
	{
		// Search each choice for the matching A/B/C triplet.
		const auto iter = std::find_if(this->choices.begin(), this->choices.end(),
			[a, b, c](const CharacterClassGeneration::ChoiceData &choice)
		{
			return (choice.a == a) && (choice.b == b) && (choice.c == c);
		});

		DebugAssertMsg(iter != this->choices.end(), "No class mapping found (a:" +
			std::to_string(a) + ", b:" + std::to_string(b) + ", c:" + std::to_string(c) + ").");
		return static_cast<int>(std::distance(this->choices.begin(), iter));
	}();

	// Calculate the class ID from the choice ID.
	const int classID = (choiceID < 48) ? (choiceID / 4) : (12 + ((choiceID - 48) / 3));

	// Get the class data associated with the class ID.
	return this->classes.at(classID);
}
