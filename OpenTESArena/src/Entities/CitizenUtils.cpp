#include "CitizenUtils.h"
#include "EntityDefinitionLibrary.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/CardinalDirectionName.h"
#include "../Math/Random.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer.h"

void CitizenUtils::CitizenGenInfo::init(EntityDefID maleEntityDefID, EntityDefID femaleEntityDefID,
	const EntityDefinition *maleEntityDef, const EntityDefinition *femaleEntityDef,
	EntityAnimationInstance &&maleAnimInst, EntityAnimationInstance &&femaleAnimInst, PaletteID paletteID, int raceID)
{
	this->maleEntityDefID = maleEntityDefID;
	this->femaleEntityDefID = femaleEntityDefID;
	this->maleEntityDef = maleEntityDef;
	this->femaleEntityDef = femaleEntityDef;
	this->maleAnimInst = std::move(maleAnimInst);
	this->femaleAnimInst = std::move(femaleAnimInst);
	this->paletteID = paletteID;
	this->raceID = raceID;
}

int CitizenUtils::getCitizenCount(const EntityManager &entityManager)
{
	constexpr EntityType entityType = EntityType::Dynamic;
	Buffer<const Entity*> entities(entityManager.getCountOfType(entityType));
	const int entityWriteCount = entityManager.getEntitiesOfType(entityType, entities.get(), entities.getCount());

	int count = 0;
	for (int i = 0; i < entityWriteCount; i++)
	{
		const Entity *entity = entities.get(i);
		const DynamicEntity *dynamicEntity = dynamic_cast<const DynamicEntity*>(entity);
		if (dynamicEntity->getDerivedType() == DynamicEntityType::Citizen)
		{
			count++;
		}
	}

	return count;
}

int CitizenUtils::getCitizenCountInChunk(const ChunkInt2 &chunk, const EntityManager &entityManager)
{
	constexpr EntityType entityType = EntityType::Dynamic;
	Buffer<const Entity*> entities(entityManager.getCountInChunk(chunk));
	const int entityWriteCount = entityManager.getEntitiesInChunk(chunk, entities.get(), entities.getCount());

	int count = 0;
	for (int i = 0; i < entityWriteCount; i++)
	{
		const Entity *entity = entities.get(i);
		if (entity->getEntityType() == entityType)
		{
			const DynamicEntity *dynamicEntity = dynamic_cast<const DynamicEntity*>(entity);
			if (dynamicEntity->getDerivedType() == DynamicEntityType::Citizen)
			{
				count++;
			}
		}
	}

	return count;
}

CitizenUtils::CitizenGenInfo CitizenUtils::makeCitizenGenInfo(int raceID, ClimateType climateType,
	const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager)
{
	// Citizen entity definitions are level-independent and stored in a library beforehand.
	static_assert(EntityDefinitionLibrary::supportsDefType(EntityDefinition::Type::Citizen));
	EntityDefinitionLibrary::Key maleEntityDefKey, femaleEntityDefKey;
	maleEntityDefKey.initCitizen(true, climateType);
	femaleEntityDefKey.initCitizen(false, climateType);

	EntityDefID maleEntityDefID, femaleEntityDefID;
	if (!entityDefLibrary.tryGetDefinitionID(maleEntityDefKey, &maleEntityDefID) ||
		!entityDefLibrary.tryGetDefinitionID(femaleEntityDefKey, &femaleEntityDefID))
	{
		DebugCrash("Couldn't get citizen entity def ID from library.");
	}

	// Only two citizen entity definitions for a given climate, based on the gender.
	const EntityDefinition &maleEntityDef = entityDefLibrary.getDefinition(maleEntityDefID);
	const EntityDefinition &femaleEntityDef = entityDefLibrary.getDefinition(femaleEntityDefID);
	EntityAnimationInstance maleAnimInst, femaleAnimInst;
	maleAnimInst.init(maleEntityDef.getAnimDef());
	femaleAnimInst.init(femaleEntityDef.getAnimDef());

	// Base palette for citizens to generate from.
	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get default palette \"" + paletteName + "\".");
	}

	CitizenGenInfo citizenGenInfo;
	citizenGenInfo.init(maleEntityDefID, femaleEntityDefID, &maleEntityDef, &femaleEntityDef,
		std::move(maleAnimInst), std::move(femaleAnimInst), *paletteID, raceID);
	return citizenGenInfo;
}

bool CitizenUtils::trySpawnCitizenInChunk(const Chunk &chunk, const CitizenGenInfo &citizenGenInfo, Random &random,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, EntityManager &entityManager)
{
	const ChunkInt2 &chunkCoord = chunk.getCoord();
	if (!entityManager.hasChunk(chunkCoord))
	{
		DebugLogWarning("Can't spawn a citizen in untracked chunk \"" + chunkCoord.toString() + "\".");
		return false;
	}

	const std::optional<VoxelInt2> spawnVoxel = [&chunk, &random]() -> std::optional<VoxelInt2>
	{
		DebugAssert(chunk.getHeight() >= 2);

		constexpr int spawnTriesCount = 20;
		for (int spawnTry = 0; spawnTry < spawnTriesCount; spawnTry++)
		{
			const VoxelInt2 spawnVoxel(random.next(Chunk::WIDTH), random.next(Chunk::DEPTH));
			const Chunk::VoxelID voxelID = chunk.getVoxel(spawnVoxel.x, 1, spawnVoxel.y);
			const Chunk::VoxelID groundVoxelID = chunk.getVoxel(spawnVoxel.x, 0, spawnVoxel.y);
			const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
			const VoxelDefinition &groundVoxelDef = chunk.getVoxelDef(groundVoxelID);

			if ((voxelDef.type == ArenaTypes::VoxelType::None) &&
				(groundVoxelDef.type == ArenaTypes::VoxelType::Floor))
			{
				return spawnVoxel;
			}
		}

		// No spawn position found.
		return std::nullopt;
	}();

	if (!spawnVoxel.has_value())
	{
		DebugLogWarning("Couldn't find spawn voxel for citizen.");
		return false;
	}

	const bool male = random.next(2) == 0;
	const EntityDefID entityDefID = male ? citizenGenInfo.maleEntityDefID : citizenGenInfo.femaleEntityDefID;
	const EntityDefinition &entityDef = male ? *citizenGenInfo.maleEntityDef : *citizenGenInfo.femaleEntityDef;
	const EntityAnimationDefinition &entityAnimDef = entityDef.getAnimDef();
	const EntityAnimationInstance &entityAnimInst = male ? citizenGenInfo.maleAnimInst : citizenGenInfo.femaleAnimInst;

	const Palette &palette = textureManager.getPaletteHandle(citizenGenInfo.paletteID);
	const uint16_t colorSeed = static_cast<uint16_t>(random.next());
	const Palette generatedPalette = ArenaAnimUtils::transformCitizenColors(
		citizenGenInfo.raceID, colorSeed, palette, binaryAssetLibrary.getExeData());

	EntityRef entityRef = entityManager.makeEntity(EntityType::Dynamic);
	DynamicEntity *dynamicEntity = entityRef.getDerived<DynamicEntity>();
	constexpr CardinalDirectionName direction = CardinalDirectionName::North;
	dynamicEntity->initCitizen(entityDefID, entityAnimInst, direction);

	// Idle animation by default.
	const std::optional<int> stateIndex = entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
	if (!stateIndex.has_value())
	{
		DebugLogWarning("Couldn't get idle state index for citizen.");
		return false;
	}

	EntityAnimationInstance &animInst = dynamicEntity->getAnimInstance();
	animInst.setStateIndex(*stateIndex);

	auto citizenParams = std::make_unique<EntityAnimationInstance::CitizenParams>();
	citizenParams->palette = generatedPalette;
	animInst.setCitizenParams(std::move(citizenParams));

	// Note: since the entity pointer is being used directly, update the position last
	// in scope to avoid a dangling pointer problem in case it changes chunks.
	const CoordDouble2 spawnCoordReal(chunkCoord, VoxelUtils::getVoxelCenter(*spawnVoxel));
	dynamicEntity->setPosition(spawnCoordReal, entityManager);

	return true;
}

void CitizenUtils::writeCitizenTextures(const EntityDefinition &maleEntityDef, const EntityDefinition &femaleEntityDef,
	TextureManager &textureManager, Renderer &renderer)
{
	auto writeTextures = [&maleEntityDef, &femaleEntityDef , &textureManager, &renderer](bool male)
	{
		const EntityDefinition &entityDef = male ? maleEntityDef : femaleEntityDef;
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		for (int i = 0; i < animDef.getStateCount(); i++)
		{
			const EntityAnimationDefinition::State &state = animDef.getState(i);
			for (int j = 0; j < state.getKeyframeListCount(); j++)
			{
				const EntityAnimationDefinition::KeyframeList &keyframeList = state.getKeyframeList(j);
				const bool flipped = keyframeList.isFlipped();
				for (int k = 0; k < keyframeList.getKeyframeCount(); k++)
				{
					const EntityAnimationDefinition::Keyframe &keyframe = keyframeList.getKeyframe(k);
					const TextureAssetReference &textureAssetRef = keyframe.getTextureAssetRef();
					constexpr bool reflective = false; // Citizens are not puddles.
					// @todo: duplicate texture check in some rendererEntityTextureCache
					if (!renderer.tryCreateEntityTexture(textureAssetRef, flipped, reflective, textureManager))
					{
						DebugLogError("Couldn't create renderer entity texture for \"" + textureAssetRef.filename + "\".");
					}
				}
			}
		}
	};

	writeTextures(true);
	writeTextures(false);
}

void CitizenUtils::clearCitizens(EntityManager &entityManager)
{
	constexpr EntityType entityType = EntityType::Dynamic;
	Buffer<const Entity*> entities(entityManager.getCountOfType(entityType));
	const int entityWriteCount = entityManager.getEntitiesOfType(entityType, entities.get(), entities.getCount());

	for (int i = 0; i < entityWriteCount; i++)
	{
		const Entity *entity = entities.get(i);
		const DynamicEntity *dynamicEntity = dynamic_cast<const DynamicEntity*>(entity);
		if (dynamicEntity->getDerivedType() == DynamicEntityType::Citizen)
		{
			entityManager.remove(dynamicEntity->getID());
		}
	}
}
