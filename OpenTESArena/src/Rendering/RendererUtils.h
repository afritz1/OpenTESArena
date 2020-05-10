#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include <vector>

#include "../Math/Matrix4.h"
#include "../Math/Quaternion.h"
#include "../World/LevelData.h"
#include "../World/VoxelDefinition.h"

namespace RendererUtils
{
	// Gets the number of render threads to use based on the given mode.
	int getRenderThreadsFromMode(int mode);

	// Converts the given chasm type to its chasm ID.
	int getChasmIdFromType(VoxelDefinition::ChasmData::Type chasmType);

	// Returns whether the chasm type is emissive and ignores ambient shading.
	bool isChasmEmissive(VoxelDefinition::ChasmData::Type chasmType);

	// Gets the percent open of a door, or zero if there's no open door at the given voxel.
	double getDoorPercentOpen(int voxelX, int voxelZ, const std::vector<LevelData::DoorState> &openDoors);

	// Gets the percent fade of a voxel, or 1 if the given voxel is not fading.
	double getFadingVoxelPercent(int voxelX, int voxelY, int voxelZ,
		const std::vector<LevelData::FadeState> &fadingVoxels);

	// Gets the y-shear value of the camera based on the Y angle relative to the horizon
	// and the zoom of the camera (dependent on vertical field of view).
	double getYShear(double angleRadians, double zoom);

	// Calculates the projected Y coordinate of a 3D point given a transform and Y-shear value.
	double getProjectedY(const Double3 &point, const Matrix4d &transform, double yShear);

	// Gets the pixel coordinate with the nearest available pixel center based on the projected
	// value and some bounding rule. This is used to keep integer drawing ranges clamped in such
	// a way that they never allow sampling of texture coordinates outside of the 0->1 range.
	int getLowerBoundedPixel(double projected, int frameDim);
	int getUpperBoundedPixel(double projected, int frameDim);

	// Creates a rotation quaternion for drawing latitude-correct distant space objects.
	Quaternion getLatitudeRotation(double latitude);

	// Creates a rotation quaternion for drawing distant space objects relative to the time of day.
	Quaternion getTimeOfDayRotation(double daytimePercent);
}

#endif
