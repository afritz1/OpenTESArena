#include <string>

#include "SDL.h"

#include "GuiUtils.h"
#include "UiPivotType.h"
#include "UiRenderSpace.h"
#include "../Math/Rect.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

namespace
{
	void MakeRenderElementPercents(int x, int y, int width, int height, int windowWidth, int windowHeight, UiRenderSpace renderSpace, UiPivotType pivotType,
		double *outXPercent, double *outYPercent, double *outWPercent, double *outHPercent)
	{
		double renderSpaceWidthReal, renderSpaceHeightReal;
		if (renderSpace == UiRenderSpace::Classic)
		{
			renderSpaceWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
			renderSpaceHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;
		}
		else if (renderSpace == UiRenderSpace::Native)
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
			if ((pivotType == UiPivotType::TopLeft) ||
				(pivotType == UiPivotType::MiddleLeft) ||
				(pivotType == UiPivotType::BottomLeft))
			{
				return 0;
			}
			else if ((pivotType == UiPivotType::Top) ||
				(pivotType == UiPivotType::Middle) ||
				(pivotType == UiPivotType::Bottom))
			{
				return -width / 2;
			}
			else if ((pivotType == UiPivotType::TopRight) ||
				(pivotType == UiPivotType::MiddleRight) ||
				(pivotType == UiPivotType::BottomRight))
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
			if ((pivotType == UiPivotType::TopLeft) ||
				(pivotType == UiPivotType::Top) ||
				(pivotType == UiPivotType::TopRight))
			{
				return 0;
			}
			else if ((pivotType == UiPivotType::MiddleLeft) ||
				(pivotType == UiPivotType::Middle) ||
				(pivotType == UiPivotType::MiddleRight))
			{
				return -height / 2;
			}
			else if ((pivotType == UiPivotType::BottomLeft) ||
				(pivotType == UiPivotType::Bottom) ||
				(pivotType == UiPivotType::BottomRight))
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
}

Rect GuiUtils::makeWindowSpaceRect(int x, int y, int width, int height, UiPivotType pivotType, UiRenderSpace renderSpace, int windowWidth, int windowHeight, Rect letterboxRect)
{
	double xPercent, yPercent, widthPercent, heightPercent;
	MakeRenderElementPercents(x, y, width, height, windowWidth, windowHeight, renderSpace, pivotType, &xPercent, &yPercent, &widthPercent, &heightPercent);

	Rect windowRect;
	if (renderSpace == UiRenderSpace::Native)
	{
		const double windowWidthReal = static_cast<double>(windowWidth);
		const double windowHeightReal = static_cast<double>(windowHeight);

		windowRect.x = static_cast<int>(std::round(xPercent * windowWidthReal));
		windowRect.y = static_cast<int>(std::round(yPercent * windowHeightReal));
		windowRect.width = static_cast<int>(std::round(widthPercent * windowWidthReal));
		windowRect.height = static_cast<int>(std::round(heightPercent * windowHeightReal));
	}
	else if (renderSpace == UiRenderSpace::Classic)
	{
		constexpr double classicScreenWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
		constexpr double classicScreenHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;

		// @todo something in here is causing hairline cracks

		auto originalPointToNative = [&letterboxRect](Int2 point)
		{
			const double originalXPercent = static_cast<double>(point.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
			const double originalYPercent = static_cast<double>(point.y) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

			const double letterboxWidthReal = static_cast<double>(letterboxRect.width);
			const double letterboxHeightReal = static_cast<double>(letterboxRect.height);
			const Int2 letterboxPoint(
				static_cast<int>(std::round(letterboxWidthReal * originalXPercent)),
				static_cast<int>(std::round(letterboxHeightReal * originalYPercent)));
			const Int2 nativePoint(
				letterboxPoint.x + letterboxRect.getLeft(),
				letterboxPoint.y + letterboxRect.getTop());
			return nativePoint;
		};

		Rect originalRect;
		originalRect.x = static_cast<int>(std::round(xPercent * classicScreenWidthReal));
		originalRect.y = static_cast<int>(std::round(yPercent * classicScreenHeightReal));
		originalRect.width = static_cast<int>(std::round(widthPercent * classicScreenWidthReal));
		originalRect.height = static_cast<int>(std::round(heightPercent * classicScreenHeightReal));

		const Int2 oldTopLeft(originalRect.x, originalRect.y);
		const Int2 oldBottomRight(originalRect.x + originalRect.width, originalRect.y + originalRect.height);
		const Int2 newTopLeft = originalPointToNative(oldTopLeft);
		const Int2 newBottomRight = originalPointToNative(oldBottomRight);

		windowRect.x = newTopLeft.x;
		windowRect.y = newTopLeft.y;
		windowRect.width = newBottomRight.x - newTopLeft.x;
		windowRect.height = newBottomRight.y - newTopLeft.y;
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(renderSpace)));
	}

	return windowRect;
}

Rect GuiUtils::makeWindowSpaceRect(Rect rect, UiPivotType pivotType, UiRenderSpace renderSpace, int windowWidth, int windowHeight, Rect letterboxRect)
{
	return GuiUtils::makeWindowSpaceRect(rect.x, rect.y, rect.width, rect.height, pivotType, renderSpace, windowWidth, windowHeight, letterboxRect);
}
