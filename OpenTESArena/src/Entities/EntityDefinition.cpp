#include <algorithm>
#include <cstdio>
#include <cstring>

#include "ArenaAnimUtils.h"
#include "EntityDefinition.h"

void EnemyEntityDefinition::CreatureDefinition::init(int creatureIndex, bool isFinalBoss, const ExeData &exeData)
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

bool EnemyEntityDefinition::CreatureDefinition::operator==(const CreatureDefinition &other) const
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

void EnemyEntityDefinition::HumanDefinition::init(bool male, int charClassID)
{
	this->male = male;
	this->charClassID = charClassID;
}

bool EnemyEntityDefinition::HumanDefinition::operator==(const HumanDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->male != other.male)
	{
		return false;
	}

	if (this->charClassID != other.charClassID)
	{
		return false;
	}

	return true;
}

EnemyEntityDefinition::EnemyEntityDefinition()
{
	this->type = static_cast<EnemyEntityDefinitionType>(-1);
}

void EnemyEntityDefinition::initCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData)
{
	this->type = EnemyEntityDefinitionType::Creature;
	this->creature.init(creatureIndex, isFinalBoss, exeData);
}

void EnemyEntityDefinition::initHuman(bool male, int charClassID)
{
	this->type = EnemyEntityDefinitionType::Human;
	this->human.init(male, charClassID);
}

bool EnemyEntityDefinition::operator==(const EnemyEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->type != other.type)
	{
		return false;
	}

	switch (this->type)
	{
	case EnemyEntityDefinitionType::Creature:
		return this->creature == other.creature;
	case EnemyEntityDefinitionType::Human:
		return this->human == other.human;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}

	return true;
}

CitizenEntityDefinition::CitizenEntityDefinition()
{
	this->male = false;
	this->climateType = static_cast<ArenaClimateType>(-1);
}

void CitizenEntityDefinition::init(bool male, ArenaClimateType climateType)
{
	this->male = male;
	this->climateType = climateType;
}

bool CitizenEntityDefinition::operator==(const CitizenEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->male != other.male)
	{
		return false;
	}

	if (this->climateType != other.climateType)
	{
		return false;
	}

	return true;
}

StaticNpcEntityDefinition::StaticNpcEntityDefinition()
{
	this->personalityType = static_cast<StaticNpcPersonalityType>(-1);
}

void StaticNpcEntityDefinition::init(StaticNpcPersonalityType personalityType)
{
	this->personalityType = personalityType;
}

bool StaticNpcEntityDefinition::operator==(const StaticNpcEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->personalityType != other.personalityType)
	{
		return false;
	}

	return true;
}

bool ItemEntityDefinition::QuestItemDefinition::operator==(const QuestItemDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->yOffset == other.yOffset;
}

ItemEntityDefinition::ItemEntityDefinition()
{
	this->type = static_cast<ItemEntityDefinitionType>(-1);
}

void ItemEntityDefinition::initKey()
{
	this->type = ItemEntityDefinitionType::Key;
}

void ItemEntityDefinition::initQuestItem(int yOffset)
{
	this->type = ItemEntityDefinitionType::QuestItem;
	this->questItem.yOffset = yOffset;
}

bool ItemEntityDefinition::operator==(const ItemEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->type != other.type)
	{
		return false;
	}

	switch (this->type)
	{
	case ItemEntityDefinitionType::Key:
		return true;
	case ItemEntityDefinitionType::QuestItem:
		return this->questItem == other.questItem;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

ContainerEntityDefinition::HolderDefinition::HolderDefinition()
{
	this->locked = false;
}

void ContainerEntityDefinition::HolderDefinition::init(bool locked)
{
	this->locked = locked;
}

bool ContainerEntityDefinition::HolderDefinition::operator==(const HolderDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->locked == other.locked;
}

bool ContainerEntityDefinition::PileDefinition::operator==(const PileDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return true;
}

ContainerEntityDefinition::ContainerEntityDefinition()
{
	this->type = static_cast<ContainerEntityDefinitionType>(-1);
}

void ContainerEntityDefinition::initHolder(bool locked)
{
	this->type = ContainerEntityDefinitionType::Holder;
	this->holder.init(locked);
}

void ContainerEntityDefinition::initPile()
{
	this->type = ContainerEntityDefinitionType::Pile;
}

bool ContainerEntityDefinition::operator==(const ContainerEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->type != other.type)
	{
		return false;
	}

	switch (this->type)
	{
	case ContainerEntityDefinitionType::Holder:
		return this->holder == other.holder;
	case ContainerEntityDefinitionType::Pile:
		return this->pile == other.pile;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

ProjectileEntityDefinition::ProjectileEntityDefinition()
{
	this->hasGravity = false;
}

void ProjectileEntityDefinition::init(bool hasGravity)
{
	this->hasGravity = hasGravity;
}

bool ProjectileEntityDefinition::operator==(const ProjectileEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->hasGravity == other.hasGravity;
}

VfxEntityDefinition::VfxEntityDefinition()
{
	this->type = static_cast<VfxEntityAnimationType>(-1);
	this->index = -1;
}

void VfxEntityDefinition::init(VfxEntityAnimationType type, int index)
{
	this->type = type;
	this->index = index;
}

bool VfxEntityDefinition::operator==(const VfxEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return (this->type == other.type) && (this->index == other.index);
}

TransitionEntityDefinition::TransitionEntityDefinition()
{
	this->transitionDefID = -1;
}

void TransitionEntityDefinition::init(LevelVoxelTransitionDefID transitionDefID)
{
	this->transitionDefID = transitionDefID;
}

bool TransitionEntityDefinition::operator==(const TransitionEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->transitionDefID == other.transitionDefID;
}

DecorationEntityDefinition::DecorationEntityDefinition()
{
	this->yOffset = 0;
	this->scale = 0.0;
	this->collider = false;
	this->transparent = false;
	this->ceiling = false;
	this->streetlight = false;
	this->puddle = false;
	this->lightIntensity = 0;
}

void DecorationEntityDefinition::init(int yOffset, double scale, bool collider, bool transparent, bool ceiling, bool streetlight, bool puddle, int lightIntensity)
{
	this->yOffset = yOffset;
	this->scale = scale;
	this->collider = collider;
	this->transparent = transparent;
	this->ceiling = ceiling;
	this->streetlight = streetlight;
	this->puddle = puddle;
	this->lightIntensity = lightIntensity;
}

bool DecorationEntityDefinition::operator==(const DecorationEntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->yOffset != other.yOffset)
	{
		return false;
	}

	if (this->scale != other.scale)
	{
		return false;
	}
	
	if (this->collider != other.collider)
	{
		return false;
	}

	if (this->transparent != other.transparent)
	{
		return false;
	}

	if (this->ceiling != other.ceiling)
	{
		return false;
	}

	if (this->streetlight != other.streetlight)
	{
		return false;
	}
	
	if (this->puddle != other.puddle)
	{
		return false;
	}

	if (this->lightIntensity != other.lightIntensity)
	{
		return false;
	}

	return true;
}

void EntityDefinition::init(EntityDefinitionType type, EntityAnimationDefinition &&animDef)
{
	this->type = type;
	this->animDef = std::move(animDef);
}

EntityDefinition::EntityDefinition()
{
	this->type = static_cast<EntityDefinitionType>(-1);
}

bool EntityDefinition::operator==(const EntityDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->type != other.type)
	{
		return false;
	}

	if (this->animDef != other.animDef)
	{
		return false;
	}

	switch (this->type)
	{
	case EntityDefinitionType::Enemy:
		return this->enemy == other.enemy;
	case EntityDefinitionType::Citizen:
		return this->citizen == other.citizen;
	case EntityDefinitionType::StaticNPC:
		return this->staticNpc == other.staticNpc;
	case EntityDefinitionType::Item:
		return this->item == other.item;
	case EntityDefinitionType::Container:
		return this->container == other.container;
	case EntityDefinitionType::Projectile:
		return this->projectile == other.projectile;
	case EntityDefinitionType::Transition:
		return this->transition == other.transition;
	case EntityDefinitionType::Decoration:
		return this->decoration == other.decoration;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

void EntityDefinition::initEnemyCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Enemy, std::move(animDef));
	this->enemy.initCreature(creatureIndex, isFinalBoss, exeData);
}

void EntityDefinition::initEnemyHuman(bool male, int charClassID, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Enemy, std::move(animDef));
	this->enemy.initHuman(male, charClassID);
}

void EntityDefinition::initCitizen(bool male, ArenaClimateType climateType,
	EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Citizen, std::move(animDef));
	this->citizen.init(male, climateType);
}

void EntityDefinition::initStaticNpc(StaticNpcPersonalityType personalityType, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::StaticNPC, std::move(animDef));
	this->staticNpc.init(personalityType);
}

void EntityDefinition::initItemKey(EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Item, std::move(animDef));
	this->item.initKey();
}

void EntityDefinition::initItemQuestItem(int yOffset, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Item, std::move(animDef));
	this->item.initQuestItem(yOffset);
}

void EntityDefinition::initContainerHolder(bool locked, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Container, std::move(animDef));
	this->container.initHolder(locked);
}

void EntityDefinition::initContainerPile(EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Container, std::move(animDef));
	this->container.initPile();
}

void EntityDefinition::initProjectile(bool hasGravity, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Projectile, std::move(animDef));
	this->projectile.init(hasGravity);
}

void EntityDefinition::initVfx(VfxEntityAnimationType type, int index, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Vfx, std::move(animDef));
	this->vfx.init(type, index);
}

void EntityDefinition::initTransition(LevelVoxelTransitionDefID defID,
	EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Transition, std::move(animDef));
	this->transition.init(defID);
}

void EntityDefinition::initDecoration(int yOffset, double scale, bool collider, bool transparent, bool ceiling, bool streetlight,
	bool puddle, int lightIntensity, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Decoration, std::move(animDef));
	this->decoration.init(yOffset, scale, collider, transparent, ceiling, streetlight, puddle, lightIntensity);
}
