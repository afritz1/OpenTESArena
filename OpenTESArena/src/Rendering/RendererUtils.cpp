#include <algorithm>
#include <cmath>

#include "RendererUtils.h"
#include "../Math/Constants.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"

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

int RendererUtils::getChasmIdFromType(VoxelDefinition::ChasmData::Type chasmType)
{
	switch (chasmType)
	{
	case VoxelDefinition::ChasmData::Type::Dry:
		return 0;
	case VoxelDefinition::ChasmData::Type::Wet:
		return 1;
	case VoxelDefinition::ChasmData::Type::Lava:
		return 2;
	default:
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(chasmType)));
	}
}

bool RendererUtils::isChasmEmissive(VoxelDefinition::ChasmData::Type chasmType)
{
	switch (chasmType)
	{
	case VoxelDefinition::ChasmData::Type::Dry:
	case VoxelDefinition::ChasmData::Type::Wet:
		return false;
	case VoxelDefinition::ChasmData::Type::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

double RendererUtils::getDoorPercentOpen(int voxelX, int voxelZ,
	const std::vector<LevelData::DoorState> &openDoors)
{
	const Int2 voxel(voxelX, voxelZ);
	const auto iter = std::find_if(openDoors.begin(), openDoors.end(),
		[&voxel](const LevelData::DoorState &openDoor)
	{
		return openDoor.getVoxel() == voxel;
	});

	return (iter != openDoors.end()) ? iter->getPercentOpen() : 0.0;
}

double RendererUtils::getFadingVoxelPercent(int voxelX, int voxelY, int voxelZ,
	const std::vector<LevelData::FadeState> &fadingVoxels)
{
	const Int3 voxel(voxelX, voxelY, voxelZ);

	// find_if was terribly slow in MSVC due to iterators, so using for loop instead.
	for (const LevelData::FadeState &fadeState : fadingVoxels)
	{
		if (fadeState.getVoxel() == voxel)
		{
			return std::clamp(1.0 - fadeState.getPercentDone(), 0.0, 1.0);
		}
	}

	return 1.0;
}

double RendererUtils::getYShear(double angleRadians, double zoom)
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

Quaternion RendererUtils::getLatitudeRotation(double latitude)
{
	return Quaternion::fromAxisAngle(Double3::UnitZ, latitude * (Constants::Pi / 8.0));
}

Quaternion RendererUtils::getTimeOfDayRotation(double daytimePercent)
{
	return Quaternion::fromAxisAngle(Double3::UnitX, daytimePercent * Constants::TwoPi);
}
