#ifndef GUI_UTILS_H
#define GUI_UTILS_H

enum class PivotType;
enum class RenderSpace;

struct Rect;

namespace GuiUtils
{
	// Converts pixel coordinates in the given render space to pixel coordinates for display.
	Rect makeWindowSpaceRect(int x, int y, int width, int height, PivotType pivotType, RenderSpace renderSpace, int windowWidth, int windowHeight, Rect letterboxRect);
	Rect makeWindowSpaceRect(Rect rect, PivotType pivotType, RenderSpace renderSpace, int windowWidth, int windowHeight, Rect letterboxRect);
}

#endif
