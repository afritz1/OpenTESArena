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
	Projectile, // Arrows, spells, etc..
	Transition, // Wilderness den.
	Doodad // Trees, chairs, streetlights, etc..
};

class EntityDefinition
{
public:
	class EnemyDefinition
	{
	public:
		enum class Type
		{
			Creature,
			Human
		};

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
			int bloodIndex;
			int diseaseChances;
			int attributes[8];
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
	private:
		EnemyDefinition::Type type;

		union
		{
			CreatureDefinition creature;
			HumanDefinition human;
		};
	public:
		EnemyDefinition();

		void initCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData);
		void initHuman(bool male, int charClassID);

		bool operator==(const EnemyDefinition &other) const;

		EnemyDefinition::Type getType() const;
		const CreatureDefinition &getCreature() const;
		const HumanDefinition &getHuman() const;
	};

	struct CitizenDefinition
	{
		bool male;
		ArenaTypes::ClimateType climateType;

		CitizenDefinition();

		void init(bool male, ArenaTypes::ClimateType climateType);

		bool operator==(const CitizenDefinition &other) const;
	};

	class StaticNpcDefinition
	{
	public:
		// Unique types of interaction.
		enum class Type
		{
			Shopkeeper,
			Person
		};

		struct ShopkeeperDefinition
		{
			enum class Type
			{
				Blacksmith,
				Bartender,
				Wizard
			};

			ShopkeeperDefinition::Type type;

			void init(ShopkeeperDefinition::Type type);

			bool operator==(const ShopkeeperDefinition &other) const;
		};

		struct PersonDefinition
		{
			// Personality, isRuler, etc..
			// @todo: probably want like a personality ID into personality library.

			bool operator==(const PersonDefinition &other) const;
		};
	private:
		StaticNpcDefinition::Type type;

		union
		{
			ShopkeeperDefinition shopkeeper;
			PersonDefinition person;
		};
	public:
		StaticNpcDefinition();

		void initShopkeeper(ShopkeeperDefinition::Type type);
		void initPerson();

		bool operator==(const StaticNpcDefinition &other) const;

		StaticNpcDefinition::Type getType() const;
		const ShopkeeperDefinition &getShopkeeper() const;
		const PersonDefinition &getPerson() const;
	};

	class ItemDefinition
	{
	public:
		enum class Type
		{
			Key, QuestItem
		};

		struct KeyDefinition
		{
			// @todo
		};

		struct QuestItemDefinition
		{
			// @todo
		};
	private:
		ItemDefinition::Type type;

		union
		{
			KeyDefinition key;
			QuestItemDefinition questItem;
		};
	public:
		ItemDefinition();

		void initKey();
		void initQuestItem();

		bool operator==(const ItemDefinition &other) const;

		ItemDefinition::Type getType() const;
		const KeyDefinition &getKey() const;
		const QuestItemDefinition &getQuestItem() const;
	};

	class ContainerDefinition
	{
	public:
		enum class Type
		{
			Holder, // Can be opened/closed.
			Pile // Loose on the ground.
		};

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
	private:
		ContainerDefinition::Type type;

		union
		{
			HolderDefinition holder;
			PileDefinition pile;
		};
	public:
		ContainerDefinition();

		void initHolder(bool locked);
		void initPile();

		bool operator==(const ContainerDefinition &other) const;

		ContainerDefinition::Type getType() const;
		const HolderDefinition &getHolder() const;
		const PileDefinition &getPile() const;
	};

	struct ProjectileDefinition
	{
		// @todo: may or may not want to store physical damage and spell effects in the same 'effect'.

		bool hasGravity;

		ProjectileDefinition();

		void init(bool hasGravity);

		bool operator==(const ProjectileDefinition &other) const;
	};

	struct TransitionDefinition
	{
		// Should be fine to store this ID that points into a LevelInfoDefinition since transition
		// entities should only exist on the level they're spawned and wouldn't be globally reusable
		// like some entity definitions.
		LevelDefinition::TransitionDefID transitionDefID;

		TransitionDefinition();

		void init(LevelDefinition::TransitionDefID transitionDefID);

		bool operator==(const TransitionDefinition &other) const;
	};

	struct DoodadDefinition
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

		DoodadDefinition();

		void init(int yOffset, double scale, bool collider, bool transparent, bool ceiling,
			bool streetlight, bool puddle, int lightIntensity);

		bool operator==(const DoodadDefinition &other) const;
	};
private:
	EntityDefinitionType type;
	EntityAnimationDefinition animDef;

	union
	{
		EnemyDefinition enemy;
		CitizenDefinition citizen;
		StaticNpcDefinition staticNpc;
		ItemDefinition item;
		ContainerDefinition container;
		ProjectileDefinition projectile;
		TransitionDefinition transition;
		DoodadDefinition doodad;
	};

	void init(EntityDefinitionType type, EntityAnimationDefinition &&animDef);
public:
	EntityDefinition();

	bool operator==(const EntityDefinition &other) const;

	EntityDefinitionType getType() const;
	const EntityAnimationDefinition &getAnimDef() const;
	const EnemyDefinition &getEnemy() const;
	const CitizenDefinition &getCitizen() const;
	const StaticNpcDefinition &getStaticNpc() const;
	const ItemDefinition &getItem() const;
	const ContainerDefinition &getContainer() const;
	const ProjectileDefinition &getProjectile() const;
	const TransitionDefinition &getTransition() const;
	const DoodadDefinition &getDoodad() const;

	// Enemy.
	void initEnemyCreature(int creatureIndex, bool isFinalBoss, const ExeData &exeData,
		EntityAnimationDefinition &&animDef);
	void initEnemyHuman(bool male, int charClassID, EntityAnimationDefinition &&animDef);

	// Citizen.
	void initCitizen(bool male, ArenaTypes::ClimateType climateType, EntityAnimationDefinition &&animDef);

	// Static NPC.
	void initStaticNpcShopkeeper(StaticNpcDefinition::ShopkeeperDefinition::Type type,
		EntityAnimationDefinition &&animDef);
	void initStaticNpcPerson(EntityAnimationDefinition &&animDef);

	// Item.
	void initItemKey(EntityAnimationDefinition &&animDef);
	void initItemQuestItem(EntityAnimationDefinition &&animDef);

	// Container.
	void initContainerHolder(bool locked, EntityAnimationDefinition &&animDef);
	void initContainerPile(EntityAnimationDefinition &&animDef);

	// Projectile.
	void initProjectile(bool hasGravity, EntityAnimationDefinition &&animDef);

	// Transition.
	void initTransition(LevelDefinition::TransitionDefID defID, EntityAnimationDefinition &&animDef);

	// Doodad.
	void initDoodad(int yOffset, double scale, bool collider, bool transparent, bool ceiling,
		bool streetlight, bool puddle, int lightIntensity, EntityAnimationDefinition &&animDef);
};

#endif
