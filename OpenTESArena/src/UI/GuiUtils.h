#ifndef GUI_UTILS_H
#define GUI_UTILS_H

enum class UiPivotType;
enum class UiRenderSpace;

struct Rect;

namespace GuiUtils
{
	// Converts pixel coordinates in the given render space to pixel coordinates for display.
	Rect makeWindowSpaceRect(int x, int y, int width, int height, UiPivotType pivotType, UiRenderSpace renderSpace, int windowWidth, int windowHeight, Rect letterboxRect);
	Rect makeWindowSpaceRect(Rect rect, UiPivotType pivotType, UiRenderSpace renderSpace, int windowWidth, int windowHeight, Rect letterboxRect);
}

#endif
