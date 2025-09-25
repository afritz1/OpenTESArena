#ifndef RENDER_WEATHER_MANAGER_H
#define RENDER_WEATHER_MANAGER_H

#include "ArenaRenderUtils.h"
#include "RenderDrawCall.h"
#include "RenderMaterialUtils.h"
#include "RenderTextureUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span.h"

class WeatherInstance;

struct RenderCamera;
struct RenderCommandList;

class RenderWeatherManager
{
private:
	VertexPositionBufferID particlePositionBufferID;
	VertexAttributeBufferID particleNormalBufferID;
	VertexAttributeBufferID particleTexCoordBufferID;
	IndexBufferID particleIndexBufferID;

	UniformBufferID rainTransformBufferID; // Contains render transforms for each raindrop.
	ObjectTextureID rainTextureID;
	RenderMaterialID rainMaterialID;
	Buffer<RenderDrawCall> rainDrawCalls;

	UniformBufferID snowTransformBufferID; // Contains render transforms for each snowflake. 
	ObjectTextureID snowTextureIDs[3]; // Each snowflake size has its own texture.
	RenderMaterialID snowMaterialIDs[3];
	Buffer<RenderDrawCall> snowDrawCalls;

	VertexPositionBufferID fogPositionBufferID;
	VertexAttributeBufferID fogNormalBufferID;
	VertexAttributeBufferID fogTexCoordBufferID;
	IndexBufferID fogIndexBufferID;
	UniformBufferID fogTransformBufferID;
	ObjectTextureID fogTextureID;
	RenderMaterialID fogMaterialID;
	RenderDrawCall fogDrawCall;

	RenderMaterialInstanceID materialInstID; // Shared by all weather particles for mesh lighting.

	ArenaFogState fogState;

	bool initMeshes(Renderer &renderer);
	bool initUniforms(Renderer &renderer);
	bool initTextures(TextureManager &textureManager, Renderer &renderer);
	bool initMaterials(Renderer &renderer);

	void freeParticleBuffers(Renderer &renderer);
	void freeFogBuffers(Renderer &renderer);
public:
	RenderWeatherManager();

	bool init(TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	void populateCommandList(RenderCommandList &commandList, const WeatherInstance &weatherInst, bool isFoggy) const;

	void loadScene();
	void update(double dt, const WeatherInstance &weatherInst, const RenderCamera &camera, Renderer &renderer);
	void unloadScene();
};

#endif
