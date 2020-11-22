#include "EntityDefinitionLibrary.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/ExeData.h"
#include "../World/ClimateType.h"
#include "../World/ClimateUtils.h"

#include "components/debug/Debug.h"

bool EntityDefinitionLibrary::Key::CreatureKey::operator==(const CreatureKey &other) const
{
	return (this->creatureIndex == other.creatureIndex) &&
		(this->isFinalBoss == other.isFinalBoss);
}

void EntityDefinitionLibrary::Key::CreatureKey::init(int creatureIndex, bool isFinalBoss)
{
	this->creatureIndex = creatureIndex;
	this->isFinalBoss = isFinalBoss;
}

/*bool EntityDefinitionLibrary::Key::HumanEnemyKey::operator==(const HumanEnemyKey &other) const
{
	return (this->male == other.male) && (this->charClassID == other.charClassID);
}

void EntityDefinitionLibrary::Key::HumanEnemyKey::init(bool male, int charClassID)
{
	this->male = male;
	this->charClassID = charClassID;
}*/

bool EntityDefinitionLibrary::Key::CitizenKey::operator==(const CitizenKey &other) const
{
	return (this->male == other.male) && (this->climateType == other.climateType);
}

void EntityDefinitionLibrary::Key::CitizenKey::init(bool male, ClimateType climateType)
{
	this->male = male;
	this->climateType = climateType;
}

EntityDefinitionLibrary::Key::Key()
{
	this->type = static_cast<Key::Type>(-1);
}

bool EntityDefinitionLibrary::Key::operator==(const Key &other) const
{
	if (this->type != other.type)
	{
		return false;
	}

	if (this->type == Key::Type::Creature)
	{
		return this->creature == other.creature;
	}
	/*else if (this->type == Key::Type::HumanEnemy)
	{
		return this->humanEnemy == other.humanEnemy;
	}*/
	else if (this->type == Key::Type::Citizen)
	{
		return this->citizen == other.citizen;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

void EntityDefinitionLibrary::Key::init(Type type)
{
	this->type = type;
}

EntityDefinitionLibrary::Key::Type EntityDefinitionLibrary::Key::getType() const
{
	return this->type;
}

const EntityDefinitionLibrary::Key::CreatureKey &EntityDefinitionLibrary::Key::getCreature() const
{
	DebugAssert(this->type == Key::Type::Creature);
	return this->creature;
}

/*const EntityDefinitionLibrary::Key::HumanEnemyKey &EntityDefinitionLibrary::Key::getHumanEnemy() const
{
	DebugAssert(this->type == Key::Type::HumanEnemy);
	return this->humanEnemy;
}*/

const EntityDefinitionLibrary::Key::CitizenKey &EntityDefinitionLibrary::Key::getCitizen() const
{
	DebugAssert(this->type == Key::Type::Citizen);
	return this->citizen;
}

void EntityDefinitionLibrary::Key::initCreature(int creatureIndex, bool isFinalBoss)
{
	this->init(Key::Type::Creature);
	this->creature.init(creatureIndex, isFinalBoss);
}

/*void EntityDefinitionLibrary::Key::initHumanEnemy(bool male, int charClassID)
{
	this->init(Key::Type::HumanEnemy);
	this->humanEnemy.init(male, charClassID);
}*/

void EntityDefinitionLibrary::Key::initCitizen(bool male, ClimateType climateType)
{
	this->init(Key::Type::Citizen);
	this->citizen.init(male, climateType);
}

EntityDefinitionLibrary::Entry::Entry(Key &&key, EntityDefinition &&def)
	: key(std::move(key)), def(std::move(def)) { }

int EntityDefinitionLibrary::findDefIndex(const Key &key) const
{
	for (int i = 0; i < static_cast<int>(this->entries.size()); i++)
	{
		const Entry &entry = this->entries[i];
		if (entry.key == key)
		{
			return i;
		}
	}

	return NO_INDEX;
}

void EntityDefinitionLibrary::init(const ExeData &exeData, TextureManager &textureManager)
{
	// This init method assumes that all creatures, human enemies, and citizens are known
	// in advance of loading any levels, and any code that relies on those definitions can
	// assume that no others are added by a level.

	auto addCreatureDefs = [this, &exeData, &textureManager]()
	{
		auto addCreatureDef = [this, &exeData, &textureManager](int creatureID, bool isFinalBoss)
		{
			EntityAnimationDefinition animDef;
			EntityAnimationInstance animInst;
			if (!ArenaAnimUtils::tryMakeDynamicEntityCreatureAnims(creatureID, exeData,
				textureManager, &animDef, &animInst))
			{
				DebugLogWarning("Couldn't make creature anims for creature ID \"" +
					std::to_string(creatureID) + "\".");
				return;
			}

			const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);

			Key key;
			key.initCreature(creatureIndex, isFinalBoss);

			EntityDefinition entityDef;
			entityDef.initEnemyCreature(creatureIndex, isFinalBoss, exeData, std::move(animDef));

			this->addDefinition(std::move(key), std::move(entityDef));
		};

		// Iterate all creatures + final boss.
		const int creatureCount = static_cast<int>(exeData.entities.creatureNames.size());
		for (int i = 0; i < creatureCount; i++)
		{
			const ArenaTypes::ItemIndex itemIndex = ArenaAnimUtils::getFirstCreatureItemIndex() + i;
			const int creatureID = ArenaAnimUtils::getCreatureIDFromItemIndex(itemIndex);
			addCreatureDef(creatureID, false);
		}

		const int finalBossID = ArenaAnimUtils::getFinalBossCreatureID();
		addCreatureDef(finalBossID, true);
	};

	auto addCitizenDefs = [this, &exeData, &textureManager]()
	{
		auto addCitizenDef = [this, &exeData, &textureManager](ClimateType climateType, bool male)
		{
			EntityAnimationDefinition animDef;
			EntityAnimationInstance animInst;
			if (!ArenaAnimUtils::tryMakeCitizenAnims(climateType, male, exeData,
				textureManager, &animDef, &animInst))
			{
				DebugLogWarning("Couldn't make citizen anims for citizen \"" +
					std::to_string(static_cast<int>(climateType)) + "\".");
				return;
			}

			Key key;
			key.initCitizen(male, climateType);

			EntityDefinition entityDef;
			entityDef.initCitizen(male, climateType, std::move(animDef));

			this->addDefinition(std::move(key), std::move(entityDef));
		};

		// Iterate all climate type + gender combinations.
		for (int i = 0; i < ClimateUtils::getClimateTypeCount(); i++)
		{
			const ClimateType climateType = ClimateUtils::getClimateType(i);
			addCitizenDef(climateType, true);
			addCitizenDef(climateType, false);
		}
	};

	addCreatureDefs();
	addCitizenDefs();
}

int EntityDefinitionLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->entries.size());
}

const EntityDefinition &EntityDefinitionLibrary::getDefinition(EntityDefID defID) const
{
	DebugAssertIndex(this->entries, defID);
	return this->entries[defID].def;
}

bool EntityDefinitionLibrary::tryGetDefinitionID(const Key &key, EntityDefID *outDefID) const
{
	const int index = this->findDefIndex(key);
	if (index != NO_INDEX)
	{
		*outDefID = static_cast<EntityDefID>(index);
		return true;
	}
	else
	{
		return false;
	}
}

EntityDefID EntityDefinitionLibrary::addDefinition(Key &&key, EntityDefinition &&def)
{
	EntityDefID existingDefID;
	if (this->tryGetDefinitionID(key, &existingDefID))
	{
		DebugLogWarning("Already added entity definition (" + std::to_string(existingDefID) + ").");
		return existingDefID;
	}

	this->entries.emplace_back(Entry(std::move(key), std::move(def)));
	return static_cast<EntityDefID>(this->entries.size()) - 1;
}

void EntityDefinitionLibrary::clear()
{
	this->entries.clear();
}
