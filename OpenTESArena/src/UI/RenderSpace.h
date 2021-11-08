#ifndef RENDER_SPACE_H
#define RENDER_SPACE_H

enum class RenderSpace
{
	Native, // Relative to the native window itself.
	Classic // Occupies a fixed-aspect-ratio portion of the native window.
};

#endif
