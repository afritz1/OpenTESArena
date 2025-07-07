#include <algorithm>

#include "CombatLogic.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Math/BoundingBox.h"
#include "../Voxels/VoxelChunkManager.h"

CombatHitSearchResult::CombatHitSearchResult()
{
	this->voxelCount = 0;
	std::fill(std::begin(this->entities), std::end(this->entities), -1);
	this->entityCount = 0;
}

Span<const WorldInt3> CombatHitSearchResult::getVoxels() const
{
	return Span<const WorldInt3>(this->voxels, this->voxelCount);
}

Span<const EntityInstanceID> CombatHitSearchResult::getEntities() const
{
	return Span<const EntityInstanceID>(this->entities, this->entityCount);
}

void CombatLogic::getHitSearchResult(const WorldDouble3 &searchPoint, double searchRadius, double ceilingScale,
	const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager, CombatHitSearchResult *outHitSearchResult)
{
	const double searchDim = searchRadius * 2.0;
	BoundingBox3D searchBBox;
	searchBBox.init(searchPoint, searchDim, searchDim, searchDim);

	const WorldDouble3 searchMinWorldPoint = searchBBox.min;
	const WorldDouble3 searchMaxWorldPoint = searchBBox.max;
	const WorldInt3 searchMinWorldVoxel = VoxelUtils::pointToVoxel(searchMinWorldPoint, ceilingScale);
	const WorldInt3 searchMaxWorldVoxel = VoxelUtils::pointToVoxel(searchMaxWorldPoint, ceilingScale);

	for (WEInt z = searchMinWorldVoxel.z; z <= searchMaxWorldVoxel.z; z++)
	{
		for (int y = searchMinWorldVoxel.y; y <= searchMaxWorldVoxel.y; y++)
		{
			for (SNInt x = searchMinWorldVoxel.x; x <= searchMaxWorldVoxel.x; x++)
			{
				// Only want voxels in gameplay.
				const WorldInt3 searchWorldVoxel(x, y, z);
				const CoordInt3 searchVoxelCoord = VoxelUtils::worldVoxelToCoord(searchWorldVoxel);
				const VoxelInt3 searchVoxel = searchVoxelCoord.voxel;
				const VoxelChunk *voxelChunk = voxelChunkManager.findChunkAtPosition(searchVoxelCoord.chunk);
				if ((voxelChunk == nullptr) || !voxelChunk->isValidVoxel(searchVoxel.x, searchVoxel.y, searchVoxel.z))
				{
					continue;
				}

				if (outHitSearchResult->voxelCount == CombatHitSearchResult::MAX_HIT_COUNT)
				{
					break;
				}

				outHitSearchResult->voxels[outHitSearchResult->voxelCount] = WorldInt3(x, y, z);
				outHitSearchResult->voxelCount++;
			}
		}
	}

	const ChunkInt2 searchMinChunk = VoxelUtils::worldVoxelToChunk(searchMinWorldVoxel);
	const ChunkInt2 searchMaxChunk = VoxelUtils::worldVoxelToChunk(searchMaxWorldVoxel);

	for (WEInt chunkZ = searchMinChunk.y; chunkZ <= searchMaxChunk.y; chunkZ++)
	{
		for (SNInt chunkX = searchMinChunk.x; chunkX <= searchMaxChunk.x; chunkX++)
		{
			const ChunkInt2 searchChunkPos(chunkX, chunkZ);
			const EntityChunk *entityChunk = entityChunkManager.findChunkAtPosition(searchChunkPos);
			if (entityChunk != nullptr)
			{
				for (const EntityInstanceID entityInstID : entityChunk->entityIDs)
				{
					const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
					if (!entityInst.canAcceptCombatHits())
					{
						continue;
					}

					const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInst.positionID);
					const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
					const WorldDouble3 entityWorldBBoxMin = entityPosition + entityBBox.min;
					const WorldDouble3 entityWorldBBoxMax = entityPosition + entityBBox.max;
					BoundingBox3D entityWorldBBox;
					entityWorldBBox.init(entityWorldBBoxMin, entityWorldBBoxMax);

					if (searchBBox.intersects(entityWorldBBox))
					{
						if (outHitSearchResult->entityCount == CombatHitSearchResult::MAX_HIT_COUNT)
						{
							break;
						}

						outHitSearchResult->entities[outHitSearchResult->entityCount] = entityInstID;
						outHitSearchResult->entityCount++;
					}
				}
			}
		}
	}
}

void CombatLogic::spawnHitVfx(const EntityDefinition &hitEntityDef, const WorldDouble3 &position, EntityChunkManager &entityChunkManager,
	Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();

	constexpr int FirstBloodIndex = 24; // Based on original game VFX array, blood is indices 24-26.
	EntityDefinitionKey key;
	int bloodIndex = FirstBloodIndex;
	if (hitEntityDef.type == EntityDefinitionType::Enemy && hitEntityDef.enemy.type == EnemyEntityDefinitionType::Creature)
	{
		bloodIndex = hitEntityDef.enemy.creature.bloodIndex;
	}

	bloodIndex -= FirstBloodIndex;
	key.initVfx(VfxEntityAnimationType::MeleeStrike, bloodIndex);

	EntityDefID hitEntityVfxEntityDefID;
	if (!entityDefLibrary.tryGetDefinitionID(key, &hitEntityVfxEntityDefID))
	{
		DebugCrash("Couldn't get hit entity VFX definition ID from library.");
	}

	EntityInitInfo initInfo;
	initInfo.defID = hitEntityVfxEntityDefID;
	initInfo.feetPosition = position;
	initInfo.initialAnimStateIndex = 0; // Assuming VFX only have one state.
	initInfo.isSensorCollider = true;
	entityChunkManager.createEntity(initInfo, random, physicsSystem, renderer);
}
