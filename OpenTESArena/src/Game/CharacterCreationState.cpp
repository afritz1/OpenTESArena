#include <cstdio>

#include "CharacterCreationState.h"

CharacterCreationState::CharacterCreationState()
{
	this->clear();
}

const std::string_view CharacterCreationState::getName() const
{
	return this->name.data();
}

int CharacterCreationState::getClassDefID() const
{
	return this->classDefID;
}

int CharacterCreationState::getRaceIndex() const
{
	return this->raceIndex;
}

int CharacterCreationState::getStrength() const
{
	return this->strength;
}

int CharacterCreationState::getIntelligence() const
{
	return this->intelligence;
}

int CharacterCreationState::getWillpower() const
{
	return this->willpower;
}

int CharacterCreationState::getAgility() const
{
	return this->agility;
}

int CharacterCreationState::getSpeed() const
{
	return this->speed;
}

int CharacterCreationState::getEndurance() const
{
	return this->endurance;
}

int CharacterCreationState::getPersonality() const
{
	return this->personality;
}

int CharacterCreationState::getLuck() const
{
	return this->luck;
}

int CharacterCreationState::getPortraitIndex() const
{
	return this->portraitIndex;
}

bool CharacterCreationState::isMale() const
{
	return this->male;
}

void CharacterCreationState::setName(const char *name)
{
	std::snprintf(this->name.data(), this->name.size(), "%s", name != nullptr ? name : "");
}

void CharacterCreationState::setClassDefID(int id)
{
	this->classDefID = id;
}

void CharacterCreationState::setRaceIndex(int index)
{
	this->raceIndex = index;
	this->setPrimaryAttributes();
}

int CharacterCreationState::rollPrimaryAttribute(int base)
{
	return base + (this->random.next() % 20) + 1;
}

void CharacterCreationState::setPrimaryAttributes()
{
	// Source: https://en.uesp.net/wiki/Arena:Character_Creation#Character_Stats
	if (this->raceIndex == 0) // Breton
	{
		this->strength = this->rollPrimaryAttribute(30);
		this->intelligence = this->rollPrimaryAttribute(50);
		this->willpower = this->rollPrimaryAttribute(50);
		this->agility = this->rollPrimaryAttribute(40);
		this->speed = this->rollPrimaryAttribute(40);
		this->endurance = this->rollPrimaryAttribute(30);
		this->personality = this->rollPrimaryAttribute(40);
		this->luck = this->rollPrimaryAttribute(40);
	}
	if (this->raceIndex == 1) // Redguard
	{
		this->strength = this->rollPrimaryAttribute(male ? 40 : 30);
		this->intelligence = this->rollPrimaryAttribute(30);
		this->willpower = this->rollPrimaryAttribute(30);
		this->agility = this->rollPrimaryAttribute(male ? 40 : 50);
		this->speed = this->rollPrimaryAttribute(50);
		this->endurance = this->rollPrimaryAttribute(50);
		this->personality = this->rollPrimaryAttribute(40);
		this->luck = this->rollPrimaryAttribute(40);
	}
	if (this->raceIndex == 2) // Nord
	{
		this->strength = this->rollPrimaryAttribute(male ? 50 : 40);
		this->intelligence = this->rollPrimaryAttribute(30);
		this->willpower = this->rollPrimaryAttribute(male ? 30 : 40);
		this->agility = this->rollPrimaryAttribute(male ? 30 : 40);
		this->speed = this->rollPrimaryAttribute(40);
		this->endurance = this->rollPrimaryAttribute(male ? 50 : 40);
		this->personality = this->rollPrimaryAttribute(40);
		this->luck = this->rollPrimaryAttribute(male ? 40 : 50);
	}
	if (this->raceIndex == 3) // Dark Elf
	{
		this->strength = this->rollPrimaryAttribute(male ? 50 : 40);
		this->intelligence = this->rollPrimaryAttribute(50);
		this->willpower = this->rollPrimaryAttribute(30);
		this->agility = this->rollPrimaryAttribute(male ? 50 : 40);
		this->speed = this->rollPrimaryAttribute(male ? 50 : 40);
		this->endurance = this->rollPrimaryAttribute(40);
		this->personality = this->rollPrimaryAttribute(40);
		this->luck = this->rollPrimaryAttribute(40);
	}
	if (this->raceIndex == 4) // High Elf
	{
		this->strength = this->rollPrimaryAttribute(30);
		this->intelligence = this->rollPrimaryAttribute(50);
		this->willpower = this->rollPrimaryAttribute(male ? 50 : 40);
		this->agility = this->rollPrimaryAttribute(40);
		this->speed = this->rollPrimaryAttribute(40);
		this->endurance = this->rollPrimaryAttribute(30);
		this->personality = this->rollPrimaryAttribute(male ? 40 : 50);
		this->luck = this->rollPrimaryAttribute(40);
	}
	if (this->raceIndex == 5) // Wood Elf
	{
		this->strength = this->rollPrimaryAttribute(40);
		this->intelligence = this->rollPrimaryAttribute(40);
		this->willpower = this->rollPrimaryAttribute(40);
		this->agility = this->rollPrimaryAttribute(male ? 50 : 40);
		this->speed = this->rollPrimaryAttribute(male ? 50 : 40);
		this->endurance = this->rollPrimaryAttribute(male ? 30 : 40);
		this->personality = this->rollPrimaryAttribute(40);
		this->luck = this->rollPrimaryAttribute(male ? 30 : 40);
	}
	if (this->raceIndex == 6) // Khajiit
	{
		this->strength = this->rollPrimaryAttribute(40);
		this->intelligence = this->rollPrimaryAttribute(40);
		this->willpower = this->rollPrimaryAttribute(30);
		this->agility = this->rollPrimaryAttribute(male ? 50 : 40);
		this->speed = this->rollPrimaryAttribute(male ? 40 : 50);
		this->endurance = this->rollPrimaryAttribute(30);
		this->personality = this->rollPrimaryAttribute(40);
		this->luck = this->rollPrimaryAttribute(50);
	}
	if (this->raceIndex == 7) // Argonian
	{
		this->strength = this->rollPrimaryAttribute(40);
		this->intelligence = this->rollPrimaryAttribute(40);
		this->willpower = this->rollPrimaryAttribute(40);
		this->agility = this->rollPrimaryAttribute(male ? 50 : 40);
		this->speed = this->rollPrimaryAttribute(male ? 50 : 40);
		this->endurance = this->rollPrimaryAttribute(male ? 30 : 40);
		this->personality = this->rollPrimaryAttribute(40);
		this->luck = this->rollPrimaryAttribute(male ? 30 : 40);
	}
}

void CharacterCreationState::setPortraitIndex(int index)
{
	this->portraitIndex = index;
}

void CharacterCreationState::setGender(bool male)
{
	this->male = male;
}

void CharacterCreationState::clear()
{
	this->name.fill('\0');
	this->classDefID = CharacterCreationState::NO_INDEX;
	this->raceIndex = CharacterCreationState::NO_INDEX;
	this->portraitIndex = CharacterCreationState::NO_INDEX;
	this->male = false;
}
