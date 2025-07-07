#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RenderLightManager.h"

RenderLightManager::RenderLightManager()
{
	this->playerLightID = -1;
}

Span<const RenderLightID> RenderLightManager::getVisibleLightIDs() const
{
	return this->visibleLightIDs;
}

void RenderLightManager::loadScene(Renderer &renderer)
{
	DebugAssert(this->playerLightID < 0);
	this->playerLightID = renderer.createLight();
}

void RenderLightManager::update(const RenderCamera &camera, bool nightLightsAreActive, bool isFogActive, bool playerHasLight,
	const EntityChunkManager &entityChunkManager, Renderer &renderer)
{
	renderer.setLightPosition(this->playerLightID, camera.worldPoint);

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

	renderer.setLightRadius(this->playerLightID, newPlayerLightStartRadius, newPlayerLightEndRadius);

	this->visibleLightIDs.clear();

	if (playerHasLight)
	{
		this->visibleLightIDs.emplace_back(this->playerLightID);
	}

	// @todo bounding box tests against all lights
}

void RenderLightManager::unloadScene(Renderer &renderer)
{
	if (this->playerLightID >= 0)
	{
		renderer.freeLight(this->playerLightID);
		this->playerLightID = -1;
	}

	for (const auto &pair : this->entityLightIDs)
	{
		const RenderLightID lightID = pair.second;
		renderer.freeLight(lightID);
	}

	this->entityLightIDs.clear();
	this->visibleLightIDs.clear();
}
