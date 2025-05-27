#include <algorithm>
#include <string>

#include "CharacterClassGeneration.h"

#include "components/debug/Debug.h"

CharacterClassGenerationClass::CharacterClassGenerationClass()
{
	this->id = 0;
	this->isSpellcaster = false;
	this->hasCriticalHit = false;
	this->isThief = false;
}

CharacterClassGenerationChoice::CharacterClassGenerationChoice()
{
	this->a = 0;
	this->b = 0;
	this->c = 0;
}

int CharacterClassGeneration::getClassIndex(int a, int b, int c) const
{
	int choiceIndex = -1;
	for (int i = 0; i < static_cast<int>(std::size(this->choices)); i++)
	{
		const CharacterClassGenerationChoice &choice = this->choices[i];
		if ((choice.a == a) && (choice.b == b) && (choice.c == c))
		{
			choiceIndex = i;
			break;
		}
	}

	if (choiceIndex == -1)
	{
		DebugLogWarningFormat("No character class mapping found (a: %d, b: %d, c: %d).", a, b, c);
		return -1;
	}

	const int classIndex = (choiceIndex < 48) ? (choiceIndex / 4) : (12 + ((choiceIndex - 48) / 3));
	return classIndex;
}
