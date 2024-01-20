#include "ArenaRenderUtils.h"
#include "Renderer.h"
#include "RenderLightChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Voxels/VoxelChunkManager.h"

#include "components/debug/Debug.h"

namespace
{
	void GetLightMinAndMaxVoxels(const WorldDouble3 &lightPosition, double endRadius, double ceilingScale, WorldInt3 *outMin, WorldInt3 *outMax)
	{
		const WorldDouble3 lightPosMin = lightPosition - WorldDouble3(endRadius, endRadius, endRadius);
		const WorldDouble3 lightPosMax = lightPosition + WorldDouble3(endRadius, endRadius, endRadius);
		*outMin = VoxelUtils::pointToVoxel(lightPosMin, ceilingScale);
		*outMax = VoxelUtils::pointToVoxel(lightPosMax, ceilingScale);
	}
}

RenderLightChunkManager::Light::Light()
{
	this->lightID = -1;
	this->startRadius = 0.0;
	this->endRadius = 0.0;
	this->enabled = false;
}

void RenderLightChunkManager::Light::init(RenderLightID lightID, const WorldDouble3 &point, double startRadius, double endRadius, bool enabled)
{
	this->lightID = lightID;
	this->point = point;
	this->minPoint = point - WorldDouble3(endRadius, endRadius, endRadius);
	this->maxPoint = point + WorldDouble3(endRadius, endRadius, endRadius);
	this->startRadius = startRadius;
	this->endRadius = endRadius;
	this->enabled = enabled;
}

void RenderLightChunkManager::Light::updatePosition(const WorldDouble3 &point, double ceilingScale, int chunkHeight)
{
	const WorldDouble3 oldPoint = this->point;
	const WorldDouble3 oldMinPoint = this->minPoint;
	const WorldDouble3 oldMaxPoint = this->maxPoint;
	this->point = point;
	this->minPoint = point - WorldDouble3(this->endRadius, this->endRadius, this->endRadius);
	this->maxPoint = point + WorldDouble3(this->endRadius, this->endRadius, this->endRadius);

	const WorldInt3 oldMinVoxel = VoxelUtils::pointToVoxel(oldMinPoint, ceilingScale);
	const WorldInt3 oldMaxVoxel = VoxelUtils::pointToVoxel(oldMaxPoint, ceilingScale);
	const WorldInt3 minVoxel = VoxelUtils::pointToVoxel(this->minPoint, ceilingScale);
	const WorldInt3 maxVoxel = VoxelUtils::pointToVoxel(this->maxPoint, ceilingScale);

	const int voxelYMax = chunkHeight - 1;
	const WorldInt3 clampedOldMinVoxel(oldMinVoxel.x, std::clamp(oldMinVoxel.y, 0, voxelYMax), oldMinVoxel.z);
	const WorldInt3 clampedOldMaxVoxel(oldMaxVoxel.x, std::clamp(oldMaxVoxel.y, 0, voxelYMax), oldMaxVoxel.z);
	const WorldInt3 clampedMinVoxel(minVoxel.x, std::clamp(minVoxel.y, 0, voxelYMax), minVoxel.z);
	const WorldInt3 clampedMaxVoxel(maxVoxel.x, std::clamp(maxVoxel.y, 0, voxelYMax), maxVoxel.z);

	const WorldInt3 componentMinVoxel(
		std::min(clampedOldMinVoxel.x, clampedMinVoxel.x),
		std::min(clampedOldMinVoxel.y, clampedMinVoxel.y),
		std::min(clampedOldMinVoxel.z, clampedMinVoxel.z));
	const WorldInt3 componentMaxVoxel(
		std::max(clampedOldMaxVoxel.x, clampedMaxVoxel.x),
		std::max(clampedOldMaxVoxel.y, clampedMaxVoxel.y),
		std::max(clampedOldMaxVoxel.z, clampedMaxVoxel.z));

	this->voxels.clear();

	// Add current voxels.
	for (WEInt z = clampedMinVoxel.z; z <= clampedMaxVoxel.z; z++)
	{
		for (int y = clampedMinVoxel.y; y <= clampedMaxVoxel.y; y++)
		{
			for (SNInt x = clampedMinVoxel.x; x <= clampedMaxVoxel.x; x++)
			{
				this->voxels.emplace_back(VoxelInt3(x, y, z));
			}
		}
	}

	this->addedVoxels.clear();
	this->removedVoxels.clear();

	// Determine which voxels are newly-touched and no-longer-touched.
	for (WEInt z = componentMinVoxel.z; z <= componentMaxVoxel.z; z++)
	{
		const bool shouldAddZ = (z < clampedOldMinVoxel.z) || (z > clampedOldMaxVoxel.z);
		const bool shouldRemoveZ = (z < clampedMinVoxel.z) || (z > clampedMaxVoxel.z);

		for (int y = componentMinVoxel.y; y <= componentMaxVoxel.y; y++)
		{
			const bool shouldAddY = (y < clampedOldMinVoxel.y) || (y > clampedOldMaxVoxel.y);
			const bool shouldRemoveY = (y < clampedMinVoxel.y) || (y > clampedMaxVoxel.y);

			for (SNInt x = componentMinVoxel.x; x <= componentMaxVoxel.x; x++)
			{
				const bool shouldAddX = (x < clampedOldMinVoxel.x) || (x > clampedOldMaxVoxel.x);
				const bool shouldRemoveX = (x < clampedMinVoxel.x) || (x > clampedMaxVoxel.x);

				const bool shouldAdd = shouldAddX && shouldAddY && shouldAddZ;
				const bool shouldRemove = shouldRemoveX && shouldRemoveY && shouldRemoveZ;

				const VoxelInt3 voxel(x, y, z);
				if (shouldAdd)
				{
					this->addedVoxels.emplace_back(voxel);
				}
				else if (shouldRemove)
				{
					this->removedVoxels.emplace_back(voxel);
				}
			}
		}
	}

}

void RenderLightChunkManager::Light::clear()
{
	this->lightID = -1;
	this->point = Double3::Zero;
	this->minPoint = Double3::Zero;
	this->maxPoint = Double3::Zero;
	this->voxels.clear();
	this->addedVoxels.clear();
	this->removedVoxels.clear();
	this->startRadius = 0.0;
	this->endRadius = 0.0;
	this->enabled = false;
}

RenderLightChunkManager::RenderLightChunkManager()
{
	
}

void RenderLightChunkManager::init(Renderer &renderer)
{
	// Player light is always allocated.
	RenderLightID playerLightID;
	if (!renderer.tryCreateLight(&playerLightID))
	{
		DebugLogError("Couldn't create render light ID for player.");
		return;
	}

	this->playerLight.init(playerLightID, Double3::Zero, ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS, ArenaRenderUtils::PLAYER_LIGHT_END_RADIUS, false);
	renderer.setLightRadius(this->playerLight.lightID, this->playerLight.startRadius, this->playerLight.endRadius);
}

void RenderLightChunkManager::shutdown(Renderer &renderer)
{
	for (int i = static_cast<int>(this->activeChunks.size()) - 1; i >= 0; i--)
	{
		ChunkPtr &chunkPtr = this->activeChunks[i];
		this->recycleChunk(i);
	}

	if (this->playerLight.lightID >= 0)
	{
		renderer.freeLight(this->playerLight.lightID);
		this->playerLight.clear();
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

				const CoordDouble3 entityCoord3D = entityChunkManager.getEntityPosition3D(entityInstID, ceilingScale, voxelChunkManager);
				const WorldDouble3 entityWorldPos = VoxelUtils::coordToWorldPoint(entityCoord3D);

				const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
				const double entityCenterYPosition = entityWorldPos.y + (entityBBox.height * 0.50);
				const WorldDouble3 entityLightWorldPos(entityWorldPos.x, entityCenterYPosition, entityWorldPos.z);

				Light light;
				light.init(lightID, entityLightWorldPos, ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS, *entityLightRadius, isLightEnabled);

				renderer.setLightPosition(lightID, light.point);
				renderer.setLightRadius(lightID, light.startRadius, light.endRadius);

				this->entityLights.emplace(entityInstID, std::move(light));
			}
		}
	}

	const WorldDouble3 prevPlayerLightPosition = this->playerLight.point;
	const WorldDouble3 newPlayerLightPosition = VoxelUtils::coordToWorldPoint(cameraCoord);
	this->playerLight.point = newPlayerLightPosition;

	if (isFogActive)
	{
		this->playerLight.startRadius = ArenaRenderUtils::PLAYER_FOG_LIGHT_START_RADIUS;
		this->playerLight.endRadius = ArenaRenderUtils::PLAYER_FOG_LIGHT_END_RADIUS;
	}
	else
	{
		this->playerLight.startRadius = ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS;
		this->playerLight.endRadius = ArenaRenderUtils::PLAYER_LIGHT_END_RADIUS;
	}

	const bool playerLightEnabledChanged = this->playerLight.enabled != playerHasLight;
	this->playerLight.enabled = playerHasLight;

	renderer.setLightPosition(this->playerLight.lightID, newPlayerLightPosition);
	renderer.setLightRadius(this->playerLight.lightID, this->playerLight.startRadius, this->playerLight.endRadius);

	// Clear all previous light references in voxels.
	// @todo: track touched, newly-touched, and no-longer-touched voxels each frame for moving lights so we don't have to iterate all voxels
	for (int i = 0; i < this->getChunkCount(); i++)
	{
		RenderLightChunk &renderChunk = this->getChunkAtIndex(i);
		for (RenderLightIdList &voxelLightIdList : renderChunk.lightIdLists)
		{
			voxelLightIdList.clear();
		}
	}

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
							RenderLightIdList &voxelLightIdList = renderChunkPtr->lightIdLists.get(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z);
							voxelLightIdList.tryAddLight(lightID);
						}
					}
				}
			}
		}
	};

	WorldInt3 prevPlayerLightVoxelMin, prevPlayerLightVoxelMax;
	WorldInt3 newPlayerLightVoxelMin, newPlayerLightVoxelMax;
	GetLightMinAndMaxVoxels(prevPlayerLightPosition, this->playerLight.endRadius, ceilingScale, &prevPlayerLightVoxelMin, &prevPlayerLightVoxelMax);
	GetLightMinAndMaxVoxels(newPlayerLightPosition, this->playerLight.endRadius, ceilingScale, &newPlayerLightVoxelMin, &newPlayerLightVoxelMax);

	if (this->playerLight.enabled)
	{
		populateTouchedVoxelLightIdLists(this->playerLight.lightID, newPlayerLightPosition, newPlayerLightVoxelMin, newPlayerLightVoxelMax);
	}

	// Populate each voxel's light ID list based on which lights touch them, preferring the nearest lights.
	// - @todo: this method doesn't implicitly allow sorting by distance because it doesn't check lights per voxel, it checks voxels per light.
	//   If sorting is desired then do it after this loop. It should also sort by intensity at the voxel center, not just distance to the light.
	for (const auto &pair : this->entityLights)
	{
		const Light &light = pair.second;
		if (light.enabled)
		{
			const RenderLightID lightID = light.lightID;
			const WorldDouble3 &lightPosition = light.point;

			WorldInt3 lightVoxelMin, lightVoxelMax;
			GetLightMinAndMaxVoxels(lightPosition, light.endRadius, ceilingScale, &lightVoxelMin, &lightVoxelMax);
			populateTouchedVoxelLightIdLists(lightID, lightPosition, lightVoxelMin, lightVoxelMax);
		}
	}

	auto tryAddDirtyLightVoxel = [this](const WorldInt3 &worldVoxel)
	{
		const CoordInt3 curLightCoord = VoxelUtils::worldVoxelToCoord(worldVoxel);
		RenderLightChunk *renderChunkPtr = this->tryGetChunkAtPosition(curLightCoord.chunk);
		if (renderChunkPtr != nullptr)
		{
			const VoxelInt3 &curLightVoxel = curLightCoord.voxel;
			if (renderChunkPtr->isValidVoxel(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z))
			{
				renderChunkPtr->setVoxelDirty(curLightVoxel);
			}
		}
	};

	// See which voxels affected by the player's light are getting their light references updated.
	// This is for dirty voxel draw calls mainly, not about setting light references (might change later).
	if ((prevPlayerLightVoxelMin != newPlayerLightVoxelMin) || (prevPlayerLightVoxelMax != newPlayerLightVoxelMax))
	{
		// Set voxel draw calls dirty for no-longer-touched voxels.
		for (WEInt z = prevPlayerLightVoxelMin.z; z <= prevPlayerLightVoxelMax.z; z++)
		{
			for (int y = prevPlayerLightVoxelMin.y; y <= prevPlayerLightVoxelMax.y; y++)
			{
				for (SNInt x = prevPlayerLightVoxelMin.x; x <= prevPlayerLightVoxelMax.x; x++)
				{
					const bool isInNewRange =
						(x >= newPlayerLightVoxelMin.x) && (x <= newPlayerLightVoxelMax.x) &&
						(y >= newPlayerLightVoxelMin.y) && (y <= newPlayerLightVoxelMax.y) &&
						(z >= newPlayerLightVoxelMin.z) && (z <= newPlayerLightVoxelMax.z);

					if (!isInNewRange)
					{
						tryAddDirtyLightVoxel(WorldInt3(x, y, z));
					}
				}
			}
		}

		// Set voxel draw calls dirty for newly-touched voxels.
		for (WEInt z = newPlayerLightVoxelMin.z; z <= newPlayerLightVoxelMax.z; z++)
		{
			for (int y = newPlayerLightVoxelMin.y; y <= newPlayerLightVoxelMax.y; y++)
			{
				for (SNInt x = newPlayerLightVoxelMin.x; x <= newPlayerLightVoxelMax.x; x++)
				{
					const bool isInPrevRange =
						(x >= prevPlayerLightVoxelMin.x) && (x <= prevPlayerLightVoxelMax.x) &&
						(y >= prevPlayerLightVoxelMin.y) && (y <= prevPlayerLightVoxelMax.y) &&
						(z >= prevPlayerLightVoxelMin.z) && (z <= prevPlayerLightVoxelMax.z);

					if (!isInPrevRange)
					{
						tryAddDirtyLightVoxel(WorldInt3(x, y, z));
					}
				}
			}
		}
	}

	// Set all player light voxels dirty if the option changed.
	if (playerLightEnabledChanged)
	{
		const WorldInt3 componentMinPlayerLightVoxelMin(
			std::min(prevPlayerLightVoxelMin.x, newPlayerLightVoxelMin.x),
			std::min(prevPlayerLightVoxelMin.y, newPlayerLightVoxelMin.y),
			std::min(prevPlayerLightVoxelMin.z, newPlayerLightVoxelMin.z));
		const WorldInt3 componentMaxPlayerLightVoxelMax(
			std::max(prevPlayerLightVoxelMax.x, newPlayerLightVoxelMax.x),
			std::max(prevPlayerLightVoxelMax.y, newPlayerLightVoxelMax.y),
			std::max(prevPlayerLightVoxelMax.z, newPlayerLightVoxelMax.z));

		for (WEInt z = componentMinPlayerLightVoxelMin.z; z <= componentMaxPlayerLightVoxelMax.z; z++)
		{
			for (int y = componentMinPlayerLightVoxelMin.y; y <= componentMaxPlayerLightVoxelMax.y; y++)
			{
				for (SNInt x = componentMinPlayerLightVoxelMin.x; x <= componentMaxPlayerLightVoxelMax.x; x++)
				{
					tryAddDirtyLightVoxel(WorldInt3(x, y, z));
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
		chunkPtr->dirtyVoxels.clear();
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
