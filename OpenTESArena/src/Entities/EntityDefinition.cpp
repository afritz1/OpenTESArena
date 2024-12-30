#include <algorithm>
#include <cstdio>
#include <cstring>

#include "EntityDefinition.h"
#include "../Assets/ArenaAnimUtils.h"

void EntityDefinition::EnemyDefinition::CreatureDefinition::init(int creatureIndex,
	bool isFinalBoss, const ExeData &exeData)
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
	std::copy(srcAttributes.begin(), srcAttributes.end(), std::begin(this->attributes));

	this->ghost = ArenaAnimUtils::isGhost(creatureIndex);
}

bool EntityDefinition::EnemyDefinition::CreatureDefinition::operator==(const CreatureDefinition &other) const
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

void EntityDefinition::EnemyDefinition::HumanDefinition::init(bool male, int charClassID)
{
	this->male = male;
	this->charClassID = charClassID;
}

bool EntityDefinition::EnemyDefinition::HumanDefinition::operator==(const HumanDefinition &other) const
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

EntityDefinition::EnemyDefinition::EnemyDefinition()
{
	this->type = static_cast<EnemyDefinition::Type>(-1);
}

void EntityDefinition::EnemyDefinition::initCreature(int creatureIndex, bool isFinalBoss,
	const ExeData &exeData)
{
	this->type = EnemyDefinition::Type::Creature;
	this->creature.init(creatureIndex, isFinalBoss, exeData);
}

void EntityDefinition::EnemyDefinition::initHuman(bool male, int charClassID)
{
	this->type = EnemyDefinition::Type::Human;
	this->human.init(male, charClassID);
}

bool EntityDefinition::EnemyDefinition::operator==(const EnemyDefinition &other) const
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
	case EnemyDefinition::Type::Creature:
		return this->creature == other.creature;
	case EnemyDefinition::Type::Human:
		return this->human == other.human;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}

	return true;
}

EntityDefinition::EnemyDefinition::Type EntityDefinition::EnemyDefinition::getType() const
{
	return this->type;
}

const EntityDefinition::EnemyDefinition::CreatureDefinition &EntityDefinition::EnemyDefinition::getCreature() const
{
	DebugAssert(this->type == EnemyDefinition::Type::Creature);
	return this->creature;
}

const EntityDefinition::EnemyDefinition::HumanDefinition &EntityDefinition::EnemyDefinition::getHuman() const
{
	DebugAssert(this->type == EnemyDefinition::Type::Human);
	return this->human;
}

EntityDefinition::CitizenDefinition::CitizenDefinition()
{
	this->male = false;
	this->climateType = static_cast<ArenaTypes::ClimateType>(-1);
}

void EntityDefinition::CitizenDefinition::init(bool male, ArenaTypes::ClimateType climateType)
{
	this->male = male;
	this->climateType = climateType;
}

bool EntityDefinition::CitizenDefinition::operator==(const CitizenDefinition &other) const
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

void EntityDefinition::StaticNpcDefinition::ShopkeeperDefinition::init(ShopkeeperDefinition::Type type)
{
	this->type = type;
}

bool EntityDefinition::StaticNpcDefinition::ShopkeeperDefinition::operator==(const ShopkeeperDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->type == other.type;
}

bool EntityDefinition::StaticNpcDefinition::PersonDefinition::operator==(const PersonDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return true;
}

EntityDefinition::StaticNpcDefinition::StaticNpcDefinition()
{
	this->type = static_cast<StaticNpcDefinition::Type>(-1);
}

void EntityDefinition::StaticNpcDefinition::initShopkeeper(ShopkeeperDefinition::Type type)
{
	this->type = StaticNpcDefinition::Type::Shopkeeper;
	this->shopkeeper.init(type);
}

void EntityDefinition::StaticNpcDefinition::initPerson()
{
	this->type = StaticNpcDefinition::Type::Person;
}

bool EntityDefinition::StaticNpcDefinition::operator==(const StaticNpcDefinition &other) const
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
	case StaticNpcDefinition::Type::Shopkeeper:
		return this->shopkeeper == other.shopkeeper;
	case StaticNpcDefinition::Type::Person:
		return this->person == other.person;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

EntityDefinition::StaticNpcDefinition::StaticNpcDefinition::Type EntityDefinition::StaticNpcDefinition::getType() const
{
	return this->type;
}

const EntityDefinition::StaticNpcDefinition::ShopkeeperDefinition &EntityDefinition::StaticNpcDefinition::getShopkeeper() const
{
	DebugAssert(this->type == StaticNpcDefinition::Type::Shopkeeper);
	return this->shopkeeper;
}

const EntityDefinition::StaticNpcDefinition::PersonDefinition &EntityDefinition::StaticNpcDefinition::getPerson() const
{
	DebugAssert(this->type == StaticNpcDefinition::Type::Person);
	return this->person;
}

EntityDefinition::ItemDefinition::ItemDefinition()
{
	this->type = static_cast<ItemDefinition::Type>(-1);
}

void EntityDefinition::ItemDefinition::initKey()
{
	this->type = ItemDefinition::Type::Key;
}

void EntityDefinition::ItemDefinition::initQuestItem()
{
	this->type = ItemDefinition::Type::QuestItem;
}

bool EntityDefinition::ItemDefinition::operator==(const ItemDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->type == other.type;
}

EntityDefinition::ItemDefinition::ItemDefinition::Type EntityDefinition::ItemDefinition::getType() const
{
	return this->type;
}

const EntityDefinition::ItemDefinition::KeyDefinition &EntityDefinition::ItemDefinition::getKey() const
{
	DebugAssert(this->type == ItemDefinition::Type::Key);
	return this->key;
}

const EntityDefinition::ItemDefinition::QuestItemDefinition &EntityDefinition::ItemDefinition::getQuestItem() const
{
	DebugAssert(this->type == ItemDefinition::Type::QuestItem);
	return this->questItem;
}

EntityDefinition::ContainerDefinition::HolderDefinition::HolderDefinition()
{
	this->locked = false;
}

void EntityDefinition::ContainerDefinition::HolderDefinition::init(bool locked)
{
	this->locked = locked;
}

bool EntityDefinition::ContainerDefinition::HolderDefinition::operator==(const HolderDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->locked == other.locked;
}

bool EntityDefinition::ContainerDefinition::PileDefinition::operator==(const PileDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return true;
}

EntityDefinition::ContainerDefinition::ContainerDefinition()
{
	this->type = static_cast<ContainerDefinition::Type>(-1);
}

void EntityDefinition::ContainerDefinition::initHolder(bool locked)
{
	this->type = ContainerDefinition::Type::Holder;
	this->holder.init(locked);
}

void EntityDefinition::ContainerDefinition::initPile()
{
	this->type = ContainerDefinition::Type::Pile;
}

bool EntityDefinition::ContainerDefinition::operator==(const ContainerDefinition &other) const
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
	case ContainerDefinition::Type::Holder:
		return this->holder == other.holder;
	case ContainerDefinition::Type::Pile:
		return this->pile == other.pile;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

EntityDefinition::ContainerDefinition::ContainerDefinition::Type EntityDefinition::ContainerDefinition::getType() const
{
	return this->type;
}

const EntityDefinition::ContainerDefinition::HolderDefinition &EntityDefinition::ContainerDefinition::getHolder() const
{
	DebugAssert(this->type == ContainerDefinition::Type::Holder);
	return this->holder;
}

const EntityDefinition::ContainerDefinition::PileDefinition &EntityDefinition::ContainerDefinition::getPile() const
{
	DebugAssert(this->type == ContainerDefinition::Type::Pile);
	return this->pile;
}

EntityDefinition::ProjectileDefinition::ProjectileDefinition()
{
	this->hasGravity = false;
}

void EntityDefinition::ProjectileDefinition::init(bool hasGravity)
{
	this->hasGravity = hasGravity;
}

bool EntityDefinition::ProjectileDefinition::operator==(const ProjectileDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->hasGravity == other.hasGravity;
}

EntityDefinition::TransitionDefinition::TransitionDefinition()
{
	this->transitionDefID = -1;
}

void EntityDefinition::TransitionDefinition::init(LevelDefinition::TransitionDefID transitionDefID)
{
	this->transitionDefID = transitionDefID;
}

bool EntityDefinition::TransitionDefinition::operator==(const TransitionDefinition &other) const
{
	if (this == &other)
	{
		return true;
	}

	return this->transitionDefID == other.transitionDefID;
}

EntityDefinition::DoodadDefinition::DoodadDefinition()
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

void EntityDefinition::DoodadDefinition::init(int yOffset, double scale, bool collider,
	bool transparent, bool ceiling, bool streetlight, bool puddle, int lightIntensity)
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

bool EntityDefinition::DoodadDefinition::operator==(const DoodadDefinition &other) const
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
	case EntityDefinitionType::Doodad:
		return this->doodad == other.doodad;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

EntityDefinitionType EntityDefinition::getType() const
{
	return this->type;
}

const EntityAnimationDefinition &EntityDefinition::getAnimDef() const
{
	return this->animDef;
}

const EntityDefinition::EnemyDefinition &EntityDefinition::getEnemy() const
{
	DebugAssert(this->type == EntityDefinitionType::Enemy);
	return this->enemy;
}

const EntityDefinition::CitizenDefinition &EntityDefinition::getCitizen() const
{
	DebugAssert(this->type == EntityDefinitionType::Citizen);
	return this->citizen;
}

const EntityDefinition::StaticNpcDefinition &EntityDefinition::getStaticNpc() const
{
	DebugAssert(this->type == EntityDefinitionType::StaticNPC);
	return this->staticNpc;
}

const EntityDefinition::ItemDefinition &EntityDefinition::getItem() const
{
	DebugAssert(this->type == EntityDefinitionType::Item);
	return this->item;
}

const EntityDefinition::ContainerDefinition &EntityDefinition::getContainer() const
{
	DebugAssert(this->type == EntityDefinitionType::Container);
	return this->container;
}

const EntityDefinition::ProjectileDefinition &EntityDefinition::getProjectile() const
{
	DebugAssert(this->type == EntityDefinitionType::Projectile);
	return this->projectile;
}

const EntityDefinition::TransitionDefinition &EntityDefinition::getTransition() const
{
	DebugAssert(this->type == EntityDefinitionType::Transition);
	return this->transition;
}

const EntityDefinition::DoodadDefinition &EntityDefinition::getDoodad() const
{
	DebugAssert(this->type == EntityDefinitionType::Doodad);
	return this->doodad;
}

void EntityDefinition::initEnemyCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData,
	EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Enemy, std::move(animDef));
	this->enemy.initCreature(creatureIndex, isFinalBoss, exeData);
}

void EntityDefinition::initEnemyHuman(bool male, int charClassID, EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Enemy, std::move(animDef));
	this->enemy.initHuman(male, charClassID);
}

void EntityDefinition::initCitizen(bool male, ArenaTypes::ClimateType climateType,
	EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Citizen, std::move(animDef));
	this->citizen.init(male, climateType);
}

void EntityDefinition::initStaticNpcShopkeeper(StaticNpcDefinition::ShopkeeperDefinition::Type type,
	EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::StaticNPC, std::move(animDef));
	this->staticNpc.initShopkeeper(type);
}

void EntityDefinition::initStaticNpcPerson(EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::StaticNPC, std::move(animDef));
	this->staticNpc.initPerson();
}

void EntityDefinition::initItemKey(EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Item, std::move(animDef));
	this->item.initKey();
}

void EntityDefinition::initItemQuestItem(EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Item, std::move(animDef));
	this->item.initQuestItem();
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

void EntityDefinition::initTransition(LevelDefinition::TransitionDefID defID,
	EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Transition, std::move(animDef));
	this->transition.init(defID);
}

void EntityDefinition::initDoodad(int yOffset, double scale, bool collider, bool transparent,
	bool ceiling, bool streetlight, bool puddle, int lightIntensity,
	EntityAnimationDefinition &&animDef)
{
	this->init(EntityDefinitionType::Doodad, std::move(animDef));
	this->doodad.init(yOffset, scale, collider, transparent, ceiling, streetlight, puddle, lightIntensity);
}
