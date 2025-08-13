#include <algorithm>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderLightManager.h"

namespace
{
	Double3 GetLightPositionInEntity(const WorldDouble3 &entityPos, const BoundingBox3D &entityBBox)
	{
		const double entityCenterYPosition = entityPos.y + entityBBox.halfHeight;
		return WorldDouble3(entityPos.x, entityCenterYPosition, entityPos.z);
	}
}

RenderLight::RenderLight()
{
	this->id = -1;
	this->startRadius = 0.0;
	this->endRadius = 0.0;
	this->enabled = false;
}

Span<const RenderLightID> RenderLightManager::getVisibleLightIDs() const
{
	return this->visibleLightIDs;
}

void RenderLightManager::loadScene(Renderer &renderer)
{
	DebugAssert(this->playerLight.id < 0);
	this->playerLight.id = renderer.createLight();
}

void RenderLightManager::update(const RenderCamera &camera, bool nightLightsAreActive, bool isFogActive, bool playerHasLight,
	const EntityChunkManager &entityChunkManager, Renderer &renderer)
{
	// Update player light.
	this->playerLight.position = camera.worldPoint;

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

	renderer.populateLight(this->playerLight.id, this->playerLight.position, newPlayerLightStartRadius, newPlayerLightEndRadius);

	// Free destroyed entity lights.
	for (const EntityInstanceID entityInstID : entityChunkManager.getQueuedDestroyEntityIDs())
	{
		const auto entityLightIter = this->entityLights.find(entityInstID);
		if (entityLightIter != this->entityLights.end())
		{
			const RenderLight &entityLight = entityLightIter->second;
			renderer.freeLight(entityLight.id);
			this->entityLights.erase(entityLightIter);
		}
	}

	// @todo shouldn't need to iterate over chunks, just the chunk manager's total
	for (int i = 0; i < entityChunkManager.getChunkCount(); i++)
	{
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtIndex(i);
		const ChunkInt2 entityChunkPos = entityChunk.position;
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const auto entityLightIter = this->entityLights.find(entityInstID);
			if (entityLightIter != this->entityLights.end())
			{
				// Entity already has a light added.
				continue;
			}

			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const std::optional<double> entityLightRadius = EntityUtils::tryGetLightRadius(entityDef);
			const bool entityHasLight = entityLightRadius.has_value();
			if (!entityHasLight)
			{
				continue;
			}

			RenderLight entityLight;
			entityLight.id = renderer.createLight();
			if (entityLight.id < 0)
			{
				DebugLogErrorFormat("Couldn't allocate render light ID for entity %d in chunk (%s).", entityInstID, entityChunkPos.toString().c_str());
				break;
			}

			// The original game doesn't seem to update a light's radius after transitioning levels, it just uses the "S:#" from the start level .INF.
			const double lightEndRadius = *entityLightRadius;
			entityLight.startRadius = lightEndRadius * 0.50;
			entityLight.endRadius = lightEndRadius;
			if (!renderer.populateLight(entityLight.id, Double3::Zero, entityLight.startRadius, entityLight.endRadius))
			{
				DebugLogErrorFormat("Couldn't populate light for entity %d in chunk (%s).", entityInstID, entityChunkPos.toString().c_str());
			}

			this->entityLights.emplace(entityInstID, std::move(entityLight));
		}
	}

	// Update entity light positions and determine lights that contribute to rendering.
	std::vector<const RenderLight*> enabledLights;
	enabledLights.reserve(this->entityLights.size());

	for (auto &pair : this->entityLights)
	{
		const EntityInstanceID entityInstID = pair.first;
		RenderLight &entityLight = pair.second;

		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInstID);
		const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
		entityLight.position = GetLightPositionInEntity(entityPosition, entityBBox);
		renderer.populateLight(entityLight.id, entityLight.position, entityLight.startRadius, entityLight.endRadius);

		const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
		entityLight.enabled = !EntityUtils::isStreetlight(entityDef) || nightLightsAreActive;
		if (!entityLight.enabled)
		{
			continue;
		}

		const double entityLightWidth = entityLight.endRadius * 2.0;
		const double entityLightHeight = entityLightWidth;
		const double entityLightDepth = entityLightWidth;
		BoundingBox3D entityLightBBox;
		entityLightBBox.init(entityLight.position, entityLightWidth, entityLightHeight, entityLightDepth);

		bool isBBoxCompletelyVisible, isBBoxCompletelyInvisible;
		RendererUtils::getBBoxVisibilityInFrustum(entityLightBBox, camera, &isBBoxCompletelyVisible, &isBBoxCompletelyInvisible);
		if (isBBoxCompletelyInvisible)
		{
			continue;
		}
		
		enabledLights.emplace_back(&entityLight);
	}

	std::sort(enabledLights.begin(), enabledLights.end(),
		[&camera](const RenderLight *a, const RenderLight *b)
	{
		const double aDistSqr = (a->position - camera.worldPoint).lengthSquared();
		const double bDistSqr = (b->position - camera.worldPoint).lengthSquared();
		return aDistSqr < bDistSqr;
	});

	this->visibleLightIDs.clear();

	if (playerHasLight)
	{
		this->visibleLightIDs.emplace_back(this->playerLight.id);
	}

	for (const RenderLight *enabledLight : enabledLights)
	{
		this->visibleLightIDs.emplace_back(enabledLight->id);
	}	
}

void RenderLightManager::unloadScene(Renderer &renderer)
{
	if (this->playerLight.id >= 0)
	{
		renderer.freeLight(this->playerLight.id);
		this->playerLight.id = -1;
	}

	for (const auto &pair : this->entityLights)
	{
		const RenderLight &entityLight = pair.second;
		renderer.freeLight(entityLight.id);
	}

	this->entityLights.clear();
	this->visibleLightIDs.clear();
}
