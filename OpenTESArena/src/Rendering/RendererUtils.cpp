#include <algorithm>
#include <cmath>
#include <tuple>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "RendererUtils.h"
#include "../Math/BoundingBox.h"
#include "../Math/Constants.h"
#include "../Utilities/Platform.h"
#include "../Voxels/VoxelChunk.h"
#include "../Weather/WeatherInstance.h"

#include "components/debug/Debug.h"

double RendererUtils::getTallPixelRatio(bool useTallPixelCorrection)
{
	if (useTallPixelCorrection)
	{
		return ArenaRenderUtils::TALL_PIXEL_RATIO;
	}

	return 1.0;
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

double RendererUtils::getYShear(Radians angleRadians, double zoom)
{
	return std::tan(angleRadians) * zoom;
}

Double4 RendererUtils::worldSpaceToCameraSpace(const Double4 &point, const Matrix4d &view)
{
	return view * point;
}

Double4 RendererUtils::cameraSpaceToClipSpace(const Double4 &point, const Matrix4d &projection)
{
	return projection * point;
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

Double2 RendererUtils::ndcToScreenSpace(const Double3 &point, double frameWidth, double frameHeight)
{
	const Double2 screenSpacePoint(
		0.50 + (point.x * 0.50),
		0.50 - (point.y * 0.50));

	return Double2(
		screenSpacePoint.x * frameWidth,
		screenSpacePoint.y * frameHeight);
}

int RendererUtils::getLowerBoundedPixelAligned(double projected, int frameDim, int alignment)
{
	const int pixel = static_cast<int>(std::ceil(projected - 0.50));
	const int alignedPixel = MathUtils::roundToLesserMultipleOf(pixel, alignment);
	return std::clamp(alignedPixel, 0, frameDim);
}

int RendererUtils::getLowerBoundedPixel(double projected, int frameDim)
{
	return RendererUtils::getLowerBoundedPixelAligned(projected, frameDim, 1);
}

int RendererUtils::getUpperBoundedPixelAligned(double projected, int frameDim, int alignment)
{
	const int pixel = static_cast<int>(std::floor(projected + 0.50));
	const int alignedPixel = MathUtils::roundToGreaterMultipleOf(pixel, alignment);
	return std::clamp(alignedPixel, 0, frameDim);
}

int RendererUtils::getUpperBoundedPixel(double projected, int frameDim)
{
	return RendererUtils::getUpperBoundedPixelAligned(projected, frameDim, 1);
}

Matrix4d RendererUtils::getLatitudeRotation(double latitude)
{
	return Matrix4d::zRotation(latitude * (Constants::Pi / 8.0));
}

Matrix4d RendererUtils::getTimeOfDayRotation(double dayPercent)
{
	return Matrix4d::xRotation(dayPercent * Constants::TwoPi);
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

void RendererUtils::getBBoxVisibilityInFrustum(const BoundingBox3D &bbox, const WorldDouble3 &frustumPoint, const Double3 &frustumForward,
	const Double3 &frustumNormalLeft, const Double3 &frustumNormalRight, const Double3 &frustumNormalBottom, const Double3 &frustumNormalTop,
	bool *outIsCompletelyVisible, bool *outIsCompletelyInvisible)
{
	// Each plane to test the bounding box against.
	const Double3 frustumNormals[] =
	{
		frustumForward,
		frustumNormalLeft,
		frustumNormalRight,
		frustumNormalBottom,
		frustumNormalTop
	};

	constexpr int bboxCornerCount = 8;
	const WorldDouble3 bboxCorners[bboxCornerCount] =
	{
		bbox.min,
		bbox.min + WorldDouble3(bbox.width, 0.0, 0.0),
		bbox.min + WorldDouble3(0.0, bbox.height, 0.0),
		bbox.min + WorldDouble3(bbox.width, bbox.height, 0.0),
		bbox.min + WorldDouble3(0.0, 0.0, bbox.depth),
		bbox.min + WorldDouble3(bbox.width, 0.0, bbox.depth),
		bbox.min + WorldDouble3(0.0, bbox.height, bbox.depth),
		bbox.max
	};

	bool isCompletelyVisible = true;
	bool isCompletelyInvisible = false;
	for (const Double3 &frustumNormal : frustumNormals)
	{
		int insidePoints = 0;
		int outsidePoints = 0;
		for (const WorldDouble3 &cornerPoint : bboxCorners)
		{
			const double dist = MathUtils::distanceToPlane(cornerPoint, frustumPoint, frustumNormal);
			if (dist >= 0.0)
			{
				insidePoints++;
			}
			else
			{
				outsidePoints++;
			}
		}

		if (insidePoints < bboxCornerCount)
		{
			isCompletelyVisible = false;
		}

		if (outsidePoints == bboxCornerCount)
		{
			isCompletelyInvisible = true;
			break;
		}
	}

	*outIsCompletelyVisible = isCompletelyVisible;
	*outIsCompletelyInvisible = isCompletelyInvisible;
}

void RendererUtils::getBBoxVisibilityInFrustum(const BoundingBox3D &bbox, const RenderCamera &camera,
	bool *outIsCompletelyVisible, bool *outIsCompletelyInvisible)
{
	RendererUtils::getBBoxVisibilityInFrustum(bbox, camera.worldPoint, camera.forward, camera.leftFrustumNormal, camera.rightFrustumNormal,
		camera.bottomFrustumNormal, camera.topFrustumNormal, outIsCompletelyVisible, outIsCompletelyInvisible);
}

Matrix4f RendererUtils::matrix4DoubleToFloat(const Matrix4d &matrix)
{
	Matrix4f mat4f;
	mat4f.x = Float4(static_cast<float>(matrix.x.x), static_cast<float>(matrix.x.y), static_cast<float>(matrix.x.z), static_cast<float>(matrix.x.w));
	mat4f.y = Float4(static_cast<float>(matrix.y.x), static_cast<float>(matrix.y.y), static_cast<float>(matrix.y.z), static_cast<float>(matrix.y.w));
	mat4f.z = Float4(static_cast<float>(matrix.z.x), static_cast<float>(matrix.z.y), static_cast<float>(matrix.z.z), static_cast<float>(matrix.z.w));
	mat4f.w = Float4(static_cast<float>(matrix.w.x), static_cast<float>(matrix.w.y), static_cast<float>(matrix.w.z), static_cast<float>(matrix.w.w));
	return mat4f;
}
