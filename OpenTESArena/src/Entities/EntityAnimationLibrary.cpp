#include <algorithm>

#include "EntityAnimationLibrary.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../World/ArenaClimateUtils.h"

#include "components/debug/Debug.h"

CreatureEntityAnimationKey::CreatureEntityAnimationKey()
{
	this->creatureID = -1;
}

void CreatureEntityAnimationKey::init(int creatureID)
{
	this->creatureID = creatureID;
}

HumanEnemyEntityAnimationKey::HumanEnemyEntityAnimationKey()
{
	this->male = false;
	this->charClassDefID = -1;
}

void HumanEnemyEntityAnimationKey::init(bool male, int charClassDefID)
{
	this->male = male;
	this->charClassDefID = charClassDefID;
}

CitizenEntityAnimationKey::CitizenEntityAnimationKey()
{
	this->male = false;
	this->climateType = static_cast<ArenaTypes::ClimateType>(-1);
}

void CitizenEntityAnimationKey::init(bool male, ArenaTypes::ClimateType climateType)
{
	this->male = male;
	this->climateType = climateType;
}

void EntityAnimationLibrary::init(const BinaryAssetLibrary &binaryAssetLibrary, const CharacterClassLibrary &charClassLibrary, TextureManager &textureManager)
{
	const ExeData &exeData = binaryAssetLibrary.getExeData();

	// Creatures
	const int creatureAnimCount = static_cast<int>(exeData.entities.creatureAnimationFilenames.size());		
	for (int i = 0; i < creatureAnimCount; i++)
	{
		const int creatureID = i + 1;
		EntityAnimationDefinition animDef;
		if (!ArenaAnimUtils::tryMakeDynamicEntityCreatureAnims(creatureID, exeData, textureManager, &animDef))
		{
			DebugLogError("Couldn't create animation definition for creature " + std::to_string(creatureID) + ".");
			continue;
		}

		const EntityAnimationDefinitionID animDefID = static_cast<EntityAnimationDefinitionID>(this->defs.size());
		this->defs.emplace_back(std::move(animDef));
		
		CreatureEntityAnimationKey animKey;
		animKey.init(creatureID);
		this->creatureDefIDs.emplace_back(std::move(animKey), animDefID);
	}

	// Human enemies (knights, mages, etc.)
	const int charClassCount = charClassLibrary.getDefinitionCount();
	for (int i = 0; i < charClassCount; i++)
	{
		const int charClassDefID = i;
		EntityAnimationDefinition maleAnimDef, femaleAnimDef;
		if (!ArenaAnimUtils::tryMakeDynamicEntityHumanAnims(charClassDefID, true, charClassLibrary, binaryAssetLibrary, textureManager, &maleAnimDef))
		{
			DebugLogError("Couldn't create animation definition for male human enemy " + std::to_string(charClassDefID) + ".");
			continue;
		}

		if (!ArenaAnimUtils::tryMakeDynamicEntityHumanAnims(charClassDefID, false, charClassLibrary, binaryAssetLibrary, textureManager, &femaleAnimDef))
		{
			DebugLogError("Couldn't create animation definition for female human enemy " + std::to_string(charClassDefID) + ".");
			continue;
		}

		const EntityAnimationDefinitionID maleAnimDefID = static_cast<EntityAnimationDefinitionID>(this->defs.size());
		const EntityAnimationDefinitionID femaleAnimDefID = maleAnimDefID + 1;
		this->defs.emplace_back(std::move(maleAnimDef));
		this->defs.emplace_back(std::move(femaleAnimDef));

		HumanEnemyEntityAnimationKey maleAnimKey, femaleAnimKey;
		maleAnimKey.init(true, charClassDefID);
		femaleAnimKey.init(false, charClassDefID);
		this->humanEnemyDefIDs.emplace_back(std::move(maleAnimKey), maleAnimDefID);
		this->humanEnemyDefIDs.emplace_back(std::move(femaleAnimKey), femaleAnimDefID);
	}

	// Citizens
	const int climateCount = ArenaClimateUtils::getClimateTypeCount();
	for (int i = 0; i < climateCount; i++)
	{
		const ArenaTypes::ClimateType climateType = ArenaClimateUtils::getClimateType(i);
		EntityAnimationDefinition maleAnimDef, femaleAnimDef;
		if (!ArenaAnimUtils::tryMakeCitizenAnims(climateType, true, exeData, textureManager, &maleAnimDef))
		{
			DebugLogError("Couldn't create animation definition for male citizen " + std::to_string(static_cast<int>(climateType)) + ".");
			continue;
		}

		if (!ArenaAnimUtils::tryMakeCitizenAnims(climateType, false, exeData, textureManager, &femaleAnimDef))
		{
			DebugLogError("Couldn't create animation definition for female citizen " + std::to_string(static_cast<int>(climateType)) + ".");
			continue;
		}

		const EntityAnimationDefinitionID maleAnimDefID = static_cast<EntityAnimationDefinitionID>(this->defs.size());
		const EntityAnimationDefinitionID femaleAnimDefID = maleAnimDefID + 1;
		this->defs.emplace_back(std::move(maleAnimDef));
		this->defs.emplace_back(std::move(femaleAnimDef));

		CitizenEntityAnimationKey maleAnimKey, femaleAnimKey;
		maleAnimKey.init(true, climateType);
		femaleAnimKey.init(false, climateType);
		this->citizenDefIDs.emplace_back(std::move(maleAnimKey), maleAnimDefID);
		this->citizenDefIDs.emplace_back(std::move(femaleAnimKey), femaleAnimDefID);
	}

	// @todo: explosion animations

}

int EntityAnimationLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->defs.size());
}

EntityAnimationDefinitionID EntityAnimationLibrary::getCreatureAnimDefID(const CreatureEntityAnimationKey &key) const
{
	const auto iter = std::find_if(this->creatureDefIDs.begin(), this->creatureDefIDs.end(),
		[&key](const auto &pair)
	{
		const CreatureEntityAnimationKey &animKey = pair.first;
		return animKey.creatureID == key.creatureID;
	});

	DebugAssert(iter != this->creatureDefIDs.end());
	return iter->second;
}

EntityAnimationDefinitionID EntityAnimationLibrary::getHumanEnemyAnimDefID(const HumanEnemyEntityAnimationKey &key) const
{
	const auto iter = std::find_if(this->humanEnemyDefIDs.begin(), this->humanEnemyDefIDs.end(),
		[&key](const auto &pair)
	{
		const HumanEnemyEntityAnimationKey &animKey = pair.first;
		return (animKey.male == key.male) && (animKey.charClassDefID == key.charClassDefID);
	});

	DebugAssert(iter != this->humanEnemyDefIDs.end());
	return iter->second;
}

EntityAnimationDefinitionID EntityAnimationLibrary::getCitizenAnimDefID(const CitizenEntityAnimationKey &key) const
{
	const auto iter = std::find_if(this->citizenDefIDs.begin(), this->citizenDefIDs.end(),
		[&key](const auto &pair)
	{
		const CitizenEntityAnimationKey &animKey = pair.first;
		return (animKey.male == key.male) && (animKey.climateType == key.climateType);
	});

	DebugAssert(iter != this->citizenDefIDs.end());
	return iter->second;
}

const EntityAnimationDefinition &EntityAnimationLibrary::getDefinition(EntityAnimationDefinitionID id) const
{
	DebugAssertIndex(this->defs, id);
	return this->defs[id];
}
