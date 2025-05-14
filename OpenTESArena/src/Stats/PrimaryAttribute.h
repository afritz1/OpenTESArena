#ifndef PRIMARY_ATTRIBUTE_H
#define PRIMARY_ATTRIBUTE_H

#include "components/utilities/BufferView.h"

struct ExeData;

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

	BufferView<PrimaryAttribute> getAttributes();
	BufferView<const PrimaryAttribute> getAttributes() const;

	void clear();
};

#endif
