#ifndef RENDERER_SYSTEM_3D_H
#define RENDERER_SYSTEM_3D_H

#include <cstdint>

#include "RenderTextureUtils.h"
#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../Media/Palette.h"

// Abstract base class for 3D renderer.

class Random;
class RenderCamera;
class RenderFrameSettings;
class RenderInitSettings;
class TextureBuilder;

class RendererSystem3D
{
public:
	struct LockedTexture
	{
		void *texels;
		bool isTrueColor;

		LockedTexture(void *texels, bool isTrueColor);

		bool isValid();
	};

	// Profiling info gathered from internal renderer state.
	struct ProfilerData
	{
		int width, height;
		int threadCount;
		int potentiallyVisFlatCount, visFlatCount, visLightCount;

		ProfilerData(int width, int height, int threadCount, int potentiallyVisFlatCount,
			int visFlatCount, int visLightCount);
	};

	virtual ~RendererSystem3D();

	virtual void init(const RenderInitSettings &settings) = 0;
	virtual void shutdown() = 0;

	virtual bool isInited() const = 0;

	virtual void resize(int width, int height) = 0;

	// Texture management functions.
	virtual bool tryCreateObjectTexture(int width, int height, ObjectTextureID *outID) = 0;
	virtual bool tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID) = 0;
	virtual LockedTexture lockObjectTexture(ObjectTextureID id) = 0;
	virtual void unlockObjectTexture(ObjectTextureID id) = 0;
	virtual void freeObjectTexture(ObjectTextureID id) = 0;

	// Tries to write out selection data for the given entity. Returns whether selection data was
	// successfully written.
	virtual bool tryGetEntitySelectionData(const Double2 &uv, ObjectTextureID textureID, bool pixelPerfect,
		bool *outIsSelected) const = 0;

	// Converts a screen point into a ray in the game world.
	virtual Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
		Degrees fovY, double aspect) const = 0;

	// Gets various profiler information about internal renderer state.
	virtual ProfilerData getProfilerData() const = 0;
	
	// Begins rendering a frame. Currently this is a blocking call and it should be safe to present the frame
	// upon returning from this.
	// @todo: this will take draw lists from SceneGraph eventually
	virtual void submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings) = 0;

	// Presents the finished frame to the screen. This may just be a copy to the screen frame buffer that
	// is then taken care of by the top-level rendering manager, since UI must be drawn afterwards.
	virtual void present() = 0;
};

#endif
