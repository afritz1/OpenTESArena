#ifndef ENTITY_DEFINITION_LIBRARY_H
#define ENTITY_DEFINITION_LIBRARY_H

#include <vector>

#include "EntityDefinition.h"
#include "EntityUtils.h"
#include "../Assets/ArenaTypes.h"

#include "components/utilities/Singleton.h"

class EntityAnimationLibrary;
class TextureManager;

struct ExeData;

enum class EntityDefinitionKeyType
{
	Creature,
	HumanEnemy,
	Citizen,
	Vfx
};

struct CreatureEntityDefinitionKey
{
	int creatureIndex;
	bool isFinalBoss;

	bool operator==(const CreatureEntityDefinitionKey &other) const;

	void init(int creatureIndex, bool isFinalBoss);
};

struct HumanEnemyEntityDefinitionKey
{
	bool male;
	int charClassID;

	bool operator==(const HumanEnemyEntityDefinitionKey &other) const;

	void init(bool male, int charClassID);
};

struct CitizenEntityDefinitionKey
{
	bool male;
	ArenaTypes::ClimateType climateType;

	bool operator==(const CitizenEntityDefinitionKey &other) const;

	void init(bool male, ArenaTypes::ClimateType climateType);
};

struct VfxEntityDefinitionKey
{
	VfxEntityAnimationType type;
	int index;

	bool operator==(const VfxEntityDefinitionKey &other) const;

	void init(VfxEntityAnimationType type, int index);
};

struct EntityDefinitionKey
{
	EntityDefinitionKeyType type;

	union
	{
		CreatureEntityDefinitionKey creature;
		HumanEnemyEntityDefinitionKey humanEnemy;
		CitizenEntityDefinitionKey citizen;
		VfxEntityDefinitionKey vfx;
	};

	void init(EntityDefinitionKeyType type);

	EntityDefinitionKey();

	bool operator==(const EntityDefinitionKey &other) const;

	void initCreature(int creatureIndex, bool isFinalBoss);
	void initHumanEnemy(bool male, int charClassID);
	void initCitizen(bool male, ArenaTypes::ClimateType climateType);
	void initVfx(VfxEntityAnimationType type, int index);
};

// Collection of various entity definitions. Not all definition types are supported
// due to insufficient information for look-up/comparison and therefore the definitions
// must be split between this library and the currently active level.
class EntityDefinitionLibrary : public Singleton<EntityDefinitionLibrary>
{
private:
	static constexpr int NO_INDEX = -1;

	struct Entry
	{
		EntityDefinitionKey key;
		EntityDefinition def;

		Entry(EntityDefinitionKey &&key, EntityDefinition &&def);
	};

	std::vector<Entry> entries;

	int findDefIndex(const EntityDefinitionKey &key) const;
public:
	// Only a subset of definition types are supported due to variable information
	// available for each. For example, it is currently hard to differentiate a tree
	// and a chair, so they are not supported here.
	static constexpr bool supportsDefType(EntityDefinitionType type)
	{
		switch (type)
		{
		case EntityDefinitionType::Citizen:
		case EntityDefinitionType::Enemy:
			return true;
		default:
			return false;
		}
	}

	void init(const ExeData &exeData, const CharacterClassLibrary &charClassLibrary, const EntityAnimationLibrary &entityAnimLibrary);

	// Gets the number of entity definitions. This is useful for the currently-active entity
	// manager that needs to start its definition IDs at the end of these.
	int getDefinitionCount() const;

	// Returns raw handle to entity definition (does not protect from dangling pointers).
	const EntityDefinition &getDefinition(EntityDefID defID) const;

	// Attempts to get the definition ID paired with the given definition key.
	bool tryGetDefinitionID(const EntityDefinitionKey &key, EntityDefID *outDefID) const;

	EntityDefID addDefinition(EntityDefinitionKey &&key, EntityDefinition &&def);
	void clear();
};

#endif
