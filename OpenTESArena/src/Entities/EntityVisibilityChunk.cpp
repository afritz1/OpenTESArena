#include <algorithm>

#include "EntityChunk.h"
#include "EntityChunkManager.h"
#include "EntityVisibilityChunk.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RendererUtils.h"

void EntityVisibilityChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
}

void EntityVisibilityChunk::update(const RenderCamera &camera, double ceilingScale, const EntityChunk &entityChunk,
	const EntityChunkManager &entityChunkManager)
{
	this->bbox.clear();
	this->entityWorldBBoxCache.clear();
	this->visibleEntityEntries.clear();

	this->entityWorldBBoxCache.reserve(entityChunk.entityIDs.size());

	// Have to bootstrap with the first entity's bounding box.
	bool hasBBoxExpandedAny = false;

	// Expand the chunk's bounding box to fit all entities in it.
	for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInst.positionID);

		// Entity's bounding box is in model space centered on them.
		const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);

		const WorldDouble3 entityWorldBBoxMin(
			entityPosition.x + entityBBox.min.x,
			entityPosition.y + entityBBox.min.y,
			entityPosition.z + entityBBox.min.z);
		const WorldDouble3 entityWorldBBoxMax(
			entityPosition.x + entityBBox.max.x,
			entityPosition.y + entityBBox.max.y,
			entityPosition.z + entityBBox.max.z);

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
		this->visibleEntityEntries.resize(entityChunk.entityIDs.size());

		for (int i = 0; i < static_cast<int>(entityChunk.entityIDs.size()); i++)
		{
			const EntityInstanceID entityInstID = entityChunk.entityIDs[i];
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInst.positionID);
			
			VisibleEntityEntry &visibleEntityEntry = this->visibleEntityEntries[i];
			visibleEntityEntry.id = entityInstID;
			visibleEntityEntry.position = entityPosition;
		}
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
				const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
				const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInst.positionID);
				this->visibleEntityEntries.emplace_back(entityInstID, entityPosition);
			}
		}
	}

	const WorldDouble2 cameraWorldPointXZ = camera.worldPoint.getXZ();

	// Sort entities far to near.
	std::sort(this->visibleEntityEntries.begin(), this->visibleEntityEntries.end(),
		[&cameraWorldPointXZ](const VisibleEntityEntry &entryA, const VisibleEntityEntry &entryB)
	{
		const WorldDouble3 entityPositionA = entryA.position;
		const WorldDouble3 entityPositionB = entryB.position;
		const WorldDouble2 entityPositionXZA = entityPositionA.getXZ();
		const WorldDouble2 entityPositionXZB = entityPositionB.getXZ();
		const double entityDistSqrA = (entityPositionXZA - cameraWorldPointXZ).lengthSquared();
		const double entityDistSqrB = (entityPositionXZB - cameraWorldPointXZ).lengthSquared();
		return entityDistSqrA > entityDistSqrB;
	});
}

void EntityVisibilityChunk::clear()
{
	Chunk::clear();
	this->bbox.clear();
	this->entityWorldBBoxCache.clear();
	this->visibleEntityEntries.clear();
}
