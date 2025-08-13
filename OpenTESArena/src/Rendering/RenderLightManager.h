#ifndef RENDER_LIGHT_MANAGER_H
#define RENDER_LIGHT_MANAGER_H

#include <unordered_map>
#include <vector>

#include "RenderLightUtils.h"
#include "../Entities/EntityChunkManager.h"

#include "components/utilities/Span.h"

class EntityChunkManager;
class Renderer;

struct RenderCamera;

struct RenderLight
{
	RenderLightID id;
	WorldDouble3 position;
	double startRadius, endRadius;
	bool enabled;

	RenderLight();
};

class RenderLightManager
{
private:
	RenderLight playerLight;
	std::unordered_map<EntityInstanceID, RenderLight> entityLights;
	std::vector<RenderLightID> visibleLightIDs;
public:
	Span<const RenderLightID> getVisibleLightIDs() const;

	void loadScene(Renderer &renderer);
	void update(const RenderCamera &camera, bool nightLightsAreActive, bool isFogActive, bool playerHasLight,
		const EntityChunkManager &entityChunkManager, Renderer &renderer);
	void unloadScene(Renderer &renderer);
};

#endif
