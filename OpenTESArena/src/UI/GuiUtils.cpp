#include <string>

#include "GuiUtils.h"
#include "PivotType.h"
#include "RenderSpace.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

void GuiUtils::makeRenderElementPercents(int x, int y, int width, int height, int windowWidth, int windowHeight,
	RenderSpace renderSpace, PivotType pivotType, double *outXPercent, double *outYPercent,
	double *outWPercent, double *outHPercent)
{
	double renderSpaceWidthReal, renderSpaceHeightReal;
	if (renderSpace == RenderSpace::Classic)
	{
		renderSpaceWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
		renderSpaceHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;
	}
	else if (renderSpace == RenderSpace::Native)
	{
		renderSpaceWidthReal = static_cast<double>(windowWidth);
		renderSpaceHeightReal = static_cast<double>(windowHeight);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(renderSpace)));
	}

	const int xOffset = [width, pivotType]()
	{
		if ((pivotType == PivotType::TopLeft) ||
			(pivotType == PivotType::MiddleLeft) ||
			(pivotType == PivotType::BottomLeft))
		{
			return 0;
		}
		else if ((pivotType == PivotType::Top) ||
			(pivotType == PivotType::Middle) ||
			(pivotType == PivotType::Bottom))
		{
			return -width / 2;
		}
		else if ((pivotType == PivotType::TopRight) ||
			(pivotType == PivotType::MiddleRight) ||
			(pivotType == PivotType::BottomRight))
		{
			return -width;
		}
		else
		{
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(pivotType)));
		}
	}();

	const int yOffset = [height, pivotType]()
	{
		if ((pivotType == PivotType::TopLeft) ||
			(pivotType == PivotType::Top) ||
			(pivotType == PivotType::TopRight))
		{
			return 0;
		}
		else if ((pivotType == PivotType::MiddleLeft) ||
			(pivotType == PivotType::Middle) ||
			(pivotType == PivotType::MiddleRight))
		{
			return -height / 2;
		}
		else if ((pivotType == PivotType::BottomLeft) ||
			(pivotType == PivotType::Bottom) ||
			(pivotType == PivotType::BottomRight))
		{
			return -height;
		}
		else
		{
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(pivotType)));
		}
	}();

	*outXPercent = static_cast<double>(x + xOffset) / renderSpaceWidthReal;
	*outYPercent = static_cast<double>(y + yOffset) / renderSpaceHeightReal;
	*outWPercent = static_cast<double>(width) / renderSpaceWidthReal;
	*outHPercent = static_cast<double>(height) / renderSpaceHeightReal;
}
