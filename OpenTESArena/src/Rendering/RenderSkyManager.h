#ifndef RENDER_SKY_MANAGER_H
#define RENDER_SKY_MANAGER_H

#include "RenderDrawCall.h"
#include "RenderGeometryUtils.h"
#include "RenderTextureUtils.h"

class Renderer;

class RenderSkyManager
{
private:
	VertexBufferID bgVertexBufferID;
	AttributeBufferID bgNormalBufferID;
	AttributeBufferID bgTexCoordBufferID;
	IndexBufferID bgIndexBufferID;
	ObjectTextureID bgObjectTextureID;
	RenderDrawCall bgDrawCall;

	// @todo: sky rendering resources
	// - list of vertex buffer IDs for sky objects
	// - list of transforms for sky positions
	// - list of draw calls
	// - order matters: stars, sun, planets, clouds, mountains (etc.)

	void freeBgBuffers(Renderer &renderer);
public:
	RenderSkyManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	RenderDrawCall getBgDrawCall() const;
};

#endif
