#ifndef RENDER_FRAME_SETTINGS_H
#define RENDER_FRAME_SETTINGS_H

#include <cstdint>

#include "RenderLightUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Utilities/Color.h"

#include "components/utilities/Span.h"

// 3D renderer variables that can change each frame.
struct RenderFrameSettings
{
	Color clearColor;
	double ambientPercent;
	UniformBufferID visibleLightsBufferID;
	int visibleLightCount;
	double screenSpaceAnimPercent;
	ObjectTextureID paletteTextureID, lightTableTextureID, skyBgTextureID;
	int renderThreadsMode;
	DitheringMode ditheringMode;

	RenderFrameSettings();

	void init(Color clearColor, double ambientPercent, UniformBufferID visibleLightsBufferID, int visibleLightCount, 
		double screenSpaceAnimPercent, ObjectTextureID paletteTextureID, ObjectTextureID lightTableTextureID,
		ObjectTextureID skyBgTextureID, int renderThreadsMode, DitheringMode ditheringMode);
};

#endif
