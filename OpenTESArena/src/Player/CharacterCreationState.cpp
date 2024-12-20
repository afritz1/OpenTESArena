#include <cstdio>

#include "CharacterCreationState.h"
#include "../Assets/BinaryAssetLibrary.h"

#include "components/debug/Debug.h"

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

const PrimaryAttributes &CharacterCreationState::getAttributes() const
{
	return this->attributes;
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
}

void CharacterCreationState::rollAttributes(Random &random)
{
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	this->attributes.init(this->raceIndex, this->male, exeData);
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
	this->attributes.clear();
}
