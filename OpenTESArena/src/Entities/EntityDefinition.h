#ifndef ENTITY_DEFINITION_H
#define ENTITY_DEFINITION_H

#include <optional>
#include <string_view>

#include "EntityAnimationDefinition.h"
#include "EntityAnimationUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/ExeData.h"
#include "../World/LevelDefinition.h"

enum class EntityDefinitionType
{
	Enemy, // Creatures and human enemies.
	Citizen, // Wandering people.
	StaticNPC, // Bartenders, priests, etc..
	Item, // Keys, tablets, staff pieces, etc..
	Container, // Chests, loot piles, etc..
	Projectile, // Arrows
	Vfx, // Spell projectile, explosion, or melee strike
	Transition, // Wilderness den.
	Decoration // Trees, chairs, streetlights, etc..
};

enum class EnemyEntityDefinitionType
{
	Creature,
	Human
};

struct EnemyEntityDefinition
{
	// @todo: move this into a creature library so it can just be an ID instead.
	// @todo: also it is basically an ArenaCreatureDefinition since it copy-pastes so much from ExeData.
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
		bool hasNoCorpse;
		int bloodIndex; // @todo this should be an EntityDefID to the vfx in EntityDefinitionLibrary, or -1
		int diseaseChances;
		int attributes[8];
		int lootChances;
		bool ghost;

		void init(int creatureIndex, bool isFinalBoss, const ExeData &exeData);

		bool operator==(const CreatureDefinition &other) const;
	};

	struct HumanDefinition
	{
		bool male;
		int charClassID;

		void init(bool male, int charClassID);

		bool operator==(const HumanDefinition &other) const;
	};

	EnemyEntityDefinitionType type;

	union
	{
		CreatureDefinition creature;
		HumanDefinition human;
	};

	EnemyEntityDefinition();

	void initCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData);
	void initHuman(bool male, int charClassID);

	bool operator==(const EnemyEntityDefinition &other) const;
};

struct CitizenEntityDefinition
{
	bool male;
	ArenaClimateType climateType;

	CitizenEntityDefinition();

	void init(bool male, ArenaClimateType climateType);

	bool operator==(const CitizenEntityDefinition &other) const;
};

enum class StaticNpcPersonalityType
{
	Shopkeeper,
	Beggar,
	Firebreather,
	Prostitute,
	Jester,
	StreetVendor,
	Musician,
	Priest,
	Thief,
	SnakeCharmer,
	StreetVendorAlchemist,
	Wizard,
	TavernPatron
};

struct StaticNpcEntityDefinition
{
	StaticNpcPersonalityType personalityType;

	StaticNpcEntityDefinition();

	void init(StaticNpcPersonalityType personalityType);

	bool operator==(const StaticNpcEntityDefinition &other) const;
};

enum class ItemEntityDefinitionType
{
	Key, QuestItem
};

struct ItemEntityDefinition
{
	struct KeyDefinition
	{
		// @todo
	};

	struct QuestItemDefinition
	{
		int yOffset;

		bool operator==(const QuestItemDefinition &other) const;
	};

	ItemEntityDefinitionType type;

	union
	{
		KeyDefinition key;
		QuestItemDefinition questItem;
	};

	ItemEntityDefinition();

	void initKey();
	void initQuestItem(int yOffset);

	bool operator==(const ItemEntityDefinition &other) const;
};

enum class ContainerEntityDefinitionType
{
	Holder, // Can be opened/closed.
	Pile // Loose on the ground.
};

struct ContainerEntityDefinition
{
	struct HolderDefinition
	{
		bool locked;
		// @todo: loot table ID?

		HolderDefinition();

		void init(bool locked);

		bool operator==(const HolderDefinition &other) const;
	};

	struct PileDefinition
	{
		// @todo: loot table ID?

		bool operator==(const PileDefinition &other) const;
	};

	ContainerEntityDefinitionType type;

	union
	{
		HolderDefinition holder;
		PileDefinition pile;
	};

	ContainerEntityDefinition();

	void initHolder(bool locked);
	void initPile();

	bool operator==(const ContainerEntityDefinition &other) const;
};

struct ProjectileEntityDefinition
{
	// @todo: may or may not want to store physical damage and spell effects in the same 'effect'.

	bool hasGravity;

	ProjectileEntityDefinition();

	void init(bool hasGravity);

	bool operator==(const ProjectileEntityDefinition &other) const;
};

enum class VfxEntityAnimationType
{
	SpellProjectile,
	SpellExplosion,
	MeleeStrike
};

struct VfxEntityDefinition
{
	VfxEntityAnimationType type;
	int index; // Points into projectiles, explosions, or blood effects.

	VfxEntityDefinition();

	void init(VfxEntityAnimationType type, int index);

	bool operator==(const VfxEntityDefinition &other) const;
};

struct TransitionEntityDefinition
{
	// Should be fine to store this ID that points into a LevelInfoDefinition since transition
	// entities should only exist on the level they're spawned and wouldn't be globally reusable
	// like some entity definitions.
	LevelVoxelTransitionDefID transitionDefID;

	TransitionEntityDefinition();

	void init(LevelVoxelTransitionDefID transitionDefID);

	bool operator==(const TransitionEntityDefinition &other) const;
};

struct DecorationEntityDefinition
{
	// @todo: eventually convert these to modern values (percentages, etc.).
	int yOffset;
	double scale;
	bool collider;
	bool transparent;
	bool ceiling;
	bool streetlight;
	bool puddle;
	int lightIntensity; // Has intensity if over 0.

	DecorationEntityDefinition();

	void init(int yOffset, double scale, bool collider, bool transparent, bool ceiling, bool streetlight, bool puddle, int lightIntensity);

	bool operator==(const DecorationEntityDefinition &other) const;
};

struct EntityDefinition
{
	EntityDefinitionType type;
	EntityAnimationDefinition animDef;

	union
	{
		EnemyEntityDefinition enemy;
		CitizenEntityDefinition citizen;
		StaticNpcEntityDefinition staticNpc;
		ItemEntityDefinition item;
		ContainerEntityDefinition container;
		ProjectileEntityDefinition projectile;
		VfxEntityDefinition vfx;
		TransitionEntityDefinition transition;
		DecorationEntityDefinition decoration;
	};

	EntityDefinition();

	bool operator==(const EntityDefinition &other) const;
	
	// Internal initializer
	void init(EntityDefinitionType type, EntityAnimationDefinition &&animDef);

	void initEnemyCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData, EntityAnimationDefinition &&animDef);
	void initEnemyHuman(bool male, int charClassID, EntityAnimationDefinition &&animDef);

	void initCitizen(bool male, ArenaClimateType climateType, EntityAnimationDefinition &&animDef);

	void initStaticNpc(StaticNpcPersonalityType type, EntityAnimationDefinition &&animDef);

	void initItemKey(EntityAnimationDefinition &&animDef);
	void initItemQuestItem(int yOffset, EntityAnimationDefinition &&animDef);

	void initContainerHolder(bool locked, EntityAnimationDefinition &&animDef);
	void initContainerPile(EntityAnimationDefinition &&animDef);

	void initProjectile(bool hasGravity, EntityAnimationDefinition &&animDef);
	
	void initVfx(VfxEntityAnimationType type, int index, EntityAnimationDefinition &&animDef);

	void initTransition(LevelVoxelTransitionDefID defID, EntityAnimationDefinition &&animDef);

	void initDecoration(int yOffset, double scale, bool collider, bool transparent, bool ceiling, bool streetlight, bool puddle,
		int lightIntensity, EntityAnimationDefinition &&animDef);
};

#endif
