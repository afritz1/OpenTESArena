#ifndef GUI_UTILS_H
#define GUI_UTILS_H

enum class PivotType;
enum class RenderSpace;

namespace GuiUtils
{
	// Converts pixel coordinates in the given render space to vector space in the same render space.
	void makeRenderElementPercents(int x, int y, int width, int height, int windowWidth, int windowHeight,
		RenderSpace renderSpace, PivotType pivotType, double *outXPercent, double *outYPercent,
		double *outWPercent, double *outHPercent);
}

#endif
