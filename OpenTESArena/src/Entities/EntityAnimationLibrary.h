#ifndef ENTITY_ANIMATION_LIBRARY_H
#define ENTITY_ANIMATION_LIBRARY_H

#include <unordered_map>
#include <vector>

#include "EntityAnimationDefinition.h"
#include "../Assets/ArenaTypes.h"

#include "components/utilities/Singleton.h"

class BinaryAssetLibrary;
class CharacterClassLibrary;
class ExeData;
class TextureManager;

struct CreatureEntityAnimationKey
{
	int creatureID; // 1-based original ID (rat = 1, goblin = 2, ...)

	CreatureEntityAnimationKey();

	void init(int creatureID);
};

struct HumanEnemyEntityAnimationKey
{
	bool male;
	int charClassDefID;

	HumanEnemyEntityAnimationKey();

	void init(bool male, int charClassDefID);
};

struct CitizenEntityAnimationKey
{
	bool male;
	ArenaTypes::ClimateType climateType;

	CitizenEntityAnimationKey();

	void init(bool male, ArenaTypes::ClimateType climateType);
};

using EntityAnimationDefinitionID = int;

class EntityAnimationLibrary : public Singleton<EntityAnimationLibrary>
{
private:
	std::vector<EntityAnimationDefinition> defs;
	std::vector<std::pair<CreatureEntityAnimationKey, EntityAnimationDefinitionID>> creatureDefIDs;
	std::vector<std::pair<HumanEnemyEntityAnimationKey, EntityAnimationDefinitionID>> humanEnemyDefIDs;
	std::vector<std::pair<CitizenEntityAnimationKey, EntityAnimationDefinitionID>> citizenDefIDs;
public:
	void init(const BinaryAssetLibrary &binaryAssetLibrary, const CharacterClassLibrary &charClassLibrary, TextureManager &textureManager);

	int getDefinitionCount() const;
	EntityAnimationDefinitionID getCreatureAnimDefID(const CreatureEntityAnimationKey &key) const;
	EntityAnimationDefinitionID getHumanEnemyAnimDefID(const HumanEnemyEntityAnimationKey &key) const;
	EntityAnimationDefinitionID getCitizenAnimDefID(const CitizenEntityAnimationKey &key) const;
	const EntityAnimationDefinition &getDefinition(EntityAnimationDefinitionID id) const;
};

#endif
