#ifndef RENDER_INIT_SETTINGS_H
#define RENDER_INIT_SETTINGS_H

class RenderInitSettings
{
private:
	// @todo: rarely modified values
	// - i.e. max render width/height of window, aspect ratio, max thread count of hardware.
	// - note that it's _max_ thread count; the render frame settings can say what fraction to use
	//   and the renderer will just limit its for loop to giving work to that fraction.

	// @todo: might also contain SDL window handle for use with present().

	int width, height;
	int renderThreadsMode;
public:
	void init(int width, int height, int renderThreadsMode);

	int getWidth() const;
	int getHeight() const;
	int getRenderThreadsMode() const;
};

#endif
