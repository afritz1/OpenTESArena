#pragma once

#include "../Stats/PrimaryAttribute.h"

struct Player;

struct CharacterLevelUpState
{
	static constexpr int BonusPointsRandomMin = 4;
	static constexpr int BonusPointsRandomMax = 6;

	PrimaryAttributes attributes;
	DerivedAttributes derivedAttributes;
	int maxHealth, maxStamina, maxSpellPoints;
	int bonusPoints;
	int changedPoints[8]; // For primary attributes

	CharacterLevelUpState();

	void init(const Player &player, int bonusPoints);

	void clearChangedPoints();
	void clear();
};
