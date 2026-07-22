#include <cstring>
#include <limits>

#include "DialogueManager.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"
#include "../Voxels/VoxelChunkManager.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	std::optional<WorldInt2> TryGetNearestInteriorEntranceOfType(const WorldDouble2 startWorldPosition, ArenaInteriorType type, const VoxelChunkManager &voxelChunkManager)
	{
		const WorldInt2 startWorldVoxel = VoxelUtils::pointToVoxel(startWorldPosition);
		const CoordInt2 startVoxelCoord = VoxelUtils::worldVoxelToCoord(startWorldVoxel);
		ChunkInt2 minChunkPos, maxChunkPos;
		ChunkUtils::getSurroundingChunks(startVoxelCoord.chunk, 1, &minChunkPos, &maxChunkPos);

		std::optional<WorldInt2> closestEntranceWorldVoxel;
		for (WEInt chunkZ = minChunkPos.y; chunkZ <= maxChunkPos.y; chunkZ++)
		{
			for (SNInt chunkX = minChunkPos.x; chunkX <= maxChunkPos.x; chunkX++)
			{
				const ChunkInt2 chunkPos(chunkX, chunkZ);

				// Have to attempt the chunk lookup because the dialogue entity might be in a different chunk than the player.
				const VoxelChunk *voxelChunk = voxelChunkManager.findChunkAtPosition(chunkPos);
				if (voxelChunk == nullptr)
				{
					continue;
				}

				for (const std::pair<const VoxelInt3, VoxelTransitionDefID> &pair : voxelChunk->transitionDefIndices)
				{
					const VoxelInt3 transitionVoxel = pair.first;
					const VoxelTransitionDefID transitionDefID = pair.second;
					const TransitionDefinition &transitionDef = voxelChunk->transitionDefs[transitionDefID];
					if (transitionDef.type != TransitionType::EnterInterior)
					{
						continue;
					}

					const InteriorEntranceTransitionDefinition &interiorEntranceTransitionDef = transitionDef.interiorEntrance;
					if (interiorEntranceTransitionDef.interiorGenInfo.interiorType != type)
					{
						continue;
					}

					const CoordInt2 interiorEntranceVoxelCoord(chunkPos, transitionVoxel.getXZ());
					const WorldInt2 interiorEntranceWorldVoxel = VoxelUtils::coordToWorldVoxel(interiorEntranceVoxelCoord);
					if (!closestEntranceWorldVoxel.has_value())
					{
						closestEntranceWorldVoxel = interiorEntranceWorldVoxel;
						continue;
					}

					const WorldDouble2 interiorEntranceWorldPosition = VoxelUtils::getVoxelCenter(interiorEntranceWorldVoxel);
					const WorldDouble2 closestWorldPosition = VoxelUtils::getVoxelCenter(*closestEntranceWorldVoxel);
					const double distanceSqr = (interiorEntranceWorldPosition - startWorldPosition).lengthSquared();
					const double closestDistanceSqr = (closestWorldPosition - startWorldPosition).lengthSquared();
					if (distanceSqr < closestDistanceSqr)
					{
						closestEntranceWorldVoxel = interiorEntranceWorldVoxel;
					}
				}
			}
		}

		return closestEntranceWorldVoxel;
	}

	std::string GetBuildingNameAtWorldVoxel(WorldInt2 worldVoxel, const VoxelChunkManager &voxelChunkManager)
	{
		const CoordInt2 voxelCoord = VoxelUtils::worldVoxelToCoord(worldVoxel);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(voxelCoord.chunk);
		const VoxelInt3 voxel(voxelCoord.voxel.x, 1, voxelCoord.voxel.y);
		VoxelBuildingNameID buildingNameID;
		if (!voxelChunk.tryGetBuildingNameID(voxel.x, voxel.y, voxel.z, &buildingNameID))
		{
			DebugLogErrorFormat("Couldn't get building name at voxel (%d, %d, %d).", voxel.x, voxel.y, voxel.z);
			return std::string();
		}

		return voxelChunk.buildingNames[buildingNameID];
	}
}

DialogueDirectionsEntry::DialogueDirectionsEntry()
{

}

DialogueManager::DialogueManager()
{
	this->game = nullptr;
	this->entityInstID = -1;
	this->dialogueGender = -1;
}

void DialogueManager::init(Game &game)
{
	this->game = &game;

	const Span<const std::pair<const char*, DialogueFunction>> sourceMappings = DialogueFunctions::FunctionMappings;
	this->sortedFunctionMappings.init(sourceMappings.getCount());
	std::copy(sourceMappings.begin(), sourceMappings.end(), this->sortedFunctionMappings.begin());
	std::sort(this->sortedFunctionMappings.begin(), this->sortedFunctionMappings.end(),
		[](const std::pair<const char*, DialogueFunction> &a, const std::pair<const char*, DialogueFunction> &b)
	{
		const size_t aLength = std::strlen(a.first);
		const size_t bLength = std::strlen(b.first);
		if (aLength != bLength)
		{
			return aLength > bLength;
		}

		return std::strcmp(a.first, b.first) < 0;
	});

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const std::string> sourceCityOptions = exeData.services.citizenWhereIsOptionsCity;
	this->cityDirectionsEntries.resize(sourceCityOptions.getCount());
	for (int i = 0; i < sourceCityOptions.getCount(); i++)
	{
		DialogueDirectionsEntry &dstEntry = this->cityDirectionsEntries[i];
		dstEntry.displayString = sourceCityOptions[i];
	}

	const Span<const std::string> sourceWildernessOptions = exeData.services.citizenWhereIsOptionsWilderness;
	this->wildernessDirectionsEntries.resize(sourceWildernessOptions.getCount());
	for (int i = 0; i < sourceWildernessOptions.getCount(); i++)
	{
		DialogueDirectionsEntry &dstEntry = this->wildernessDirectionsEntries[i];
		dstEntry.displayString = sourceWildernessOptions[i];
	}
}

void DialogueManager::beginDialogue(EntityInstanceID entityInstID)
{
	this->entityInstID = entityInstID;
}

void DialogueManager::endDialogue()
{
	this->entityInstID = -1;
	this->dialogueGender = -1;
}

const EntityInstance &DialogueManager::getEntityInstance() const
{
	const EntityChunkManager &entityChunkManager = this->game->sceneManager.entityChunkManager;
	return entityChunkManager.entities.get(this->entityInstID);
}

ArenaNpcPersonalityType DialogueManager::getEntityPersonalityType() const
{
	const EntityInstance &entityInst = this->getEntityInstance();
	const EntityChunkManager &entityChunkManager = this->game->sceneManager.entityChunkManager;
	const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);

	if (entityDef.type == EntityDefinitionType::Citizen)
	{
		return ArenaNpcPersonalityType::Citizen;
	}
	else if (entityDef.type == EntityDefinitionType::StaticNPC)
	{
		const StaticNpcEntityDefinition &staticNpcEntityDef = entityDef.staticNpc;
		DebugAssert(staticNpcEntityDef.type == StaticNpcEntityDefinitionType::General);
		return staticNpcEntityDef.general.type;
	}
	else
	{
		DebugUnhandledReturnMsg(ArenaNpcPersonalityType, std::to_string(static_cast<int>(entityDef.type)));
	}
}

bool DialogueManager::hasEntityBeenIntroduced() const
{
	const EntityInstance &entityInst = this->getEntityInstance();
	// @todo retrieve from entity instance
	return false;
}

WorldDouble2 DialogueManager::getEntityPosition() const
{
	const EntityInstance &entityInst = this->getEntityInstance();
	const EntityChunkManager &entityChunkManager = this->game->sceneManager.entityChunkManager;
	return entityChunkManager.positions.get(entityInst.positionID).getXZ();
}

int DialogueManager::getEntityOccupationIndex() const
{
	const EntityInstance &entityInst = this->getEntityInstance();
	// @todo store in entity instance
	ArenaRandom tempRandom(entityInst.instanceID); // Hacky randomization for now
	return tempRandom.next(100);
}

bool DialogueManager::isDialogueGenderValid() const
{
	return this->dialogueGender >= 0;
}

std::string DialogueManager::getNearestEquipmentStoreName() const
{
	const WorldDouble2 entityWorldPosition = this->getEntityPosition();
	const VoxelChunkManager &voxelChunkManager = this->game->sceneManager.voxelChunkManager;
	const std::optional<WorldInt2> nearestWorldVoxel = TryGetNearestInteriorEntranceOfType(entityWorldPosition, ArenaInteriorType::Equipment, voxelChunkManager);
	if (!nearestWorldVoxel.has_value())
	{
		return "<no nearby equipment store>";
	}	

	return GetBuildingNameAtWorldVoxel(*nearestWorldVoxel, voxelChunkManager);
}

std::string DialogueManager::getNearestTavernName() const
{
	const WorldDouble2 entityWorldPosition = this->getEntityPosition();
	const VoxelChunkManager &voxelChunkManager = this->game->sceneManager.voxelChunkManager;
	const std::optional<WorldInt2> nearestWorldVoxel = TryGetNearestInteriorEntranceOfType(entityWorldPosition, ArenaInteriorType::Tavern, voxelChunkManager);
	if (!nearestWorldVoxel.has_value())
	{
		return "<no nearby tavern>";
	}

	return GetBuildingNameAtWorldVoxel(*nearestWorldVoxel, voxelChunkManager);
}

std::string DialogueManager::getNearestTempleName() const
{
	const WorldDouble2 entityWorldPosition = this->getEntityPosition();
	const VoxelChunkManager &voxelChunkManager = this->game->sceneManager.voxelChunkManager;
	const std::optional<WorldInt2> nearestWorldVoxel = TryGetNearestInteriorEntranceOfType(entityWorldPosition, ArenaInteriorType::Temple, voxelChunkManager);
	if (!nearestWorldVoxel.has_value())
	{
		return "<no nearby temple>";
	}

	return GetBuildingNameAtWorldVoxel(*nearestWorldVoxel, voxelChunkManager);
}

const std::string &DialogueManager::getTemplateDatEntryValueAtIndex(int entryKey, int index) const
{
	const ArenaTemplateDat &templateDat = TextAssetLibrary::getInstance().templateDat;
	const ArenaTemplateDatEntry &entry = templateDat.getEntry(entryKey);
	const Span<const std::string> entryValues = entry.values;
	return entryValues[index];
}

const std::string &DialogueManager::getRandomTemplateDatEntryValue(int entryKey) const
{
	const ArenaTemplateDat &templateDat = TextAssetLibrary::getInstance().templateDat;
	const ArenaTemplateDatEntry &entry = templateDat.getEntry(entryKey);
	const Span<const std::string> entryValues = entry.values;
	const int entryValuesRandomIndex = this->game->random.next(static_cast<int>(entry.values.size()));
	return entryValues[entryValuesRandomIndex];
}

std::string DialogueManager::getSubstitutedText(const char *text, int maxCharsPerLine)
{
	DebugAssert(this->entityInstID >= 0);
	DebugAssert(maxCharsPerLine > 0);

	// @todo optimize these string allocations
	std::string newText = text;

	for (const std::pair<const char*, DialogueFunction> &functionMapping : this->sortedFunctionMappings)
	{
		const char *substitutionToken = functionMapping.first;
		const size_t substitutionTokenLength = std::strlen(substitutionToken);
		const DialogueFunction &substitutionFunction = functionMapping.second;
		
		size_t tokenIndex = newText.find(substitutionToken);
		while (tokenIndex != std::string::npos)
		{
			const std::string replacementString = substitutionFunction(*this->game);
			newText.replace(tokenIndex, substitutionTokenLength, replacementString);
			tokenIndex = newText.find(substitutionToken, tokenIndex + replacementString.length());
		}
	}

	return String::distributeNewlines(newText, maxCharsPerLine);
}
