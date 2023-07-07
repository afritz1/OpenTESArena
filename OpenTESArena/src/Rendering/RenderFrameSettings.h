#ifndef RENDER_FRAME_SETTINGS_H
#define RENDER_FRAME_SETTINGS_H

#include <cstdint>

#include "RenderTextureUtils.h"

// 3D renderer variables that can change each frame.

struct RenderFrameSettings
{
	double ambientPercent;
	ObjectTextureID paletteTextureID, lightTableTextureID;
	int renderWidth, renderHeight, renderThreadsMode;

	void init(double ambientPercent, ObjectTextureID paletteTextureID, ObjectTextureID lightTableTextureID,
		int renderWidth, int renderHeight, int renderThreadsMode);
};

#endif
