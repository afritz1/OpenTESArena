#include <cstdio>

#include "CharacterCreationState.h"
#include "../Assets/BinaryAssetLibrary.h"

#include "components/debug/Debug.h"

CharacterCreationState::CharacterCreationState()
{
	this->clear();
}

void CharacterCreationState::setName(const char *name)
{
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name != nullptr ? name : "");
}

void CharacterCreationState::populateBaseAttributes()
{
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	this->attributes.init(this->raceIndex, this->male, exeData);
}

void CharacterCreationState::clearChangedPoints()
{
	std::fill(std::begin(this->changedPoints), std::end(this->changedPoints), 0);
}

void CharacterCreationState::clear()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->classDefID = CharacterCreationState::NO_INDEX;
	this->raceIndex = CharacterCreationState::NO_INDEX;
	this->portraitIndex = CharacterCreationState::NO_INDEX;
	this->male = false;
	this->attributes.clear();
	this->derivedAttributes.clear();
	this->maxHealth = 0;
	this->maxStamina = 0;
	this->maxSpellPoints = 0;
	this->gold = 0;
	this->bonusPoints = 0;
	this->clearChangedPoints();
}
