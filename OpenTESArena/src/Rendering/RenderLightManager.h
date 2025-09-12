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
	WorldDouble3 position;
	double startRadius, endRadius;

	RenderLight();
};

struct RenderLightEntry
{
	RenderLight light;
	bool enabled;

	RenderLightEntry();
};

class RenderLightManager
{
private:
	RenderLight playerLight;
	std::unordered_map<EntityInstanceID, RenderLightEntry> entityLights;
	UniformBufferID visibleLightsBufferID;
	int visibleLightCount;
public:
	static constexpr int MAX_VISIBLE_LIGHTS = 256;

	RenderLightManager();

	bool init(Renderer &renderer);
	void shutdown(Renderer &renderer);
	
	UniformBufferID getVisibleLightsBufferID() const;
	int getVisibleLightCount();

	void loadScene(Renderer &renderer);
	void update(const RenderCamera &camera, bool nightLightsAreActive, bool isFogActive, bool playerHasLight,
		const EntityChunkManager &entityChunkManager, Renderer &renderer);
	void unloadScene(Renderer &renderer);
};

#endif
