#ifndef RENDER_FRAME_SETTINGS_H
#define RENDER_FRAME_SETTINGS_H

#include <cstdint>

#include "RenderTextureUtils.h"

// 3D renderer variables that can change each frame.

struct RenderFrameSettings
{
	RenderCamera camera;
	double ambientPercent;
	ObjectTextureID paletteTextureID, lightTableTextureID, skyColorsTextureID, thunderstormColorsTextureID;
	int renderWidth, renderHeight, renderThreadsMode;

	void init(const RenderCamera &camera, double ambientPercent, ObjectTextureID paletteTextureID,
		ObjectTextureID lightTableTextureID, ObjectTextureID skyColorsTextureID,
		ObjectTextureID thunderstormColorsTextureID, int renderWidth, int renderHeight, int renderThreadsMode);
};

#endif
