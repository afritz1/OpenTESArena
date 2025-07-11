#ifndef RENDER_FRAME_SETTINGS_H
#define RENDER_FRAME_SETTINGS_H

#include <cstdint>

#include "RenderLightUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"

#include "components/utilities/Span.h"

// 3D renderer variables that can change each frame.
struct RenderFrameSettings
{
	double ambientPercent;
	Span<const RenderLightID> visibleLightIDs;
	double screenSpaceAnimPercent;
	ObjectTextureID paletteTextureID, lightTableTextureID, skyBgTextureID;
	int renderWidth, renderHeight, renderThreadsMode;
	DitheringMode ditheringMode;

	RenderFrameSettings();

	void init(double ambientPercent, Span<const RenderLightID> visibleLightIDs, double screenSpaceAnimPercent, ObjectTextureID paletteTextureID,
		ObjectTextureID lightTableTextureID, ObjectTextureID skyBgTextureID, int renderWidth, int renderHeight, int renderThreadsMode,
		DitheringMode ditheringMode);
};

#endif
