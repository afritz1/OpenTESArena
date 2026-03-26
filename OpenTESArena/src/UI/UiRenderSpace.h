#ifndef UI_RENDER_SPACE_H
#define UI_RENDER_SPACE_H

enum class UiRenderSpace
{
	Native, // Relative to the native window itself.
	Classic // Occupies a fixed-aspect-ratio portion of the native window.
};

#endif
