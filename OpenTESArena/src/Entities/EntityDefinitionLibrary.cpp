#include "EntityAnimationLibrary.h"
#include "EntityDefinitionLibrary.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/ExeData.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../World/ArenaClimateUtils.h"

#include "components/debug/Debug.h"

bool CreatureEntityDefinitionKey::operator==(const CreatureEntityDefinitionKey &other) const
{
	return (this->creatureIndex == other.creatureIndex) && (this->isFinalBoss == other.isFinalBoss);
}

void CreatureEntityDefinitionKey::init(int creatureIndex, bool isFinalBoss)
{
	this->creatureIndex = creatureIndex;
	this->isFinalBoss = isFinalBoss;
}

bool HumanEnemyEntityDefinitionKey::operator==(const HumanEnemyEntityDefinitionKey &other) const
{
	return (this->male == other.male) && (this->charClassID == other.charClassID);
}

void HumanEnemyEntityDefinitionKey::init(bool male, int charClassID)
{
	this->male = male;
	this->charClassID = charClassID;
}

bool CitizenEntityDefinitionKey::operator==(const CitizenEntityDefinitionKey &other) const
{
	return (this->male == other.male) && (this->climateType == other.climateType);
}

void CitizenEntityDefinitionKey::init(bool male, ArenaTypes::ClimateType climateType)
{
	this->male = male;
	this->climateType = climateType;
}

bool VfxEntityDefinitionKey::operator==(const VfxEntityDefinitionKey &other) const
{
	return (this->type == other.type) && (this->index == other.index);
}

void VfxEntityDefinitionKey::init(VfxEntityAnimationType type, int index)
{
	this->type = type;
	this->index = index;
}

EntityDefinitionKey::EntityDefinitionKey()
{
	this->type = static_cast<EntityDefinitionKeyType>(-1);
}

bool EntityDefinitionKey::operator==(const EntityDefinitionKey &other) const
{
	if (this->type != other.type)
	{
		return false;
	}

	if (this->type == EntityDefinitionKeyType::Creature)
	{
		return this->creature == other.creature;
	}
	else if (this->type == EntityDefinitionKeyType::HumanEnemy)
	{
		return this->humanEnemy == other.humanEnemy;
	}
	else if (this->type == EntityDefinitionKeyType::Citizen)
	{
		return this->citizen == other.citizen;
	}
	else if (this->type == EntityDefinitionKeyType::Vfx)
	{
		return this->vfx == other.vfx;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}

void EntityDefinitionKey::init(EntityDefinitionKeyType type)
{
	this->type = type;
}

void EntityDefinitionKey::initCreature(int creatureIndex, bool isFinalBoss)
{
	this->init(EntityDefinitionKeyType::Creature);
	this->creature.init(creatureIndex, isFinalBoss);
}

void EntityDefinitionKey::initHumanEnemy(bool male, int charClassID)
{
	this->init(EntityDefinitionKeyType::HumanEnemy);
	this->humanEnemy.init(male, charClassID);
}

void EntityDefinitionKey::initCitizen(bool male, ArenaTypes::ClimateType climateType)
{
	this->init(EntityDefinitionKeyType::Citizen);
	this->citizen.init(male, climateType);
}

void EntityDefinitionKey::initVfx(VfxEntityAnimationType type, int index)
{
	this->init(EntityDefinitionKeyType::Vfx);
	this->vfx.init(type, index);
}

EntityDefinitionLibrary::Entry::Entry(EntityDefinitionKey &&key, EntityDefinition &&def)
	: key(std::move(key)), def(std::move(def)) { }

int EntityDefinitionLibrary::findDefIndex(const EntityDefinitionKey &key) const
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

void EntityDefinitionLibrary::init(const ExeData &exeData, const CharacterClassLibrary &charClassLibrary, const EntityAnimationLibrary &entityAnimLibrary)
{
	// This init method assumes that all creatures, human enemies, and citizens are known
	// in advance of loading any levels, and any code that relies on those definitions can
	// assume that no others are added by a level.

	auto addCreatureDef = [this, &exeData, &entityAnimLibrary](int creatureID, bool isFinalBoss)
	{
		CreatureEntityAnimationKey animKey;
		animKey.init(creatureID);

		const EntityAnimationDefinitionID animDefID = entityAnimLibrary.getCreatureAnimDefID(animKey);
		EntityAnimationDefinition animDef = entityAnimLibrary.getDefinition(animDefID); // @todo: make const ref and give anim def ID to EntityDefinition instead
		const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);

		EntityDefinitionKey key;
		key.initCreature(creatureIndex, isFinalBoss);

		EntityDefinition entityDef;
		entityDef.initEnemyCreature(creatureIndex, isFinalBoss, exeData, std::move(animDef));

		this->addDefinition(std::move(key), std::move(entityDef));
	};

	auto addHumanEnemyDef = [this, &exeData, &entityAnimLibrary](bool male, int charClassID)
	{
		HumanEnemyEntityAnimationKey animKey;
		animKey.init(male, charClassID);

		const EntityAnimationDefinitionID animDefID = entityAnimLibrary.getHumanEnemyAnimDefID(animKey);
		EntityAnimationDefinition animDef = entityAnimLibrary.getDefinition(animDefID); // @todo: make const ref and give anim def ID to EntityDefinition instead

		EntityDefinitionKey key;
		key.initHumanEnemy(male, charClassID);

		EntityDefinition entityDef;
		entityDef.initEnemyHuman(male, charClassID, std::move(animDef));

		this->addDefinition(std::move(key), std::move(entityDef));
	};

	auto addCitizenDef = [this, &exeData, &entityAnimLibrary](ArenaTypes::ClimateType climateType, bool male)
	{
		CitizenEntityAnimationKey animKey;
		animKey.init(male, climateType);

		const EntityAnimationDefinitionID animDefID = entityAnimLibrary.getCitizenAnimDefID(animKey);
		EntityAnimationDefinition animDef = entityAnimLibrary.getDefinition(animDefID); // @todo: make const ref and give anim def ID to EntityDefinition instead

		EntityDefinitionKey key;
		key.initCitizen(male, climateType);

		EntityDefinition entityDef;
		entityDef.initCitizen(male, climateType, std::move(animDef));

		this->addDefinition(std::move(key), std::move(entityDef));
	};

	auto addVfxDef = [this, &exeData, &entityAnimLibrary](VfxEntityAnimationType type, int index)
	{
		VfxEntityAnimationKey animKey;
		switch (type)
		{
		case VfxEntityAnimationType::SpellProjectile:
			animKey.initSpellProjectile(index);
			break;
		case VfxEntityAnimationType::SpellExplosion:
			animKey.initSpellExplosion(index);
			break;
		case VfxEntityAnimationType::MeleeStrike:
			animKey.initMeleeStrike(index);
			break;
		default:
			DebugNotImplemented();
			break;
		}

		const EntityAnimationDefinitionID animDefID = entityAnimLibrary.getVfxAnimDefID(animKey);
		EntityAnimationDefinition animDef = entityAnimLibrary.getDefinition(animDefID); // @todo: make const ref and give anim def ID to EntityDefinition instead

		EntityDefinitionKey key;
		key.initVfx(type, index);

		EntityDefinition entityDef;
		entityDef.initVfx(type, index, std::move(animDef));

		this->addDefinition(std::move(key), std::move(entityDef));
	};

	// Iterate all creatures + final boss.
	const int creatureCount = static_cast<int>(exeData.entities.creatureNames.size());
	for (int i = 0; i < creatureCount; i++)
	{
		const ArenaTypes::ItemIndex itemIndex = ArenaAnimUtils::FirstCreatureItemIndex + i;
		const int creatureID = ArenaAnimUtils::getCreatureIDFromItemIndex(itemIndex);
		addCreatureDef(creatureID, false);
	}

	const int finalBossID = ArenaAnimUtils::FinalBossCreatureID;
	addCreatureDef(finalBossID, true);

	// Iterate all human enemies.
	const int charClassCount = charClassLibrary.getDefinitionCount();
	for (int i = 0; i < charClassCount; i++)
	{
		const int charClassID = i;
		addHumanEnemyDef(true, charClassID);
		addHumanEnemyDef(false, charClassID);
	}

	// Iterate all climate type + gender combinations for citizens.
	for (int i = 0; i < ArenaClimateUtils::getClimateTypeCount(); i++)
	{
		const ArenaTypes::ClimateType climateType = ArenaClimateUtils::getClimateType(i);
		addCitizenDef(climateType, true);
		addCitizenDef(climateType, false);
	}

	// Iterate all spell effects and melee effects.
	const int spellTypeCount = EntityAnimationUtils::SPELL_TYPE_COUNT;
	const int meleeVfxCount = EntityAnimationUtils::MELEE_VFX_COUNT;
	for (int i = 0; i < spellTypeCount; i++)
	{
		addVfxDef(VfxEntityAnimationType::SpellProjectile, i);
	}

	for (int i = 0; i < spellTypeCount; i++)
	{
		addVfxDef(VfxEntityAnimationType::SpellExplosion, i);
	}

	for (int i = 0; i < meleeVfxCount; i++)
	{
		addVfxDef(VfxEntityAnimationType::MeleeStrike, i);
	}
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

bool EntityDefinitionLibrary::tryGetDefinitionID(const EntityDefinitionKey &key, EntityDefID *outDefID) const
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

EntityDefID EntityDefinitionLibrary::addDefinition(EntityDefinitionKey &&key, EntityDefinition &&def)
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
