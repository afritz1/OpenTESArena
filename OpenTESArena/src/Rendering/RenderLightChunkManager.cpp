#include "ArenaRenderUtils.h"
#include "Renderer.h"
#include "RenderLightChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Voxels/VoxelChunkManager.h"

#include "components/debug/Debug.h"

namespace
{
	Double3 GetEntityLightPosition(const WorldDouble3 &entityPos, const BoundingBox3D &entityBBox)
	{
		const double entityCenterYPosition = EntityUtils::getCenterY(entityPos.y, entityBBox.height);
		return WorldDouble3(entityPos.x, entityCenterYPosition, entityPos.z);
	}
}

RenderLightChunkManager::Light::Light()
{
	this->lightID = -1;
	this->startRadius = 0.0;
	this->endRadius = 0.0;
	this->enabled = false;
}

void RenderLightChunkManager::Light::init(RenderLightID lightID, const WorldDouble3 &point, double startRadius, double endRadius, bool enabled, double ceilingScale, int chunkHeight)
{
	this->lightID = lightID;
	this->point = point;
	this->minPoint = point - WorldDouble3(endRadius, endRadius, endRadius);
	this->maxPoint = point + WorldDouble3(endRadius, endRadius, endRadius);
	this->startRadius = startRadius;
	this->endRadius = endRadius;
	this->enabled = enabled;

	const int voxelYMax = chunkHeight - 1;
	const WorldInt3 minVoxel = VoxelUtils::pointToVoxel(this->minPoint, ceilingScale);
	const WorldInt3 maxVoxel = VoxelUtils::pointToVoxel(this->maxPoint, ceilingScale);
	const WorldInt3 clampedMinVoxel(minVoxel.x, std::clamp(minVoxel.y, 0, voxelYMax), minVoxel.z);
	const WorldInt3 clampedMaxVoxel(maxVoxel.x, std::clamp(maxVoxel.y, 0, voxelYMax), maxVoxel.z);

	// Add current and newly-touched voxels.
	for (WEInt z = clampedMinVoxel.z; z <= clampedMaxVoxel.z; z++)
	{
		for (int y = clampedMinVoxel.y; y <= clampedMaxVoxel.y; y++)
		{
			for (SNInt x = clampedMinVoxel.x; x <= clampedMaxVoxel.x; x++)
			{
				const VoxelInt3 voxel(x, y, z);
				this->voxels.emplace_back(voxel);
				this->addedVoxels.emplace_back(voxel);
			}
		}
	}
}

void RenderLightChunkManager::Light::update(const WorldDouble3 &point, double startRadius, double endRadius, double ceilingScale, int chunkHeight)
{
	const double oldRadius = this->endRadius;
	const double radius = endRadius;
	this->startRadius = startRadius;
	this->endRadius = endRadius;

	const WorldDouble3 oldPoint = this->point;
	const WorldDouble3 oldMinPoint = this->minPoint;
	const WorldDouble3 oldMaxPoint = this->maxPoint;
	this->point = point;
	this->minPoint = point - WorldDouble3(radius, radius, radius);
	this->maxPoint = point + WorldDouble3(radius, radius, radius);

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

	this->removedVoxels.clear();

	// Add no-longer-touched voxels.
	for (const VoxelInt3 &voxel : this->voxels)
	{
		const bool shouldRemoveX = (voxel.x < clampedMinVoxel.x) || (voxel.x > clampedMaxVoxel.x);
		const bool shouldRemoveY = (voxel.y < clampedMinVoxel.y) || (voxel.y > clampedMaxVoxel.y);
		const bool shouldRemoveZ = (voxel.z < clampedMinVoxel.z) || (voxel.z > clampedMaxVoxel.z);
		const bool shouldRemove = shouldRemoveX || shouldRemoveY || shouldRemoveZ;
		if (shouldRemove)
		{
			this->removedVoxels.emplace_back(voxel);
		}
	}

	this->voxels.clear();
	this->addedVoxels.clear();

	// Add current and newly-touched voxels.
	for (WEInt z = clampedMinVoxel.z; z <= clampedMaxVoxel.z; z++)
	{
		const bool shouldAddZ = (z < clampedOldMinVoxel.z) || (z > clampedOldMaxVoxel.z);

		for (int y = clampedMinVoxel.y; y <= clampedMaxVoxel.y; y++)
		{
			const bool shouldAddY = (y < clampedOldMinVoxel.y) || (y > clampedOldMaxVoxel.y);

			for (SNInt x = clampedMinVoxel.x; x <= clampedMaxVoxel.x; x++)
			{
				const bool shouldAddX = (x < clampedOldMinVoxel.x) || (x > clampedOldMaxVoxel.x);
				const VoxelInt3 voxel(x, y, z);

				this->voxels.emplace_back(voxel);

				const bool shouldAdd = shouldAddX || shouldAddY || shouldAddZ;
				if (shouldAdd)
				{
					this->addedVoxels.emplace_back(voxel);
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
	this->isSceneChanged = false;
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

	this->playerLight.init(playerLightID, Double3::Zero, 0.0, 0.0, false, 0.0, 1);
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

void RenderLightChunkManager::registerLightToVoxels(const Light &light, BufferView<const WorldInt3> voxels, double ceilingScale)
{
	for (const WorldInt3 &voxel : voxels)
	{
		const CoordInt3 curLightCoord = VoxelUtils::worldVoxelToCoord(voxel);
		RenderLightChunk *renderChunkPtr = this->tryGetChunkAtPosition(curLightCoord.chunk);
		if (renderChunkPtr != nullptr)
		{
			const VoxelInt3 &curLightVoxel = curLightCoord.voxel;

			// Calculate distance to voxel center for sorting.
			const WorldDouble3 voxelCenter = VoxelUtils::getVoxelCenter(voxel, ceilingScale);
			const double distanceSqr = (voxelCenter - light.point).lengthSquared();

			RenderLightIdList &voxelLightIdList = renderChunkPtr->lightIdLists.get(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z);
			voxelLightIdList.tryAddLight(light.lightID, distanceSqr);

			renderChunkPtr->setVoxelDirty(curLightVoxel);
		}
	}
}

void RenderLightChunkManager::unregisterLightFromVoxels(const Light &light, BufferView<const WorldInt3> voxels)
{
	for (const WorldInt3 &voxel : voxels)
	{
		const CoordInt3 curLightCoord = VoxelUtils::worldVoxelToCoord(voxel);
		RenderLightChunk *renderChunkPtr = this->tryGetChunkAtPosition(curLightCoord.chunk);
		if (renderChunkPtr != nullptr)
		{
			const VoxelInt3 &curLightVoxel = curLightCoord.voxel;

			// @todo: this check is currently needed for scene transitions when chunk heights between scenes differ
			if (renderChunkPtr->isValidVoxel(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z))
			{
				RenderLightIdList &voxelLightIdList = renderChunkPtr->lightIdLists.get(curLightVoxel.x, curLightVoxel.y, curLightVoxel.z);
				voxelLightIdList.removeLight(light.lightID);

				renderChunkPtr->setVoxelDirty(curLightVoxel);
			}
		}
	}
}

void RenderLightChunkManager::loadScene()
{
	this->isSceneChanged = true;
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
			this->unregisterLightFromVoxels(light, light.voxels);
			this->unregisterLightFromVoxels(light, light.removedVoxels);

			renderer.freeLight(light.lightID);
			this->entityLights.erase(iter);
		}
	}

	const int chunkHeight = this->getChunkAtIndex(0).getHeight();
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
				const WorldDouble3 entityLightWorldPos = GetEntityLightPosition(entityWorldPos, entityBBox);

				Light light;
				light.init(lightID, entityLightWorldPos, ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS, *entityLightRadius, isLightEnabled, ceilingScale, chunkHeight);

				if (isLightEnabled)
				{
					this->registerLightToVoxels(light, light.addedVoxels, ceilingScale);
				}

				renderer.setLightPosition(lightID, light.point);
				renderer.setLightRadius(lightID, light.startRadius, light.endRadius);

				this->entityLights.emplace(entityInstID, std::move(light));
			}
		}
	}

	// Update player light position and touched voxels.
	const WorldDouble3 newPlayerLightPosition = VoxelUtils::coordToWorldPoint(cameraCoord);
	double newPlayerLightStartRadius, newPlayerLightEndRadius;
	if (isFogActive)
	{
		newPlayerLightStartRadius = ArenaRenderUtils::PLAYER_FOG_LIGHT_START_RADIUS;
		newPlayerLightEndRadius = ArenaRenderUtils::PLAYER_FOG_LIGHT_END_RADIUS;
	}
	else
	{
		newPlayerLightStartRadius = ArenaRenderUtils::PLAYER_LIGHT_START_RADIUS;
		newPlayerLightEndRadius = ArenaRenderUtils::PLAYER_LIGHT_END_RADIUS;
	}

	this->playerLight.update(newPlayerLightPosition, newPlayerLightStartRadius, newPlayerLightEndRadius, ceilingScale, chunkHeight);

	const bool playerLightEnabledChanged = this->playerLight.enabled != playerHasLight;
	if (playerHasLight)
	{
		this->playerLight.enabled = true;
		renderer.setLightPosition(this->playerLight.lightID, newPlayerLightPosition);
		renderer.setLightRadius(this->playerLight.lightID, this->playerLight.startRadius, this->playerLight.endRadius);
	}
	else
	{
		this->playerLight.enabled = false;
	}

	// Update entity light positions and touched voxels.
	for (auto &pair : this->entityLights)
	{
		const EntityInstanceID entityInstID = pair.first;
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const CoordDouble3 entityCoord = entityChunkManager.getEntityPosition3D(entityInstID, ceilingScale, voxelChunkManager);
		const WorldDouble3 entityWorldPos = VoxelUtils::coordToWorldPoint(entityCoord);
		const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
		const WorldDouble3 entityLightWorldPos = GetEntityLightPosition(entityWorldPos, entityBBox);

		Light &light = pair.second;
		light.update(entityLightWorldPos, light.startRadius, light.endRadius, ceilingScale, chunkHeight);

		if (light.enabled)
		{
			renderer.setLightPosition(light.lightID, entityLightWorldPos);
		}
	}

	// Unassign lights from no-longer-touched light ID lists and current-touched light ID lists if disabled.
	this->unregisterLightFromVoxels(this->playerLight, this->playerLight.removedVoxels);
	if ((playerLightEnabledChanged && !this->playerLight.enabled) || this->isSceneChanged)
	{
		this->unregisterLightFromVoxels(this->playerLight, this->playerLight.voxels);
	}

	for (const auto &pair : this->entityLights)
	{
		const Light &light = pair.second;
		this->unregisterLightFromVoxels(light, light.removedVoxels);
		if (!light.enabled)
		{
			this->unregisterLightFromVoxels(light, light.voxels);
		}
	}

	// Add lights to newly-touched light ID lists if enabled.
	if (this->playerLight.enabled)
	{
		if (playerLightEnabledChanged || this->isSceneChanged)
		{
			this->registerLightToVoxels(this->playerLight, this->playerLight.voxels, ceilingScale);
		}
		else
		{
			this->registerLightToVoxels(this->playerLight, this->playerLight.addedVoxels, ceilingScale);
		}
	}

	for (const auto &pair : this->entityLights)
	{
		const Light &light = pair.second;
		if (light.enabled)
		{
			this->registerLightToVoxels(light, light.addedVoxels, ceilingScale);
		}
	}

	// @todo: add unordered_map<EntityInstanceID, RenderLightIdList> so entities don't rely on RenderLightChunk voxel light ID lists
	// - and then sort each entity's light ID list by distance to entity position
}

void RenderLightChunkManager::setNightLightsActive(bool enabled, double ceilingScale, const EntityChunkManager &entityChunkManager)
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
				if (enabled)
				{
					light.enabled = true;
					this->registerLightToVoxels(light, light.voxels, ceilingScale);
				}
				else
				{
					light.enabled = false;
					this->unregisterLightFromVoxels(light, light.voxels);
				}
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

	this->playerLight.addedVoxels.clear();
	this->playerLight.removedVoxels.clear();

	for (auto &pair : this->entityLights)
	{
		Light &light = pair.second;
		light.addedVoxels.clear();
		light.removedVoxels.clear();
	}

	this->isSceneChanged = false;
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
