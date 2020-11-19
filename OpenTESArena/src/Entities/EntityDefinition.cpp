#include <algorithm>
#include <cstdio>
#include <cstring>

#include "EntityDefinition.h"
#include "../World/ClimateType.h"

void EntityDefinition::EnemyDefinition::CreatureDefinition::init(int creatureIndex,
	bool isFinalBoss, const ExeData &exeData)
{
	const auto &entities = exeData.entities;

	const std::string &nameStr = isFinalBoss ? entities.finalBossName :
		entities.creatureNames[creatureIndex];
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", nameStr.c_str());

	this->level = entities.creatureLevels[creatureIndex];
	this->minHP = entities.creatureHitPoints[creatureIndex].first;
	this->maxHP = entities.creatureHitPoints[creatureIndex].second;
	this->baseExp = entities.creatureBaseExps[creatureIndex];
	this->expMultiplier = entities.creatureExpMultipliers[creatureIndex];
	this->soundIndex = entities.creatureSounds[creatureIndex];

	std::snprintf(std::begin(this->soundName), std::size(this->soundName),
		"%s", entities.creatureSoundNames[this->soundIndex].c_str());

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

void EntityDefinition::EnemyDefinition::HumanDefinition::init(bool male, int charClassID)
{
	this->male = male;
	this->charClassID = charClassID;
}

EntityDefinition::EnemyDefinition::EnemyDefinition()
{
	this->type = static_cast<EnemyDefinition::Type>(-1);
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

EntityDefinition::CitizenDefinition::CitizenDefinition()
{
	this->male = false;
	this->climateType = static_cast<ClimateType>(-1);
}

void EntityDefinition::CitizenDefinition::init(bool male, ClimateType climateType)
{
	this->male = male;
	this->climateType = climateType;
}

void EntityDefinition::StaticNpcDefinition::ShopkeeperDefinition::init(ShopkeeperDefinition::Type type)
{
	this->type = type;
}

EntityDefinition::StaticNpcDefinition::StaticNpcDefinition()
{
	this->type = static_cast<StaticNpcDefinition::Type>(-1);
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

void EntityDefinition::StaticNpcDefinition::initShopkeeper(ShopkeeperDefinition::Type type)
{
	this->type = StaticNpcDefinition::Type::Shopkeeper;
	this->shopkeeper.init(type);
}

void EntityDefinition::StaticNpcDefinition::initPerson()
{
	this->type = StaticNpcDefinition::Type::Person;
}

EntityDefinition::ItemDefinition::ItemDefinition()
{
	this->type = static_cast<ItemDefinition::Type>(-1);
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

void EntityDefinition::ItemDefinition::initKey()
{
	this->type = ItemDefinition::Type::Key;
}

void EntityDefinition::ItemDefinition::initQuestItem()
{
	this->type = ItemDefinition::Type::QuestItem;
}

EntityDefinition::ContainerDefinition::HolderDefinition::HolderDefinition()
{
	this->locked = false;
}

void EntityDefinition::ContainerDefinition::HolderDefinition::init(bool locked)
{
	this->locked = locked;
}

EntityDefinition::ContainerDefinition::ContainerDefinition()
{
	this->type = static_cast<ContainerDefinition::Type>(-1);
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

void EntityDefinition::ContainerDefinition::initHolder(bool locked)
{
	this->type = ContainerDefinition::Type::Holder;
	this->holder.init(locked);
}

void EntityDefinition::ContainerDefinition::initPile()
{
	this->type = ContainerDefinition::Type::Pile;
}

EntityDefinition::ProjectileDefinition::ProjectileDefinition()
{
	this->hasGravity = false;
}

void EntityDefinition::ProjectileDefinition::init(bool hasGravity)
{
	this->hasGravity = hasGravity;
}

EntityDefinition::TransitionDefinition::TransitionDefinition()
{
	this->transitionDefID = -1;
}

void EntityDefinition::TransitionDefinition::init(LevelDefinition::TransitionDefID transitionDefID)
{
	this->transitionDefID = transitionDefID;
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

void EntityDefinition::init(Type type, EntityAnimationDefinition &&animDef)
{
	this->type = type;
	this->animDef = std::move(animDef);
}

EntityDefinition::EntityDefinition()
{
	this->type = static_cast<Type>(-1);
}

EntityDefinition::Type EntityDefinition::getType() const
{
	return this->type;
}

const EntityAnimationDefinition &EntityDefinition::getAnimDef() const
{
	return this->animDef;
}

const EntityDefinition::EnemyDefinition &EntityDefinition::getEnemy() const
{
	DebugAssert(this->type == Type::Enemy);
	return this->enemy;
}

const EntityDefinition::CitizenDefinition &EntityDefinition::getCitizen() const
{
	DebugAssert(this->type == Type::Citizen);
	return this->citizen;
}

const EntityDefinition::StaticNpcDefinition &EntityDefinition::getStaticNpc() const
{
	DebugAssert(this->type == Type::StaticNPC);
	return this->staticNpc;
}

const EntityDefinition::ItemDefinition &EntityDefinition::getItem() const
{
	DebugAssert(this->type == Type::Item);
	return this->item;
}

const EntityDefinition::ContainerDefinition &EntityDefinition::getContainer() const
{
	DebugAssert(this->type == Type::Container);
	return this->container;
}

const EntityDefinition::ProjectileDefinition &EntityDefinition::getProjectile() const
{
	DebugAssert(this->type == Type::Projectile);
	return this->projectile;
}

const EntityDefinition::TransitionDefinition &EntityDefinition::getTransition() const
{
	DebugAssert(this->type == Type::Transition);
	return this->transition;
}

const EntityDefinition::DoodadDefinition &EntityDefinition::getDoodad() const
{
	DebugAssert(this->type == Type::Doodad);
	return this->doodad;
}

void EntityDefinition::initEnemyCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData,
	EntityAnimationDefinition &&animDef)
{
	this->init(Type::Enemy, std::move(animDef));
	this->enemy.initCreature(creatureIndex, isFinalBoss, exeData);
}

void EntityDefinition::initEnemyHuman(bool male, int charClassID, EntityAnimationDefinition &&animDef)
{
	this->init(Type::Enemy, std::move(animDef));
	this->enemy.initHuman(male, charClassID);
}

void EntityDefinition::initCitizen(bool male, ClimateType climateType,
	EntityAnimationDefinition &&animDef)
{
	this->init(Type::Citizen, std::move(animDef));
	this->citizen.init(male, climateType);
}

void EntityDefinition::initStaticNpcShopkeeper(StaticNpcDefinition::ShopkeeperDefinition::Type type,
	EntityAnimationDefinition &&animDef)
{
	this->init(Type::StaticNPC, std::move(animDef));
	this->staticNpc.initShopkeeper(type);
}

void EntityDefinition::initStaticNpcPerson(EntityAnimationDefinition &&animDef)
{
	this->init(Type::StaticNPC, std::move(animDef));
	this->staticNpc.initPerson();
}

void EntityDefinition::initItemKey(EntityAnimationDefinition &&animDef)
{
	this->init(Type::Item, std::move(animDef));
	this->item.initKey();
}

void EntityDefinition::initItemQuestItem(EntityAnimationDefinition &&animDef)
{
	this->init(Type::Item, std::move(animDef));
	this->item.initQuestItem();
}

void EntityDefinition::initContainerHolder(bool locked, EntityAnimationDefinition &&animDef)
{
	this->init(Type::Container, std::move(animDef));
	this->container.initHolder(locked);
}

void EntityDefinition::initContainerPile(EntityAnimationDefinition &&animDef)
{
	this->init(Type::Container, std::move(animDef));
	this->container.initPile();
}

void EntityDefinition::initProjectile(bool hasGravity, EntityAnimationDefinition &&animDef)
{
	this->init(Type::Projectile, std::move(animDef));
	this->projectile.init(hasGravity);
}

void EntityDefinition::initTransition(LevelDefinition::TransitionDefID defID,
	EntityAnimationDefinition &&animDef)
{
	this->init(Type::Transition, std::move(animDef));
	this->transition.init(defID);
}

void EntityDefinition::initDoodad(int yOffset, double scale, bool collider, bool transparent,
	bool ceiling, bool streetlight, bool puddle, int lightIntensity,
	EntityAnimationDefinition &&animDef)
{
	this->init(Type::Doodad, std::move(animDef));
	this->doodad.init(yOffset, scale, collider, transparent, ceiling, streetlight,
		puddle, lightIntensity);
}
