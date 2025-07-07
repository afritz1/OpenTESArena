#ifndef RENDER_WEATHER_MANAGER_H
#define RENDER_WEATHER_MANAGER_H

#include "RenderDrawCall.h"
#include "RenderTextureUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Span.h"

class WeatherInstance;

struct RenderCamera;
struct RenderCommandBuffer;

class RenderWeatherManager
{
private:
	VertexPositionBufferID particlePositionBufferID;
	VertexAttributeBufferID particleNormalBufferID;
	VertexAttributeBufferID particleTexCoordBufferID;
	IndexBufferID particleIndexBufferID;

	UniformBufferID rainTransformBufferID; // Contains render transforms for each raindrop.
	ObjectTextureID rainTextureID;
	Buffer<RenderDrawCall> rainDrawCalls;

	UniformBufferID snowTransformBufferID; // Contains render transforms for each snowflake. 
	ObjectTextureID snowTextureIDs[3]; // Each snowflake size has its own texture.
	Buffer<RenderDrawCall> snowDrawCalls;

	VertexPositionBufferID fogPositionBufferID;
	VertexAttributeBufferID fogNormalBufferID;
	VertexAttributeBufferID fogTexCoordBufferID;
	IndexBufferID fogIndexBufferID;
	UniformBufferID fogTransformBufferID;
	ObjectTextureID fogTextureID;
	RenderDrawCall fogDrawCall;

	bool initMeshes(Renderer &renderer);
	bool initUniforms(Renderer &renderer);
	bool initTextures(Renderer &renderer);

	void freeParticleBuffers(Renderer &renderer);
	void freeFogBuffers(Renderer &renderer);
public:
	RenderWeatherManager();

	bool init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	void populateCommandBuffer(RenderCommandBuffer &commandBuffer, const WeatherInstance &weatherInst, bool isFoggy) const;

	void loadScene();
	void update(const WeatherInstance &weatherInst, const RenderCamera &camera, Renderer &renderer);
	void unloadScene();
};

#endif
