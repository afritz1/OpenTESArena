#include "ArenaRenderUtils.h"
#include "Renderer.h"
#include "RenderLightChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Voxels/VoxelChunkManager.h"

#include "components/debug/Debug.h"

RenderLightChunkManager::Light::Light()
{
	this->lightID = -1;
	this->enabled = false;
}

void RenderLightChunkManager::Light::init(RenderLightID lightID, bool enabled)
{
	this->lightID = lightID;
	this->enabled = enabled;
}

RenderLightChunkManager::RenderLightChunkManager()
{
	this->playerLightID = -1;
}

void RenderLightChunkManager::init(Renderer &renderer)
{
	// Populate global lights.
	if (!renderer.tryCreateLight(&this->playerLightID))
	{
		DebugLogError("Couldn't create render light ID for player.");
		return;
	}

	renderer.setLightRadius(this->playerLightID, ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS, ArenaRenderUtils::PLAYER_LIGHT_END_RADIUS);
}

void RenderLightChunkManager::shutdown(Renderer &renderer)
{
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		this->recycleChunk(i);
	}

	if (this->playerLightID >= 0)
	{
		renderer.freeLight(this->playerLightID);
		this->playerLightID = -1;
	}

	for (auto &pair : this->entityLights)
	{
		Light &light = pair.second;
		if (light.lightID >= 0)
		{
			renderer.freeLight(light.lightID);
			light.lightID = -1;
		}
	}

	this->entityLights.clear();
}

void RenderLightChunkManager::updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager, Renderer &renderer)
{
	for (const ChunkInt2 &chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		RenderLightChunk &renderChunk = this->getChunkAtIndex(chunkIndex);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		RenderLightChunk &renderChunk = this->getChunkAtIndex(spawnIndex);
		renderChunk.init(chunkPos, voxelChunk.getHeight());
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();
}

void RenderLightChunkManager::update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
	const CoordDouble3 &cameraCoord, double ceilingScale, bool isFogActive, bool nightLightsAreActive, bool playerHasLight,
	const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunkManager.getQueuedDestroyEntityIDs())
	{
		const auto iter = this->entityLights.find(entityInstID);
		if (iter != this->entityLights.end())
		{
			const Light &light = iter->second;
			renderer.freeLight(light.lightID);
			this->entityLights.erase(iter);
		}
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const std::optional<double> entityLightRadius = EntityUtils::tryGetLightRadius(entityDef);
			const bool isLight = entityLightRadius.has_value();
			if (isLight)
			{
				RenderLightID lightID;
				if (!renderer.tryCreateLight(&lightID))
				{
					DebugLogError("Couldn't allocate render light ID in chunk (" + chunkPos.toString() + ").");
					continue;
				}

				const bool isLightEnabled = !EntityUtils::isStreetlight(entityDef) || nightLightsAreActive;

				Light light;
				light.init(lightID, isLightEnabled);
				this->entityLights.emplace(entityInstID, std::move(light));

				const CoordDouble3 entityCoord3D = entityChunkManager.getEntityPosition3D(entityInstID, ceilingScale, voxelChunkManager);
				const WorldDouble3 entityWorldPos = VoxelUtils::coordToWorldPoint(entityCoord3D);

				const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
				const double entityCenterYPosition = entityWorldPos.y + (entityBBox.height * 0.50);
				const WorldDouble3 entityLightWorldPos(entityWorldPos.x, entityCenterYPosition, entityWorldPos.z);
				renderer.setLightPosition(lightID, entityLightWorldPos);
				renderer.setLightRadius(lightID, ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS, *entityLightRadius);
			}
		}
	}

	const WorldDouble3 prevPlayerLightPosition = renderer.getLightPosition(this->playerLightID);
	const WorldDouble3 playerLightPosition = VoxelUtils::coordToWorldPoint(cameraCoord);
	renderer.setLightPosition(this->playerLightID, playerLightPosition);

	double playerLightRadiusStart, playerLightRadiusEnd;
	if (isFogActive)
	{
		playerLightRadiusStart = ArenaRenderUtils::PLAYER_FOG_LIGHT_START_RADIUS;
		playerLightRadiusEnd = ArenaRenderUtils::PLAYER_FOG_LIGHT_END_RADIUS;
	}
	else
	{
		playerLightRadiusStart = ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS;
		playerLightRadiusEnd = ArenaRenderUtils::PLAYER_LIGHT_END_RADIUS;
	}

	if (!playerHasLight)
	{
		playerLightRadiusStart = 0.0;
		playerLightRadiusEnd = 0.0;
	}

	renderer.setLightRadius(this->playerLightID, playerLightRadiusStart, playerLightRadiusEnd);

	// Clear all previous light references in voxels.
	// @todo: track touched, newly-touched, and no-longer-touched voxels each frame for moving lights so we don't have to iterate all voxels
	for (int i = 0; i < this->getChunkCount(); i++)
	{
		RenderLightChunk &renderChunk = this->getChunkAtIndex(i);
		for (RenderVoxelLightIdList &voxelLightIdList : renderChunk.voxelLightIdLists)
		{
			voxelLightIdList.clear();
		}
	}

	auto getLightMinAndMaxVoxels = [ceilingScale](const WorldDouble3 &lightPosition, double endRadius, WorldInt3 *outMin, WorldInt3 *outMax)
	{
		const WorldDouble3 lightPosMin = lightPosition - WorldDouble3(endRadius, endRadius, endRadius);
		const WorldDouble3 lightPosMax = lightPosition + WorldDouble3(endRadius, endRadius, endRadius);
		*outMin = VoxelUtils::pointToVoxel(lightPosMin, ceilingScale);
		*outMax = VoxelUtils::pointToVoxel(lightPosMax, ceilingScale);
	};

	auto populateTouchedVoxelLightIdLists = [this, &renderer, ceilingScale](RenderLightID lightID, const WorldDouble3 &lightPosition,
		const WorldInt3 &lightVoxelMin, const WorldInt3 &lightVoxelMax)
	{
		// Iterate over all voxels the light's bounding box touches.
		for (WEInt z = lightVoxelMin.z; z <= lightVoxelMax.z; z++)
		{
			for (int y = lightVoxelMin.y; y <= lightVoxelMax.y; y++)
			{
				for (SNInt x = lightVoxelMin.x; x <= lightVoxelMax.x; x++)
				{
					const CoordInt3 curLightCoord = VoxelUtils::worldVoxelToCoord(WorldInt3(x, y, z));
					RenderLightChunk *renderChunkPtr = this->tryGetChunkAtPosition(curLightCoord.chunk);
					if (renderChunkPtr != nullptr)
					{
						const VoxelInt3 &curLightVoxel = curLightCoord.voxel;
						if (renderChunkPtr->isValidVoxel(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z))
						{
							RenderVoxelLightIdList &voxelLightIdList = renderChunkPtr->voxelLightIdLists.get(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z);
							voxelLightIdList.tryAddLight(lightID);
						}
					}
				}
			}
		}
	};

	WorldInt3 playerLightVoxelMin, playerLightVoxelMax;
	getLightMinAndMaxVoxels(playerLightPosition, playerLightRadiusEnd, &playerLightVoxelMin, &playerLightVoxelMax);
	populateTouchedVoxelLightIdLists(this->playerLightID, playerLightPosition, playerLightVoxelMin, playerLightVoxelMax);

	// Populate each voxel's light ID list based on which lights touch them, preferring the nearest lights.
	// - @todo: this method doesn't implicitly allow sorting by distance because it doesn't check lights per voxel, it checks voxels per light.
	//   If sorting is desired then do it after this loop. It should also sort by intensity at the voxel center, not just distance to the light.
	for (const auto &pair : this->entityLights)
	{
		const Light &light = pair.second;
		if (light.enabled)
		{
			const RenderLightID lightID = light.lightID;
			const WorldDouble3 &lightPosition = renderer.getLightPosition(lightID);

			double dummyLightStartRadius, lightEndRadius;
			renderer.getLightRadii(lightID, &dummyLightStartRadius, &lightEndRadius);

			WorldInt3 lightVoxelMin, lightVoxelMax;
			getLightMinAndMaxVoxels(lightPosition, lightEndRadius, &lightVoxelMin, &lightVoxelMax);
			populateTouchedVoxelLightIdLists(lightID, lightPosition, lightVoxelMin, lightVoxelMax);
		}
	}

	WorldInt3 prevPlayerLightVoxelMin, prevPlayerLightVoxelMax;
	getLightMinAndMaxVoxels(prevPlayerLightPosition, playerLightRadiusEnd, &prevPlayerLightVoxelMin, &prevPlayerLightVoxelMax);

	// See which voxels affected by the player's light are getting their light references updated.
	// This is for dirty voxel draw calls mainly, not about setting light references (might change later).
	if ((prevPlayerLightVoxelMin != playerLightVoxelMin) || (prevPlayerLightVoxelMax != playerLightVoxelMax))
	{
		for (WEInt z = playerLightVoxelMin.z; z <= playerLightVoxelMax.z; z++)
		{
			for (int y = playerLightVoxelMin.y; y <= playerLightVoxelMax.y; y++)
			{
				for (SNInt x = playerLightVoxelMin.x; x <= playerLightVoxelMax.x; x++)
				{
					const bool isInPrevRange =
						(x >= prevPlayerLightVoxelMin.x) && (x <= prevPlayerLightVoxelMax.x) &&
						(y >= prevPlayerLightVoxelMin.y) && (y <= prevPlayerLightVoxelMax.y) &&
						(z >= prevPlayerLightVoxelMin.z) && (z <= prevPlayerLightVoxelMax.z);

					if (!isInPrevRange)
					{
						const CoordInt3 curLightCoord = VoxelUtils::worldVoxelToCoord(WorldInt3(x, y, z));
						RenderLightChunk *renderChunkPtr = this->tryGetChunkAtPosition(curLightCoord.chunk);
						if (renderChunkPtr != nullptr)
						{
							renderChunkPtr->addDirtyLightPosition(curLightCoord.voxel);
						}
					}
				}
			}
		}
	}
}

void RenderLightChunkManager::setNightLightsActive(bool enabled, const EntityChunkManager &entityChunkManager)
{
	// Update streetlight enabled states.
	for (ChunkPtr &chunkPtr : this->activeChunks)
	{
		const ChunkInt2 &chunkPos = chunkPtr->getPosition();
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			if (EntityUtils::isStreetlight(entityDef))
			{
				const auto lightIter = this->entityLights.find(entityInstID);
				DebugAssertMsg(lightIter != this->entityLights.end(), "Couldn't find light for streetlight entity \"" + std::to_string(entityInstID) + "\" in chunk (" + chunkPos.toString() + ").");

				Light &light = lightIter->second;
				light.enabled = enabled;
			}
		}
	}
}

void RenderLightChunkManager::cleanUp()
{
	for (ChunkPtr &chunkPtr : this->activeChunks)
	{
		chunkPtr->dirtyLightPositions.clear();
	}
}

void RenderLightChunkManager::unloadScene(Renderer &renderer)
{
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		this->recycleChunk(i);
	}

	for (auto &pair : this->entityLights)
	{
		Light &light = pair.second;
		if (light.lightID >= 0)
		{
			renderer.freeLight(light.lightID);
			light.lightID = -1;
		}
	}

	this->entityLights.clear();
}
