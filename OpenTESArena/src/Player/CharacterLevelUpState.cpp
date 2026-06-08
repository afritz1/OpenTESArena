#include "CharacterLevelUpState.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Player/Player.h"

CharacterLevelUpState::CharacterLevelUpState()
{
	this->clear();
}

void CharacterLevelUpState::init(const Player &player, int bonusPoints)
{
	this->attributes = player.primaryAttributes;
	this->maxHealth = static_cast<int>(player.maxHealth);
	this->maxStamina = static_cast<int>(player.maxStamina);
	this->maxSpellPoints = static_cast<int>(player.maxSpellPoints);
	this->bonusPoints = bonusPoints;
	this->clearChangedPoints();
}

void CharacterLevelUpState::clearChangedPoints()
{
	std::fill(std::begin(this->changedPoints), std::end(this->changedPoints), 0);
}

void CharacterLevelUpState::clear()
{
	this->attributes.clear();
	this->derivedAttributes.clear();
	this->maxHealth = 0;
	this->maxStamina = 0;
	this->maxSpellPoints = 0;
	this->bonusPoints = 0;
	this->clearChangedPoints();
}
