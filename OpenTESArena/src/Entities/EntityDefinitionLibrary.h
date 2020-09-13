#ifndef ENTITY_DEFINITION_LIBRARY_H
#define ENTITY_DEFINITION_LIBRARY_H

#include <vector>

#include "EntityDefinition.h"
#include "EntityUtils.h"

// Collection of various entity definitions. Not all definition types are supported
// due to insufficient information for look-up/comparison and therefore the definitions
// must be split between this library and the currently active level.

class ExeData;
class TextureManager;

enum class ClimateType;

class EntityDefinitionLibrary
{
public:
	class Key
	{
	public:
		enum class Type
		{
			Creature,
			//HumanEnemy, // Not supported due to dependence on .INF file's corpse texture.
			Citizen
		};

		struct CreatureKey
		{
			int creatureIndex;
			bool isFinalBoss;

			bool operator==(const CreatureKey &other) const;

			void init(int creatureIndex, bool isFinalBoss);
		};

		/*struct HumanEnemyKey
		{
			bool male;
			int charClassID;

			bool operator==(const HumanEnemyKey &other) const;

			void init(bool male, int charClassID);
		};*/

		struct CitizenKey
		{
			bool male;
			ClimateType climateType;
			
			bool operator==(const CitizenKey &other) const;

			void init(bool male, ClimateType climateType);
		};
	private:
		Type type;

		union
		{
			CreatureKey creature;
			//HumanEnemyKey humanEnemy;
			CitizenKey citizen;
		};

		void init(Type type);
	public:
		Key();

		bool operator==(const Key &other) const;

		Type getType() const;
		const CreatureKey &getCreature() const;
		//const HumanEnemyKey &getHumanEnemy() const;
		const CitizenKey &getCitizen() const;

		void initCreature(int creatureIndex, bool isFinalBoss);
		//void initHumanEnemy(bool male, int charClassID);
		void initCitizen(bool male, ClimateType climateType);
	};
private:
	static constexpr int NO_INDEX = -1;

	struct Entry
	{
		Key key;
		EntityDefinition def;

		Entry(Key &&key, EntityDefinition &&def);
	};

	std::vector<Entry> entries;

	int findDefIndex(const Key &key) const;
public:
	// Only a subset of definition types are supported due to variable information
	// available for each. For example, it is currently hard to differentiate a tree
	// and a chair, so they are not supported here.
	static constexpr bool supportsDefType(EntityDefinition::Type type)
	{
		switch (type)
		{
		case EntityDefinition::Type::Citizen:
		case EntityDefinition::Type::Enemy:
			return true;
		default:
			return false;
		}
	}

	void init(const ExeData &exeData, TextureManager &textureManager);

	// Gets the number of entity definitions. This is useful for the currently-active entity
	// manager that needs to start its definition IDs at the end of these.
	int getDefinitionCount() const;

	// Returns raw handle to entity definition (does not protect from dangling pointers).
	const EntityDefinition &getDefinition(EntityDefID defID) const;

	// Attempts to get the definition ID paired with the given definition key.
	bool tryGetDefinitionID(const Key &key, EntityDefID *outDefID) const;

	EntityDefID addDefinition(Key &&key, EntityDefinition &&def);
	void clear();
};

#endif
