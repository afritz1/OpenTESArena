#include "DynamicEntity.h"
#include "EntityChunkManager.h"
#include "EntityDefinitionLibrary.h"
#include "EntityType.h"
#include "EntityVisibilityState.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextureManager.h"
#include "../Audio/AudioManager.h"
#include "../Game/CardinalDirection.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Rendering/Renderer.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/LevelDefinition.h"
#include "../World/LevelInfoDefinition.h"
#include "../World/MapDefinition.h"
#include "../World/MapType.h"

#include "components/utilities/String.h"

namespace
{
	Buffer<ScopedObjectTextureRef> MakeAnimTextureRefs(const EntityAnimationDefinition &animDef, TextureManager &textureManager, Renderer &renderer)
	{
		const int keyframeCount = animDef.keyframeCount;
		Buffer<ScopedObjectTextureRef> animTextureRefs(keyframeCount);
		
		for (int i = 0; i < keyframeCount; i++)
		{
			const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[i];
			const TextureAsset &textureAsset = keyframe.textureAsset;
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
			if (!textureBuilderID.has_value())
			{
				DebugLogWarning("Couldn't load entity anim texture \"" + textureAsset.filename + "\".");
				continue;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID textureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &textureID))
			{
				DebugLogWarning("Couldn't create entity anim texture \"" + textureAsset.filename + "\".");
				continue;
			}

			ScopedObjectTextureRef textureRef(textureID, renderer);
			animTextureRefs.set(i, std::move(textureRef));
		}

		return animTextureRefs;
	}
}

const EntityDefinition &EntityChunkManager::getEntityDef(EntityDefID defID, const EntityDefinitionLibrary &defLibrary) const
{
	const auto iter = this->entityDefs.find(defID);
	if (iter != this->entityDefs.end())
	{
		return iter->second;
	}
	else
	{
		return defLibrary.getDefinition(defID);
	}
}

EntityDefID EntityChunkManager::addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &defLibrary)
{
	const int libraryDefCount = defLibrary.getDefinitionCount();
	const EntityDefID defID = static_cast<EntityDefID>(libraryDefCount + this->entityDefs.size());
	this->entityDefs.emplace(defID, std::move(def));
	return defID;
}

EntityDefID EntityChunkManager::getOrAddEntityDefID(const EntityDefinition &def, const EntityDefinitionLibrary &defLibrary)
{
	for (const auto &pair : this->entityDefs)
	{
		const EntityDefID currentDefID = pair.first;
		const EntityDefinition &currentDef = pair.second;
		if (currentDef == def) // There doesn't seem to be a better way than value comparisons.
		{
			return currentDefID;
		}
	}

	return this->addEntityDef(EntityDefinition(def), defLibrary);
}

EntityInstanceID EntityChunkManager::spawnEntity()
{
	EntityInstanceID instID;
	if (!this->entities.tryAlloc(&instID))
	{
		DebugCrash("Couldn't allocate EntityInstanceID.");
	}

	return instID;
}

void EntityChunkManager::populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &chunk,
	const LevelDefinition &levelDefinition, const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	const double ceilingScale = levelInfoDefinition.getCeilingScale();

	for (int i = 0; i < levelDefinition.getEntityPlacementDefCount(); i++)
	{
		const LevelDefinition::EntityPlacementDef &placementDef = levelDefinition.getEntityPlacementDef(i);
		const LevelDefinition::EntityDefID levelEntityDefID = placementDef.id;
		const EntityDefinition &entityDef = levelInfoDefinition.getEntityDef(levelEntityDefID);
		const EntityDefinition::Type entityDefType = entityDef.getType();
		const EntityType entityType = EntityUtils::getEntityTypeFromDefType(entityDefType);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		const std::string &defaultAnimStateName = EntityGeneration::getDefaultAnimationStateName(entityDef, entityGenInfo);
		const std::optional<int> defaultAnimStateIndex = animDef.tryGetStateIndex(defaultAnimStateName.c_str());
		if (!defaultAnimStateIndex.has_value())
		{
			DebugLogWarning("Couldn't get default anim state index for entity.");
			continue;
		}

		std::optional<EntityDefID> entityDefID; // Global entity def ID (shared across all active chunks).
		for (const LevelDouble3 &position : placementDef.positions)
		{
			const LevelInt3 voxelPosition = VoxelUtils::pointToVoxel(position, ceilingScale);
			if (ChunkUtils::IsInWritingRange(voxelPosition, startX, endX, startY, endY, startZ, endZ))
			{
				if (!entityDefID.has_value())
				{
					entityDefID = this->getOrAddEntityDefID(entityDef, entityDefLibrary);
				}

				const VoxelDouble3 point = ChunkUtils::MakeChunkPointFromLevel(position, startX, startY, startZ);
				EntityInstanceID entityInstID = this->spawnEntity();
				EntityInstance &entityInst = this->entities.get(entityInstID);

				EntityPositionID positionID;
				if (!this->positions.tryAlloc(&positionID))
				{
					DebugCrash("Couldn't allocate EntityPositionID.");
				}

				entityInst.init(entityInstID, *entityDefID, positionID);

				CoordDouble2 &entityCoord = this->positions.get(positionID);
				entityCoord.chunk = chunk.getPosition();
				entityCoord.point = VoxelDouble2(point.x, point.z);

				if (entityType == EntityType::Dynamic) // Dynamic entities have a direction.
				{
					if (!this->directions.tryAlloc(&entityInst.directionID))
					{
						DebugCrash("Couldn't allocate EntityDirectionID.");
					}

					VoxelDouble2 &entityDir = this->directions.get(entityInst.directionID);
					entityDir = CardinalDirection::North;

					if (entityDefType == EntityDefinition::Type::Enemy)
					{
						if (!this->creatureSoundInsts.tryAlloc(&entityInst.creatureSoundInstID))
						{
							DebugCrash("Couldn't allocate EntityCreatureSoundInstanceID.");
						}

						double &secondsTillCreatureSound = this->creatureSoundInsts.get(entityInst.creatureSoundInstID);
						secondsTillCreatureSound = DynamicEntity::nextCreatureSoundWaitTime(random);
					}
				}

				if (!this->animInsts.tryAlloc(&entityInst.animInstID))
				{
					DebugCrash("Couldn't allocate EntityAnimationInstanceID.");
				}

				EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);

				// Populate anim inst states now so the def doesn't need to be provided later.
				for (int animDefStateIndex = 0; animDefStateIndex < animDef.stateCount; animDefStateIndex++)
				{
					const EntityAnimationDefinitionState &animDefState = animDef.states[animDefStateIndex];
					animInst.addState(animDefState.seconds, animDefState.isLooping);
				}

				const EntityAnimationDefinitionState &defaultAnimDefState = animDef.states[*defaultAnimStateIndex];
				animInst.setStateIndex(*defaultAnimStateIndex);

				entityChunk.entityIDs.emplace_back(entityInstID);
			}
		}
	}

	if (citizenGenInfo.has_value())
	{
		DebugLogError("Not implemented: citizen spawning");

		// Spawn citizens if the total active limit has not been reached.
		/*const int currentCitizenCount = CitizenUtils::getCitizenCount(entityManager);
		const int remainingCitizensToSpawn = std::min(
			CitizenUtils::MAX_ACTIVE_CITIZENS - currentCitizenCount, CitizenUtils::CITIZENS_PER_CHUNK);

		for (int i = 0; i < remainingCitizensToSpawn; i++)
		{
			if (!CitizenUtils::trySpawnCitizenInChunk(chunk, *citizenGenInfo, random, binaryAssetLibrary, textureManager, entityManager))
			{
				DebugLogWarning("Couldn't spawn citizen in chunk \"" + chunk.getPosition().toString() + "\".");
			}
		}*/
	}
}

void EntityChunkManager::populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = entityChunk.getPosition();

	// Populate all or part of the chunk from a level definition depending on the map type.
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		DebugAssert(activeLevelIndex.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(*activeLevelIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(*activeLevelIndex);

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			DebugAssert(!citizenGenInfo.has_value());
			this->populateChunkEntities(entityChunk, voxelChunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, textureManager, renderer);
		}
	}
	else if (mapType == MapType::City)
	{
		DebugAssert(activeLevelIndex.has_value() && (*activeLevelIndex == 0));
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(0);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			DebugAssert(citizenGenInfo.has_value());
			this->populateChunkEntities(entityChunk, voxelChunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, textureManager, renderer);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		// The wilderness doesn't have an active level index since it's always just the one level.
		DebugAssert(!activeLevelIndex.has_value() || (*activeLevelIndex == 0));

		const MapDefinition::Wild &mapDefWild = mapDefinition.getWild();
		const int levelDefIndex = mapDefWild.getLevelDefIndex(chunkPos);
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(levelDefIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(levelDefIndex);

		// Copy level definition directly into chunk.
		DebugAssert(levelDefinition.getWidth() == Chunk::WIDTH);
		DebugAssert(levelDefinition.getDepth() == Chunk::DEPTH);
		const LevelInt2 levelOffset = LevelInt2::Zero;

		DebugAssert(citizenGenInfo.has_value());
		this->populateChunkEntities(entityChunk, voxelChunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
			citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, textureManager, renderer);
	}
}

std::string EntityChunkManager::getCreatureSoundFilename(const EntityDefID defID, const EntityDefinitionLibrary &entityDefLibrary) const
{
	const EntityDefinition &entityDef = this->getEntityDef(defID, entityDefLibrary);
	if (entityDef.getType() != EntityDefinition::Type::Enemy)
	{
		return std::string();
	}

	const auto &enemyDef = entityDef.getEnemy();
	if (enemyDef.getType() != EntityDefinition::EnemyDefinition::Type::Creature)
	{
		return std::string();
	}

	const auto &creatureDef = enemyDef.getCreature();
	const std::string_view creatureSoundName = creatureDef.soundName;
	return String::toUppercase(std::string(creatureSoundName));
}

const EntityInstance &EntityChunkManager::getEntity(EntityInstanceID id) const
{
	return this->entities.get(id);
}

const CoordDouble2 &EntityChunkManager::getEntityPosition(EntityPositionID id) const
{
	return this->positions.get(id);
}

const VoxelDouble2 &EntityChunkManager::getEntityDirection(EntityDirectionID id) const
{
	return this->directions.get(id);
}

const EntityAnimationInstance &EntityChunkManager::getEntityAnimationInstance(EntityAnimationInstanceID id) const
{
	return this->animInsts.get(id);
}

const Palette &EntityChunkManager::getEntityPalette(EntityPaletteInstanceID id) const
{
	return this->palettes.get(id);
}

int EntityChunkManager::getCountInChunkWithDirection(const ChunkInt2 &chunkPos) const
{
	int count = 0;
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunkPos);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Missing chunk (" + chunkPos.toString() + ") for counting entities with direction.");
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(*chunkIndex);
	for (const EntityInstanceID entityInstID : chunk.entityIDs)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		if (entityInst.directionID >= 0)
		{
			count++;
		}
	}

	return count;
}

int EntityChunkManager::getCountInChunkWithCreatureSound(const ChunkInt2 &chunkPos) const
{
	int count = 0;
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunkPos);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Missing chunk (" + chunkPos.toString() + ") for counting entities with creature sound.");
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(*chunkIndex);
	for (const EntityInstanceID entityInstID : chunk.entityIDs)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		if (entityInst.creatureSoundInstID >= 0)
		{
			count++;
		}
	}

	return count;
}

int EntityChunkManager::getCountInChunkWithPalette(const ChunkInt2 &chunkPos) const
{
	int count = 0;
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunkPos);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Missing chunk (" + chunkPos.toString() + ") for counting entities with palette.");
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(*chunkIndex);
	for (const EntityInstanceID entityInstID : chunk.entityIDs)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		if (entityInst.paletteInstID >= 0)
		{
			count++;
		}
	}

	return count;
}

void EntityChunkManager::getEntityVisibilityState2D(EntityInstanceID id, const CoordDouble2 &eye2D,
	const EntityDefinitionLibrary &entityDefLibrary, EntityVisibilityState2D &outVisState) const
{
	const EntityInstance &entityInst = this->entities.get(id);
	const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID, entityDefLibrary);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
	const EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
	
	const CoordDouble2 &entityCoord = this->positions.get(entityInst.positionID);
	const bool isDynamic = entityInst.isDynamic();

	// Get active animation state.
	const int stateIndex = animInst.currentStateIndex;
	DebugAssert(stateIndex >= 0);
	DebugAssert(stateIndex < animDef.stateCount);
	const EntityAnimationDefinitionState &animDefState = animDef.states[stateIndex];

	// Get animation angle based on entity direction relative to some camera/eye.
	const int angleCount = animDefState.keyframeListCount;
	const Radians animAngle = [this, &eye2D, &entityInst, &entityCoord, isDynamic, angleCount]()
	{
		if (!isDynamic)
		{
			// Static entities always face the camera.
			return 0.0;
		}
		else
		{
			const VoxelDouble2 &entityDir = this->getEntityDirection(entityInst.directionID);

			// Dynamic entities are angle-dependent.
			const VoxelDouble2 diffDir = (eye2D - entityCoord).normalized();

			const Radians entityAngle = MathUtils::fullAtan2(entityDir);
			const Radians diffAngle = MathUtils::fullAtan2(diffDir);

			// Use the difference of the two angles to get the relative angle.
			const Radians resultAngle = Constants::TwoPi + (entityAngle - diffAngle);

			// Angle bias so the final direction is centered within its angle range.
			const Radians angleBias = (Constants::TwoPi / static_cast<double>(angleCount)) * 0.50;

			return std::fmod(resultAngle + angleBias, Constants::TwoPi);
		}
	}();

	// Index into animation keyframe lists for the state.
	const int angleIndex = [angleCount, animAngle]()
	{
		const double angleCountReal = static_cast<double>(angleCount);
		const double anglePercent = animAngle / Constants::TwoPi;
		const int angleIndex = static_cast<int>(angleCountReal * anglePercent);
		return std::clamp(angleIndex, 0, angleCount - 1);
	}();

	// Keyframe list for the current state and angle.
	const int animDefKeyframeListIndex = animDefState.keyframeListsIndex + angleIndex;
	DebugAssert(animDefKeyframeListIndex >= 0);
	DebugAssert(animDefKeyframeListIndex < animDef.keyframeListCount);
	const EntityAnimationDefinitionKeyframeList &animDefKeyframeList = animDef.keyframeLists[animDefKeyframeListIndex];

	// Progress through current animation.
	const int keyframeIndex = [&animInst, &animDefState, &animDefKeyframeList]()
	{
		const int keyframeCount = animDefKeyframeList.keyframeCount;
		const double keyframeCountReal = static_cast<double>(keyframeCount);
		const double animPercent = animInst.progressPercent;
		const int keyframeIndex = static_cast<int>(keyframeCountReal * animPercent);
		return std::clamp(keyframeIndex, 0, keyframeCount - 1);
	}();

	const CoordDouble2 flatPosition(
		entityCoord.chunk,
		VoxelDouble2(entityCoord.point.x, entityCoord.point.y));

	outVisState.init(id, flatPosition, stateIndex, angleIndex, keyframeIndex);
}

void EntityChunkManager::getEntityVisibilityState3D(EntityInstanceID id, const CoordDouble2 &eye2D,
	double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
	EntityVisibilityState3D &outVisState) const
{
	EntityVisibilityState2D visState2D;
	this->getEntityVisibilityState2D(id, eye2D, entityDefLibrary, visState2D);

	const EntityInstance &entityInst = this->entities.get(id);
	const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID, entityDefLibrary);
	const int baseYOffset = EntityUtils::getYOffset(entityDef);
	const double flatYOffset = static_cast<double>(-baseYOffset) / MIFUtils::ARENA_UNITS;

	// If the entity is in a raised platform voxel, they are set on top of it.
	const double raisedPlatformYOffset = [ceilingScale, &voxelChunkManager, &visState2D]()
	{
		const CoordInt2 entityVoxelCoord(
			visState2D.flatPosition.chunk,
			VoxelUtils::pointToVoxel(visState2D.flatPosition.point));
		const VoxelChunk *chunk = voxelChunkManager.tryGetChunkAtPosition(entityVoxelCoord.chunk);
		if (chunk == nullptr)
		{
			// Not sure this is ever reachable, but handle just in case.
			return 0.0;
		}

		const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk->getTraitsDefID(entityVoxelCoord.voxel.x, 1, entityVoxelCoord.voxel.y);
		const VoxelTraitsDefinition &voxelTraitsDef = chunk->getTraitsDef(voxelTraitsDefID);

		if (voxelTraitsDef.type == ArenaTypes::VoxelType::Raised)
		{
			const VoxelTraitsDefinition::Raised &raised = voxelTraitsDef.raised;
			return (raised.yOffset + raised.ySize) * ceilingScale;
		}
		else
		{
			// No raised platform offset.
			return 0.0;
		}
	}();

	// Bottom center of flat.
	const VoxelDouble3 flatPoint(
		visState2D.flatPosition.point.x,
		ceilingScale + flatYOffset + raisedPlatformYOffset,
		visState2D.flatPosition.point.y);
	const CoordDouble3 flatPosition(visState2D.flatPosition.chunk, flatPoint);

	outVisState.init(id, flatPosition, visState2D.stateIndex, visState2D.angleIndex, visState2D.keyframeIndex);
}

void EntityChunkManager::updateCreatureSounds(double dt, EntityChunk &entityChunk, const CoordDouble3 &playerCoord,
	double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary, AudioManager &audioManager)
{
	const int entityCount = static_cast<int>(entityChunk.entityIDs.size());
	for (int i = 0; i < entityCount; i++)
	{
		const EntityInstanceID instID = entityChunk.entityIDs[i];
		EntityInstance &entityInst = this->entities.get(instID);
		if (entityInst.creatureSoundInstID >= 0)
		{
			double &secondsTillCreatureSound = this->creatureSoundInsts.get(entityInst.creatureSoundInstID);
			secondsTillCreatureSound -= dt;
			if (secondsTillCreatureSound <= 0.0)
			{
				const CoordDouble2 &entityCoord = this->positions.get(entityInst.positionID);
				if (EntityUtils::withinHearingDistance(playerCoord, entityCoord, ceilingScale))
				{
					// @todo: store some kind of sound def ID w/ the secondsTillCreatureSound instead of generating the sound filename here.
					const std::string creatureSoundFilename = this->getCreatureSoundFilename(entityInst.defID, entityDefLibrary);
					if (creatureSoundFilename.empty())
					{
						continue;
					}

					// Center the sound inside the creature.
					const CoordDouble3 soundCoord(
						entityCoord.chunk,
						VoxelDouble3(entityCoord.point.x, ceilingScale * 1.50, entityCoord.point.y));
					const WorldDouble3 absoluteSoundPosition = VoxelUtils::coordToWorldPoint(soundCoord);
					audioManager.playSound(creatureSoundFilename, absoluteSoundPosition);

					secondsTillCreatureSound = DynamicEntity::nextCreatureSoundWaitTime(random);
				}
			}
		}
	}
}

void EntityChunkManager::update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
	const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, AudioManager &audioManager, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < freedChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = freedChunkPositions.get(i);
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		EntityChunk &entityChunk = this->getChunkAtIndex(spawnIndex);
		entityChunk.init(chunkPos, voxelChunk.getHeight());

		this->populateChunk(entityChunk, voxelChunk, activeLevelIndex, mapDefinition, entityGenInfo, citizenGenInfo,
			ceilingScale, random, entityDefLibrary, binaryAssetLibrary, textureManager, renderer);
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	for (int i = 0; i < activeChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = activeChunkPositions.get(i);
		const int chunkIndex = this->getChunkIndex(chunkPos);
		EntityChunk &entityChunk = this->getChunkAtIndex(chunkIndex);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		// @todo: simulate/animate AI

		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = this->entities.get(entityInstID);
			EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
			animInst.update(dt);
		}

		this->updateCreatureSounds(dt, entityChunk, playerCoord, ceilingScale, random, entityDefLibrary, audioManager);

		// @todo: citizen spawning and management by player distance

		// @todo: rebuild entity chunk draw calls
		//this->rebuildVoxelChunkDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, updateStatics, true);
	}
}
