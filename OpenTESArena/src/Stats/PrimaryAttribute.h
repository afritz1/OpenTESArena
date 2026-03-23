#ifndef PRIMARY_ATTRIBUTE_H
#define PRIMARY_ATTRIBUTE_H

#include "components/utilities/Span.h"

struct ExeData;

using PrimaryAttributeID = int;

struct PrimaryAttribute
{
	char name[32];
	int maxValue;

	PrimaryAttribute();
	
	void init(const char *name, int maxValue);
	void clear();
};

struct PrimaryAttributes
{
	static constexpr int COUNT = 8;

	PrimaryAttribute strength, intelligence, willpower, agility, speed, endurance, personality, luck;

	void init(int raceID, bool isMale, const ExeData &exeData);

	Span<PrimaryAttribute> getView();
	Span<const PrimaryAttribute> getView() const;

	void clear();
};

struct DerivedAttributes
{
	static constexpr int COUNT = 8;

	int bonusDamage;
	int maxKilos;
	int magicDef;
	int bonusToHit;
	int bonusToDefend;
	int bonusToHealth;
	int healMod;
	int bonusToCharisma;

	DerivedAttributes();

	static bool isModifier(int index);

	Span<const int> getView() const;

	void clear();
};

#endif
