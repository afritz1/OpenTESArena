#include <algorithm>
#include <cstdio>

#include "ArenaAnimUtils.h"
#include "CreatureDefinitionLibrary.h"
#include "../Assets/BinaryAssetLibrary.h"

#include "components/debug/Debug.h"

// @todo why does this need an isFinalBoss parameter? isn't it if creatureIndex == 25? check

void CreatureDefinition::init(int creatureIndex, bool isFinalBoss, const ExeData &exeData)
{
	const auto &entities = exeData.entities;

	const std::string &nameStr = isFinalBoss ? entities.finalBossName : entities.creatureNames[creatureIndex];
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", nameStr.c_str());

	this->level = entities.creatureLevels[creatureIndex];
	this->minHP = entities.creatureHitPoints[creatureIndex].first;
	this->maxHP = entities.creatureHitPoints[creatureIndex].second;
	this->baseExp = entities.creatureBaseExps[creatureIndex];
	this->expMultiplier = entities.creatureExpMultipliers[creatureIndex];
	this->soundIndex = entities.creatureSounds[creatureIndex];

	std::snprintf(std::begin(this->soundName), std::size(this->soundName), "%s", entities.creatureSoundNames[this->soundIndex].c_str());

	this->minDamage = entities.creatureDamages[creatureIndex].first;
	this->maxDamage = entities.creatureDamages[creatureIndex].second;
	this->magicEffects = entities.creatureMagicEffects[creatureIndex];
	this->scale = entities.creatureScales[creatureIndex];
	this->yOffset = entities.creatureYOffsets[creatureIndex];
	this->hasNoCorpse = entities.creatureHasNoCorpse[creatureIndex] != 0;
	this->bloodIndex = entities.creatureBlood[creatureIndex];
	this->diseaseChances = entities.creatureDiseaseChances[creatureIndex];

	const auto &srcAttributes = entities.creatureAttributes[creatureIndex];
	std::copy(std::begin(srcAttributes), std::end(srcAttributes), std::begin(this->attributes));

	if (isFinalBoss)
	{
		this->lootChances = 0; // @todo Figure out how final boss is handled
	}
	else
	{
		const int lootChancesIndex = creatureIndex + 1;
		DebugAssertIndex(entities.creatureLootChances, lootChancesIndex);
		this->lootChances = entities.creatureLootChances[lootChancesIndex];
	}

	this->ghost = ArenaAnimUtils::isGhost(creatureIndex);
}

bool CreatureDefinition::operator==(const CreatureDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (std::strncmp(std::begin(this->name), std::begin(other.name), std::size(this->name)) != 0)
	{
		return false;
	}

	if (this->level != other.level)
	{
		return false;
	}

	if (this->minHP != other.minHP)
	{
		return false;
	}

	if (this->maxHP != other.maxHP)
	{
		return false;
	}

	if (this->baseExp != other.baseExp)
	{
		return false;
	}

	if (this->expMultiplier != other.expMultiplier)
	{
		return false;
	}

	if (this->soundIndex != other.soundIndex)
	{
		return false;
	}

	if (std::strncmp(std::begin(this->soundName), std::begin(other.soundName), std::size(this->soundName)) != 0)
	{
		return false;
	}

	if (this->minDamage != other.minDamage)
	{
		return false;
	}

	if (this->maxDamage != other.maxDamage)
	{
		return false;
	}

	if (this->magicEffects != other.magicEffects)
	{
		return false;
	}

	if (this->scale != other.scale)
	{
		return false;
	}

	if (this->yOffset != other.yOffset)
	{
		return false;
	}

	if (this->hasNoCorpse != other.hasNoCorpse)
	{
		return false;
	}

	if (this->bloodIndex != other.bloodIndex)
	{
		return false;
	}

	if (this->diseaseChances != other.diseaseChances)
	{
		return false;
	}

	if (std::memcmp(std::begin(this->attributes), std::begin(other.attributes), std::size(this->attributes) * sizeof(this->attributes[0])) != 0)
	{
		return false;
	}

	if (this->ghost != other.ghost)
	{
		return false;
	}

	return true;
}

void CreatureDefinitionLibrary::init(const ExeData &exeData)
{
	for (int creatureIndex = 0; creatureIndex < ArenaAnimUtils::CreatureCount; creatureIndex++)
	{
		const bool isFinalBoss = creatureIndex == (ArenaAnimUtils::FinalBossCreatureID - 1);

		CreatureDefinition creatureDef;
		creatureDef.init(creatureIndex, isFinalBoss, exeData);
		this->entries.emplace_back(std::move(creatureDef));
	}
}

int CreatureDefinitionLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->entries.size());
}

const CreatureDefinition &CreatureDefinitionLibrary::getDefinition(CreatureDefinitionID id) const
{
	DebugAssertIndex(this->entries, id);
	return this->entries[id];
}
