#pragma once

#include <cstdint>
#include <vector>

#include "components/utilities/Singleton.h"

struct ExeData;

// @todo: this is basically an ArenaCreatureDefinition since it copy-pastes so much from ExeData.
struct CreatureDefinition
{
	char name[64];
	int level;
	int minHP;
	int maxHP;
	int baseExp;
	int expMultiplier;
	int soundIndex;
	char soundName[32];
	int minDamage;
	int maxDamage;
	int magicEffects;
	int scale;
	int yOffset;
	bool hasCorpse;
	int bloodIndex; // @todo this should be an EntityDefID to the vfx in EntityDefinitionLibrary, or -1
	int diseaseChances;
	int attributes[8];
	uint32_t lootChances;
	bool ghost;

	void init(int creatureIndex, bool isFinalBoss, const ExeData &exeData);

	bool operator==(const CreatureDefinition &other) const;
};

using CreatureDefinitionID = int;

class CreatureDefinitionLibrary : public Singleton<CreatureDefinitionLibrary>
{
private:
	std::vector<CreatureDefinition> entries;
public:
	void init(const ExeData &exeData);

	int getDefinitionCount() const;

	const CreatureDefinition &getDefinition(CreatureDefinitionID id) const;
};
