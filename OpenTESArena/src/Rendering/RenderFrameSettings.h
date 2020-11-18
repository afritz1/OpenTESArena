#ifndef RENDER_FRAME_SETTINGS_H
#define RENDER_FRAME_SETTINGS_H

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
