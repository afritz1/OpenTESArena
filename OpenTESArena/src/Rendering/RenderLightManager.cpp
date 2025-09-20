#include <algorithm>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderLightManager.h"

#include "components/utilities/Span.h"

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
	this->startRadius = 0.0;
	this->endRadius = 0.0;
}

RenderLightEntry::RenderLightEntry()
{
	this->enabled = false;
}

RenderLightManager::RenderLightManager()
{
	this->visibleLightsBufferID = -1;
}

bool RenderLightManager::init(Renderer &renderer)
{
	this->visibleLightsBufferID = renderer.createUniformBufferLights(RenderLightManager::MAX_VISIBLE_LIGHTS);
	if (this->visibleLightsBufferID < 0)
	{
		DebugLogError("Couldn't create visible lights uniform buffer.");
		return false;
	}

	return true;
}

void RenderLightManager::shutdown(Renderer &renderer)
{
	if (this->visibleLightsBufferID >= 0)
	{
		renderer.freeUniformBuffer(this->visibleLightsBufferID);
		this->visibleLightsBufferID = -1;
	}
}

UniformBufferID RenderLightManager::getVisibleLightsBufferID() const
{
	return this->visibleLightsBufferID;
}

int RenderLightManager::getVisibleLightCount()
{
	return this->visibleLightCount;
}

void RenderLightManager::loadScene(Renderer &renderer)
{
	
}

void RenderLightManager::update(const RenderCamera &camera, bool nightLightsAreActive, bool isFogActive, bool playerHasLight,
	const EntityChunkManager &entityChunkManager, Renderer &renderer)
{
	for (const EntityInstanceID entityInstID : entityChunkManager.getQueuedDestroyEntityIDs())
	{
		const auto entityLightIter = this->entityLights.find(entityInstID);
		if (entityLightIter != this->entityLights.end())
		{
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

			RenderLightEntry entityLight;
			entityLight.light.position = Double3::Zero;

			// The original game doesn't seem to update a light's radius after transitioning levels, it just uses the "S:#" from the start level .INF.
			const double lightEndRadius = *entityLightRadius;
			entityLight.light.startRadius = lightEndRadius * 0.50;
			entityLight.light.endRadius = lightEndRadius;

			entityLight.enabled = false;

			this->entityLights.emplace(entityInstID, std::move(entityLight));
		}
	}

	std::vector<RenderLight> visibleLights;
	visibleLights.reserve(this->entityLights.size());

	if (playerHasLight)
	{
		this->playerLight.position = camera.floatingWorldPoint;

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

		visibleLights.emplace_back(this->playerLight);
	}

	for (std::pair<const EntityInstanceID, RenderLightEntry> &pair : this->entityLights)
	{
		const EntityInstanceID entityInstID = pair.first;
		RenderLightEntry &entityLight = pair.second;

		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInstID);
		const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);
		const WorldDouble3 lightPosition = GetLightPositionInEntity(entityPosition, entityBBox);
		entityLight.light.position = lightPosition - camera.floatingOriginPoint;

		const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
		entityLight.enabled = !EntityUtils::isStreetlight(entityDef) || nightLightsAreActive;
		if (!entityLight.enabled)
		{
			continue;
		}

		const double entityLightWidth = entityLight.light.endRadius * 2.0;
		const double entityLightHeight = entityLightWidth;
		const double entityLightDepth = entityLightWidth;
		BoundingBox3D entityLightBBox;
		entityLightBBox.init(lightPosition, entityLightWidth, entityLightHeight, entityLightDepth);

		bool isBBoxCompletelyVisible, isBBoxCompletelyInvisible;
		RendererUtils::getBBoxVisibilityInFrustum(entityLightBBox, camera, &isBBoxCompletelyVisible, &isBBoxCompletelyInvisible);
		if (isBBoxCompletelyInvisible)
		{
			continue;
		}
		
		visibleLights.emplace_back(entityLight.light);
	}

	std::sort(visibleLights.begin(), visibleLights.end(),
		[&camera](const RenderLight &a, const RenderLight &b)
	{
		const double aDistSqr = (a.position - camera.floatingWorldPoint).lengthSquared();
		const double bDistSqr = (b.position - camera.floatingWorldPoint).lengthSquared();
		return aDistSqr < bDistSqr;
	});

	this->visibleLightCount = std::min(static_cast<int>(visibleLights.size()), RenderLightManager::MAX_VISIBLE_LIGHTS);

	Span<const RenderLight> visibleLightsView(visibleLights.data(), this->visibleLightCount);
	renderer.populateUniformBufferLights(this->visibleLightsBufferID, visibleLightsView);
}

void RenderLightManager::unloadScene(Renderer &renderer)
{
	this->entityLights.clear();
}
