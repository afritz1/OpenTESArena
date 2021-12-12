#ifndef RENDER_FRAME_SETTINGS_H
#define RENDER_FRAME_SETTINGS_H

// 3D renderer variables that can change each frame.

// @todo: will likely want RenderFrameSettings with something like:
// - RenderCamera?
// - double resolutionScale, ambientPercent; // Fog doesn't exist anymore
// - ObjectTextureID paletteTextureID, lightTableTextureID, skyColorsTextureID, thunderstormColorsTextureID; // Sky colors will be a 1D texture updated per frame
// - int renderThreadsMode;
// - uint32_t *outputBuffer;
// and these will only be visible to SceneGraph:
// - deltaTime, daytimePercent, nightLightsAreActive, playerHasLight, chunkDistance, ceilingScale, levelInst/skyInst/weatherInst, random, latitude, chasmAnimPercent, etc.

class RenderFrameSettings
{
private:
	// @todo: shader variables for a given frame, time of day, delta time, etc.
	// - things that don't fit into camera or bulk voxel/entity/sky-object data.
	// - ceiling height
	double resolutionScale;
public:
	void init();
};

#endif
