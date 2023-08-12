#include <algorithm>
#include <cmath>
#include <tuple>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "RendererUtils.h"
#include "../Game/CardinalDirection.h"
#include "../Math/Constants.h"
#include "../Utilities/Platform.h"
#include "../Voxels/VoxelChunk.h"
#include "../Weather/WeatherInstance.h"

#include "components/debug/Debug.h"

RenderCamera RendererUtils::makeCamera(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction,
	Degrees fovY, double aspectRatio, bool tallPixelCorrection)
{
	const double tallPixelRatio = tallPixelCorrection ? ArenaRenderUtils::TALL_PIXEL_RATIO : 1.0;

	RenderCamera camera;
	camera.init(chunk, point, direction, fovY, aspectRatio, tallPixelRatio);

	return camera;
}

int RendererUtils::getRenderThreadsFromMode(int mode)
{
	if (mode == 0)
	{
		// Very low.
		return 1;
	}
	else if (mode == 1)
	{
		// Low.
		return std::max(Platform::getThreadCount() / 4, 1);
	}
	else if (mode == 2)
	{
		// Medium.
		return std::max(Platform::getThreadCount() / 2, 1);
	}
	else if (mode == 3)
	{
		// High.
		return std::max((3 * Platform::getThreadCount()) / 4, 1);
	}
	else if (mode == 4)
	{
		// Very high.
		return std::max(Platform::getThreadCount() - 1, 1);
	}
	else if (mode == 5)
	{
		// Max.
		return Platform::getThreadCount();
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(mode));
	}
}

bool RendererUtils::isChasmEmissive(ArenaTypes::ChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaTypes::ChasmType::Dry:
	case ArenaTypes::ChasmType::Wet:
		return false;
	case ArenaTypes::ChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

double RendererUtils::getYShear(Radians angleRadians, double zoom)
{
	return std::tan(angleRadians) * zoom;
}

Double4 RendererUtils::worldSpaceToCameraSpace(const Double4 &point, const Matrix4d &view)
{
	return view * point;
}

Double4 RendererUtils::cameraSpaceToClipSpace(const Double4 &point, const Matrix4d &perspective)
{
	return perspective * point;
}

Double4 RendererUtils::worldSpaceToClipSpace(const Double4 &point, const Matrix4d &transform)
{
	return transform * point;
}

Double3 RendererUtils::clipSpaceToNDC(const Double4 &point)
{
	const double wRecip = 1.0 / point.w;
	return Double3(point.x * wRecip, point.y * wRecip, point.z * wRecip);
}

Double3 RendererUtils::ndcToScreenSpace(const Double3 &point, double yShear, double frameWidth, double frameHeight)
{
	const Double3 screenSpacePoint(
		0.50 - (point.x * 0.50),
		(0.50 + yShear) + (point.y * 0.50),
		point.z);

	return Double3(
		screenSpacePoint.x * frameWidth,
		screenSpacePoint.y * frameHeight,
		screenSpacePoint.z);
}

bool RendererUtils::clipLineSegment(Double4 *p1, Double4 *p2, double *outStart, double *outEnd)
{
	// Trivial case: both points are behind the camera.
	if ((p1->w <= 0.0) && (p2->w <= 0.0))
	{
		return false;
	}

	const bool p1XInsideLeft = (p1->w + p1->x) > 0.0; // In the shaded area above "x + w = 0" line.
	const bool p1YInsideLeft = (p1->w + p1->y) > 0.0; // In the shaded area above "y + w = 0" line.
	const bool p1ZInsideLeft = (p1->w + p1->z) > 0.0; // In the shaded area above "z + w = 0" line.
	const bool p1XInsideRight = (p1->w - p1->x) > 0.0; // In the shaded area above "x - w = 0" line.
	const bool p1YInsideRight = (p1->w - p1->y) > 0.0; // In the shaded area above "y - w = 0" line.
	const bool p1ZInsideRight = (p1->w - p1->z) > 0.0; // In the shaded area above "z - w = 0" line.
	const bool p1XInside = p1XInsideLeft && p1XInsideRight;
	const bool p1YInside = p1YInsideLeft && p1YInsideRight;
	const bool p1ZInside = p1ZInsideLeft && p1ZInsideRight;

	const bool p2XInsideLeft = (p2->w + p2->x) > 0.0; // In the shaded area above "x + w = 0" line.
	const bool p2YInsideLeft = (p2->w + p2->y) > 0.0; // In the shaded area above "y + w = 0" line.
	const bool p2ZInsideLeft = (p2->w + p2->z) > 0.0; // In the shaded area above "z + w = 0" line.
	const bool p2XInsideRight = (p2->w - p2->x) > 0.0; // In the shaded area above "x - w = 0" line.
	const bool p2YInsideRight = (p2->w - p2->y) > 0.0; // In the shaded area above "y - w = 0" line.
	const bool p2ZInsideRight = (p2->w - p2->z) > 0.0; // In the shaded area above "z - w = 0" line.
	const bool p2XInside = p2XInsideLeft && p2XInsideRight;
	const bool p2YInside = p2YInsideLeft && p2YInsideRight;
	const bool p2ZInside = p2ZInsideLeft && p2ZInsideRight;

	// Check both points are off of one side in their {X,Y,Z}W space.
	/*if ((!p1XInsideLeft && !p2XInsideLeft) ||
		(!p1YInsideLeft && !p2YInsideLeft) ||
		(!p1ZInsideLeft && !p2ZInsideLeft) ||
		(!p1XInsideRight && !p2XInsideRight) ||
		(!p1YInsideRight && !p2YInsideRight) ||
		(!p1ZInsideRight && !p2ZInsideRight))
	{
		return false;
	}*/

	// Check line segment intersecting "x + w = 0" line.
	if (!p1XInsideLeft && p2XInside)
	{
		const double t = (p1->w + p1->x) / ((p1->w + p1->x) - (p2->w + p2->x));
		p1->x = p1->x + ((p2->x - p1->x) * t);
		p1->w = p1->w + ((p2->w - p1->w) * t);
		*outStart = t;
		*outEnd = 1.0;
	}

	// Check line segment intersecting "x - w = 0" line.
	if (p1XInside && !p2XInsideRight)
	{
		const double t = (p1->w - p1->x) / ((p1->w - p1->x) - (p2->w - p2->x));
		p2->x = p1->x + ((p2->x - p1->x) * t);
		p2->w = p1->w + ((p2->w - p1->w) * t);
		*outStart = 0.0;
		*outEnd = t;
	}

	// Check line segment intersecting "y + w = 0" line.
	if (!p1YInsideLeft && p2YInside)
	{
		const double t = (p1->w + p1->y) / ((p1->w + p1->y) - (p2->w + p2->y));
		p1->y = p1->y + ((p2->y - p1->y) * t);
		p1->w = p1->w + ((p2->w - p1->w) * t);
		*outStart = t;
		*outEnd = 1.0;
	}

	// Check line segment intersecting "y - w = 0" line.
	if (p1YInside && !p2YInsideRight)
	{
		const double t = (p1->w - p1->y) / ((p1->w - p1->y) - (p2->w - p2->y));
		p2->y = p1->y + ((p2->y - p1->y) * t);
		p2->w = p1->w + ((p2->w - p1->w) * t);
		*outStart = 0.0;
		*outEnd = t;
	}

	// Check line segment intersecting "z + w = 0" line.
	if (!p1ZInsideLeft && p2ZInside)
	{
		const double t = (p1->w + p1->z) / ((p1->w + p1->z) - (p2->w + p2->z));
		p1->z = p1->z + ((p2->z - p1->z) * t);
		p1->w = p1->w + ((p2->w - p1->w) * t);
		*outStart = t;
		*outEnd = 1.0;
	}

	// Check line segment intersecting "z - w = 0" line.
	if (p1ZInside && !p2ZInsideRight)
	{
		const double t = (p1->w - p1->z) / ((p1->w - p1->z) - (p2->w - p2->z));
		p2->z = p1->z + ((p2->z - p1->z) * t);
		p2->w = p1->w + ((p2->w - p1->w) * t);
		*outStart = 0.0;
		*outEnd = t;
	}

	return true;
}

int RendererUtils::getLowerBoundedPixel(double projected, int frameDim)
{
	return std::clamp(static_cast<int>(std::ceil(projected - 0.50)), 0, frameDim);
}

int RendererUtils::getUpperBoundedPixel(double projected, int frameDim)
{
	return std::clamp(static_cast<int>(std::floor(projected + 0.50)), 0, frameDim);
}

Matrix4d RendererUtils::getLatitudeRotation(double latitude)
{
	return Matrix4d::zRotation(latitude * (Constants::Pi / 8.0));
}

Matrix4d RendererUtils::getTimeOfDayRotation(double daytimePercent)
{
	return Matrix4d::xRotation(daytimePercent * Constants::TwoPi);
}

int RendererUtils::getNearestPaletteColorIndex(const Color &color, const Palette &palette)
{
	const Double3 colorRGB = Double3::fromRGB(color.toRGB());

	std::optional<int> nearestIndex;
	for (int i = 0; i < static_cast<int>(palette.size()); i++)
	{
		const Color &paletteColor = palette[i];
		const Double3 paletteColorRGB = Double3::fromRGB(paletteColor.toRGB());

		if (!nearestIndex.has_value())
		{
			nearestIndex = i;
		}
		else
		{
			const Color &currentNearestColor = palette[*nearestIndex];
			const Double3 currentNearestColorRGB = Double3::fromRGB(currentNearestColor.toRGB());

			if ((colorRGB - paletteColorRGB).length() < (colorRGB - currentNearestColorRGB).length())
			{
				nearestIndex = i;
			}
		}
	}

	DebugAssert(nearestIndex.has_value());
	return *nearestIndex;
}
