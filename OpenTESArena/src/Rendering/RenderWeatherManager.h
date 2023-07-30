#ifndef RENDER_WEATHER_MANAGER_H
#define RENDER_WEATHER_MANAGER_H

#include "RenderDrawCall.h"
#include "RenderTextureUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

class WeatherInstance;

struct RenderCamera;

class RenderWeatherManager
{
private:
	VertexBufferID particleVertexBufferID;
	AttributeBufferID particleNormalBufferID;
	AttributeBufferID particleTexCoordBufferID;
	IndexBufferID particleIndexBufferID;

	ObjectTextureID rainTextureID;
	Buffer<RenderDrawCall> rainDrawCalls;

	ObjectTextureID snowTextureIDs[3]; // Each snowflake size has its own texture.
	Buffer<RenderDrawCall> snowDrawCalls;

	VertexBufferID fogVertexBufferID;
	AttributeBufferID fogNormalBufferID;
	AttributeBufferID fogTexCoordBufferID;
	IndexBufferID fogIndexBufferID;
	ObjectTextureID fogTextureID;
	RenderDrawCall fogDrawCall;

	bool initMeshes(Renderer &renderer);
	bool initTextures(Renderer &renderer);

	void freeParticleBuffers(Renderer &renderer);
	void freeFogBuffers(Renderer &renderer);
public:
	RenderWeatherManager();

	bool init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	BufferView<const RenderDrawCall> getRainDrawCalls() const;
	BufferView<const RenderDrawCall> getSnowDrawCalls() const;
	const RenderDrawCall &getFogDrawCall() const;

	void loadScene();
	void update(const WeatherInstance &weatherInst, const RenderCamera &camera);
	void unloadScene();
};

#endif
