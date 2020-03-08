#include <algorithm>
#include <cstdio>

#include "EntityDefinition.h"

EntityDefinition::CreatureData::CreatureData()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->level = 0;
	this->minHP = 0;
	this->maxHP = 0;
	this->baseExp = 0;
	this->expMultiplier = 0;
	this->soundIndex = 0;
	std::fill(std::begin(this->soundName), std::end(this->soundName), '\0');
	this->minDamage = 0;
	this->maxDamage = 0;
	this->magicEffects = 0;
	this->scale = 0;
	this->yOffset = 0;
	this->hasNoCorpse = false;
	this->bloodIndex = 0;
	this->diseaseChances = 0;
	std::fill(std::begin(this->attributes), std::end(this->attributes), 0);
}

void EntityDefinition::CreatureData::init(const ExeData::Entities &entities,
	int creatureIndex, bool isFinalBoss)
{
	if (!isFinalBoss)
	{
		std::snprintf(std::begin(this->name), std::size(this->name),
			"%s", entities.creatureNames[creatureIndex].c_str());
	}
	else
	{
		std::snprintf(std::begin(this->name), std::size(this->name), "%s", "TODO");
	}

	this->level = entities.creatureLevels[creatureIndex];
	this->minHP = entities.creatureHitPoints[creatureIndex].first;
	this->maxHP = entities.creatureHitPoints[creatureIndex].second;
	this->baseExp = entities.creatureBaseExps[creatureIndex];
	this->expMultiplier = entities.creatureExpMultipliers[creatureIndex];
	this->soundIndex = entities.creatureSounds[creatureIndex];

	std::snprintf(std::begin(this->soundName), std::size(this->soundName),
		"%s", entities.creatureSoundNames[creatureIndex].c_str());

	this->minDamage = entities.creatureDamages[creatureIndex].first;
	this->maxDamage = entities.creatureDamages[creatureIndex].second;
	this->magicEffects = entities.creatureMagicEffects[creatureIndex];
	this->scale = entities.creatureScales[creatureIndex];
	this->yOffset = entities.creatureYOffsets[creatureIndex];
	this->hasNoCorpse = entities.creatureHasNoCorpse[creatureIndex] != 0;
	this->bloodIndex = entities.creatureBlood[creatureIndex];
	this->diseaseChances = entities.creatureDiseaseChances[creatureIndex];

	const auto &srcAttributes = entities.creatureAttributes[creatureIndex];
	std::copy(srcAttributes.begin(), srcAttributes.end(), std::begin(this->attributes));
}

EntityDefinition::InfData::InfData()
{
	this->flatIndex = -1;
	this->yOffset = 0;
	this->collider = false;
	this->puddle = false;
	this->largeScale = false;
	this->dark = false;
	this->transparent = false;
	this->ceiling = false;
	this->mediumScale = false;
	this->streetLight = false;
	this->lightIntensity = std::nullopt;
}

void EntityDefinition::InfData::init(int flatIndex, int yOffset, bool collider, bool puddle,
	bool largeScale, bool dark, bool transparent, bool ceiling, bool mediumScale, bool streetLight,
	const std::optional<int> &lightIntensity)
{
	this->flatIndex = flatIndex;
	this->yOffset = yOffset;
	this->collider = collider;
	this->puddle = puddle;
	this->largeScale = largeScale;
	this->dark = dark;
	this->transparent = transparent;
	this->ceiling = ceiling;
	this->mediumScale = mediumScale;
	this->streetLight = streetLight;
	this->lightIntensity = lightIntensity;
}

EntityDefinition::EntityDefinition()
{
	this->isCreatureInited = false;
	this->isHumanEnemyInited = false;
	this->isOtherInited = false;
}

void EntityDefinition::initCreature(const ExeData::Entities &entities, int creatureIndex,
	bool isFinalBoss, int flatIndex)
{
	this->creatureData.init(entities, creatureIndex, isFinalBoss);
	this->infData.flatIndex = flatIndex;
	this->infData.collider = true;

	this->isCreatureInited = true;
}

void EntityDefinition::initHumanEnemy(const char *name, int flatIndex, int yOffset, bool collider,
	bool largeScale, bool dark, bool transparent, bool ceiling, bool mediumScale,
	const std::optional<int> &lightIntensity)
{
	const bool puddle = false;
	const bool streetlight = false;
	this->infData.init(flatIndex, yOffset, collider, puddle, largeScale, dark, transparent,
		ceiling, mediumScale, streetlight, lightIntensity);

	std::snprintf(std::begin(this->creatureData.name), std::size(this->creatureData.name), "%s", name);

	this->isHumanEnemyInited = true;
}

void EntityDefinition::initOther(int flatIndex, int yOffset, bool collider, bool puddle,
	bool largeScale, bool dark, bool transparent, bool ceiling, bool mediumScale, bool streetLight,
	const std::optional<int> &lightIntensity)
{
	this->infData.init(flatIndex, yOffset, collider, puddle, largeScale, dark, transparent,
		ceiling, mediumScale, streetLight, lightIntensity);

	std::fill(std::begin(this->creatureData.name), std::end(this->creatureData.name), '\0');

	this->isOtherInited = true;
}

std::string_view EntityDefinition::getDisplayName() const
{
	// @todo: branch on the entity def type (creature, human enemy, etc.).
	return std::string_view(std::begin(this->creatureData.name), std::strlen(this->creatureData.name));
}

bool EntityDefinition::isCreature() const
{
	return isCreatureInited;
}

bool EntityDefinition::isHumanEnemy() const
{
	return isHumanEnemyInited;
}

bool EntityDefinition::isOther() const
{
	return isOtherInited;
}

EntityAnimationData &EntityDefinition::getAnimationData()
{
	return this->animationData;
}

const EntityAnimationData &EntityDefinition::getAnimationData() const
{
	return this->animationData;
}

EntityDefinition::CreatureData &EntityDefinition::getCreatureData()
{
	return this->creatureData;
}

const EntityDefinition::CreatureData &EntityDefinition::getCreatureData() const
{
	return this->creatureData;
}

EntityDefinition::InfData &EntityDefinition::getInfData()
{
	return this->infData;
}

const EntityDefinition::InfData &EntityDefinition::getInfData() const
{
	return this->infData;
}
