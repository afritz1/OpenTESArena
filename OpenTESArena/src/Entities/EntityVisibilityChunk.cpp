#include <algorithm>

#include "EntityChunk.h"
#include "EntityChunkManager.h"
#include "EntityVisibilityChunk.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RendererUtils.h"
#include "../Voxels/VoxelChunkManager.h"

void EntityVisibilityChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
}

void EntityVisibilityChunk::update(const RenderCamera &camera, double ceilingScale, const EntityChunk &entityChunk,
	const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager)
{
	this->bbox.clear();
	this->entityWorldBBoxCache.clear();
	this->visibleEntities.clear();

	this->entityWorldBBoxCache.reserve(entityChunk.entityIDs.size());

	const CoordDouble2 cameraCoordXZ(camera.chunk, VoxelDouble2(camera.chunkPoint.x, camera.chunkPoint.z));
	const WorldDouble2 cameraWorldPointXZ = VoxelUtils::coordToWorldPoint(cameraCoordXZ);

	// Have to bootstrap with the first entity's bounding box.
	bool hasBBoxExpandedAny = false;

	// Expand the chunk's bounding box to fit all entities in it.
	for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const CoordDouble2 &entityCoord = entityChunkManager.getEntityPosition(entityInst.positionID);
		const WorldDouble2 entityWorldPosXZ = VoxelUtils::coordToWorldPoint(entityCoord);
		const double entityYPosition = entityChunkManager.getEntityCorrectedY(entityInstID, ceilingScale, voxelChunkManager);

		// Entity's bounding box is in model space centered on them.
		const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);

		const WorldDouble3 entityWorldBBoxMin(
			entityWorldPosXZ.x + entityBBox.min.x,
			entityYPosition + entityBBox.min.y,
			entityWorldPosXZ.y + entityBBox.min.z);
		const WorldDouble3 entityWorldBBoxMax(
			entityWorldPosXZ.x + entityBBox.max.x,
			entityYPosition + entityBBox.max.y,
			entityWorldPosXZ.y + entityBBox.max.z);

		BoundingBox3D entityWorldBBox;
		entityWorldBBox.init(entityWorldBBoxMin, entityWorldBBoxMax);

		if (!hasBBoxExpandedAny)
		{
			hasBBoxExpandedAny = true;
			this->bbox = entityWorldBBox;
		}
		else
		{
			this->bbox.expandToInclude(entityWorldBBox);
		}

		this->entityWorldBBoxCache.emplace_back(std::move(entityWorldBBox));
	}

	if (!hasBBoxExpandedAny)
	{
		// No entities in chunk.
		return;
	}

	bool isBBoxCompletelyVisible, isBBoxCompletelyInvisible;
	RendererUtils::getBBoxVisibilityInFrustum(this->bbox, camera, &isBBoxCompletelyVisible, &isBBoxCompletelyInvisible);

	if (isBBoxCompletelyInvisible)
	{
		// Can't see the root bounding box or any entities inside.
		return;
	}
	
	if (isBBoxCompletelyVisible)
	{
		// All entities are visible.
		this->visibleEntities.resize(entityChunk.entityIDs.size());
		std::copy(entityChunk.entityIDs.begin(), entityChunk.entityIDs.end(), this->visibleEntities.begin());
	}
	else
	{
		// Check each entity's bounding box for visibility.		
		for (int i = 0; i < static_cast<int>(entityChunk.entityIDs.size()); i++)
		{
			const EntityInstanceID entityInstID = entityChunk.entityIDs[i];
			const BoundingBox3D &entityWorldBBox = this->entityWorldBBoxCache[i];

			bool isEntityBBoxCompletelyVisible, isEntityBBoxCompletelyInvisible;
			RendererUtils::getBBoxVisibilityInFrustum(entityWorldBBox, camera, &isEntityBBoxCompletelyVisible, &isEntityBBoxCompletelyInvisible);

			if (!isEntityBBoxCompletelyInvisible)
			{
				this->visibleEntities.emplace_back(entityInstID);
			}
		}
	}

	// Sort entities far to near.
	std::sort(this->visibleEntities.begin(), this->visibleEntities.end(),
		[&entityChunkManager, &cameraWorldPointXZ](const EntityInstanceID idA, const EntityInstanceID idB)
	{
		const EntityInstance &entityInstA = entityChunkManager.getEntity(idA);
		const EntityInstance &entityInstB = entityChunkManager.getEntity(idB);
		const CoordDouble2 &entityCoordA = entityChunkManager.getEntityPosition(entityInstA.positionID);
		const CoordDouble2 &entityCoordB = entityChunkManager.getEntityPosition(entityInstB.positionID);
		const WorldDouble2 entityWorldPosA = VoxelUtils::coordToWorldPoint(entityCoordA);
		const WorldDouble2 entityWorldPosB = VoxelUtils::coordToWorldPoint(entityCoordB);
		const double entityDistSqrA = (entityWorldPosA - cameraWorldPointXZ).lengthSquared();
		const double entityDistSqrB = (entityWorldPosB - cameraWorldPointXZ).lengthSquared();
		return entityDistSqrA > entityDistSqrB;
	});
}

void EntityVisibilityChunk::clear()
{
	Chunk::clear();
	this->bbox.clear();
	this->entityWorldBBoxCache.clear();
	this->visibleEntities.clear();
}
