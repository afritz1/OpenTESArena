#include <algorithm>
#include <cmath>

#include "RendererUtils.h"
#include "../Game/CardinalDirection.h"
#include "../Math/Constants.h"
#include "../Utilities/Platform.h"
#include "../World/Chunk.h"

#include "components/debug/Debug.h"

void RendererUtils::LoadedEntityTextureEntry::init(TextureAssetReference &&textureAssetRef, bool flipped,
	bool reflective)
{
	this->textureAssetRef = std::move(textureAssetRef);
	this->flipped = flipped;
	this->reflective = reflective;
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

int RendererUtils::getChasmIdFromType(ArenaTypes::ChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaTypes::ChasmType::Dry:
		return 0;
	case ArenaTypes::ChasmType::Wet:
		return 1;
	case ArenaTypes::ChasmType::Lava:
		return 2;
	default:
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(chasmType)));
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

void RendererUtils::getVoxelCorners2D(SNInt voxelX, WEInt voxelZ, NewDouble2 *outTopLeftCorner,
	NewDouble2 *outTopRightCorner, NewDouble2 *outBottomLeftCorner, NewDouble2 *outBottomRightCorner)
{
	// In the +X south/+Z west coordinate system, the top right of a voxel is its origin.
	*outTopRightCorner = NewDouble2(static_cast<SNDouble>(voxelX), static_cast<WEDouble>(voxelZ));
	*outTopLeftCorner = *outTopRightCorner + CardinalDirection::West;
	*outBottomRightCorner = *outTopRightCorner + CardinalDirection::South;
	*outBottomLeftCorner = *outTopRightCorner + CardinalDirection::West + CardinalDirection::South;
}

void RendererUtils::getDiag1Points2D(SNInt voxelX, WEInt voxelZ, NewDouble2 *outStart,
	NewDouble2 *outMiddle, NewDouble2 *outEnd)
{
	// Top right to bottom left.
	const NewDouble2 diff = CardinalDirection::South + CardinalDirection::West;
	*outStart = NewDouble2(static_cast<SNDouble>(voxelX), static_cast<WEDouble>(voxelZ));
	*outMiddle = *outStart + (diff * 0.50);
	*outEnd = *outStart + (diff * Constants::JustBelowOne);
}

void RendererUtils::getDiag2Points2D(SNInt voxelX, WEInt voxelZ, NewDouble2 *outStart,
	NewDouble2 *outMiddle, NewDouble2 *outEnd)
{
	// Bottom right to top left.
	const NewDouble2 diff = CardinalDirection::North + CardinalDirection::West;
	*outStart = NewDouble2(
		static_cast<SNDouble>(voxelX) + Constants::JustBelowOne,
		static_cast<WEDouble>(voxelZ));
	*outMiddle = *outStart + (diff * 0.50);
	*outEnd = *outStart + (diff * Constants::JustBelowOne);
}

double RendererUtils::getDoorPercentOpen(SNInt voxelX, WEInt voxelZ, const Chunk &chunk)
{
	const VoxelInt3 voxel(voxelX, 1, voxelZ);
	const VoxelInstance *voxelInst = chunk.tryGetVoxelInst(voxel, VoxelInstance::Type::OpenDoor);
	if (voxelInst != nullptr)
	{
		const VoxelInstance::DoorState &doorState = voxelInst->getDoorState();
		return doorState.getPercentOpen();
	}
	else
	{
		return 0.0;
	}
}

double RendererUtils::getFadingVoxelPercent(SNInt voxelX, int voxelY, WEInt voxelZ, const Chunk &chunk)
{
	const VoxelInt3 voxel(voxelX, voxelY, voxelZ);
	const VoxelInstance *voxelInst = chunk.tryGetVoxelInst(voxel, VoxelInstance::Type::Fading);
	if (voxelInst != nullptr)
	{
		const VoxelInstance::FadeState &fadeState = voxelInst->getFadeState();
		return std::clamp(1.0 - fadeState.getPercentFaded(), 0.0, 1.0);
	}
	else
	{
		return 1.0;
	}
}

double RendererUtils::getYShear(Radians angleRadians, double zoom)
{
	return std::tan(angleRadians) * zoom;
}

double RendererUtils::getProjectedY(const Double3 &point, const Matrix4d &transform, double yShear)
{
	// Just do 3D projection for the Y and W coordinates instead of a whole
	// matrix * vector4 multiplication to keep from doing some unnecessary work.
	double projectedY, projectedW;
	transform.ywMultiply(point, projectedY, projectedW);

	// Convert the projected Y to normalized coordinates.
	projectedY /= projectedW;

	// Calculate the Y position relative to the center row of the screen, and offset it by 
	// the Y-shear. Multiply by 0.5 for the correct aspect ratio.
	return (0.50 + yShear) - (projectedY * 0.50);
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

Double3 RendererUtils::getSunColor(const Double3 &sunDirection, bool isExterior)
{
	if (isExterior)
	{
		// @todo: model this better/differently?
		const Double3 baseSunColor(0.90, 0.875, 0.85); // Arbitrary value.

		// Darken the sun color if it's below the horizon so wall faces aren't lit 
		// as much during the night. This is just a made-up artistic value to compensate
		// for the lack of shadows.
		return (sunDirection.y >= 0.0) ? baseSunColor :
			(baseSunColor * (1.0 - (5.0 * std::abs(sunDirection.y)))).clamped();
	}
	else
	{
		// No sunlight indoors.
		return Double3::Zero;
	}
}

void RendererUtils::writeSkyColors(const std::vector<Double3> &skyColors,
	BufferView<Double3> &outSkyColorsView, double daytimePercent)
{
	// The "sliding window" of sky colors is backwards in the AM (horizon is latest in the palette)
	// and forwards in the PM (horizon is earliest in the palette).
	const bool isAM = RendererUtils::isBeforeNoon(daytimePercent);
	const int slideDirection = isAM ? -1 : 1;

	// Get the real index (not the integer index) of the color for the current time as a
	// reference point so each sky color can be interpolated between two samples.
	const int skyColorCount = static_cast<int>(skyColors.size());
	const double realIndex = MathUtils::getRealIndex(skyColorCount, daytimePercent);
	const double percent = realIndex - std::floor(realIndex);

	// Calculate sky colors based on the time of day.
	for (int i = 0; i < outSkyColorsView.getCount(); i++)
	{
		const int indexDiff = slideDirection * i;
		const int index = MathUtils::getWrappedIndex(skyColorCount, static_cast<int>(realIndex) + indexDiff);
		const int nextIndex = MathUtils::getWrappedIndex(skyColorCount, index + slideDirection);
		const Double3 &color = skyColors.at(index);
		const Double3 &nextColor = skyColors.at(nextIndex);
		const Double3 skyColor = color.lerp(nextColor, isAM ? (1.0 - percent) : percent);
		outSkyColorsView.set(i, skyColor);
	}
}

double RendererUtils::getDistantAmbientPercent(double ambientPercent)
{
	// At their darkest, distant objects are ~1/4 of their intensity.
	return std::clamp(ambientPercent, 0.25, 1.0);
}

bool RendererUtils::isBeforeNoon(double daytimePercent)
{
	return daytimePercent < 0.50;
}
