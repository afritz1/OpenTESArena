#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include <vector>

#include "../Assets/ArenaTypes.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelUtils.h"

#include "components/utilities/BufferView.h"

class LevelData;

namespace RendererUtils
{
	// Gets the number of render threads to use based on the given mode.
	int getRenderThreadsFromMode(int mode);

	// Converts the given chasm type to its chasm ID.
	int getChasmIdFromType(ArenaTypes::ChasmType chasmType);

	// Returns whether the chasm type is emissive and ignores ambient shading.
	bool isChasmEmissive(ArenaTypes::ChasmType chasmType);

	// Gets the world space coordinates of the given voxel's four corners from a top-down perspective.
	void getVoxelCorners2D(SNInt voxelX, WEInt voxelZ, NewDouble2 *outTopLeftCorner,
		NewDouble2 *outTopRightCorner, NewDouble2 *outBottomLeftCorner, NewDouble2 *outBottomRightCorner);

	// Gets the world space coordinates of the given diagonal voxel's three points (beginning, middle,
	// and end point). Diag1 includes the top right and bottom left corners, diag2 includes the top left
	// and bottom right corners.
	void getDiag1Points2D(SNInt voxelX, WEInt voxelZ, NewDouble2 *outStart,
		NewDouble2 *outMiddle, NewDouble2 *outEnd);
	void getDiag2Points2D(SNInt voxelX, WEInt voxelZ, NewDouble2 *outStart,
		NewDouble2 *outMiddle, NewDouble2 *outEnd);

	// Gets the percent open of a door, or zero if there's no open door at the given voxel.
	double getDoorPercentOpen(SNInt voxelX, WEInt voxelZ, const LevelData &levelData);

	// Gets the percent fade of a voxel, or 1 if the given voxel is not fading.
	double getFadingVoxelPercent(SNInt voxelX, int voxelY, WEInt voxelZ, const LevelData &levelData);

	// Gets the y-shear value of the camera based on the Y angle relative to the horizon
	// and the zoom of the camera (dependent on vertical field of view).
	double getYShear(Radians angleRadians, double zoom);

	// Calculates the projected Y coordinate of a 3D point given a transform and Y-shear value.
	double getProjectedY(const Double3 &point, const Matrix4d &transform, double yShear);

	// Gets the pixel coordinate with the nearest available pixel center based on the projected
	// value and some bounding rule. This is used to keep integer drawing ranges clamped in such
	// a way that they never allow sampling of texture coordinates outside of the 0->1 range.
	int getLowerBoundedPixel(double projected, int frameDim);
	int getUpperBoundedPixel(double projected, int frameDim);

	// Creates a rotation matrix for drawing latitude-correct distant space objects.
	Matrix4d getLatitudeRotation(double latitude);

	// Creates a rotation matrix for drawing distant space objects relative to the time of day.
	Matrix4d getTimeOfDayRotation(double daytimePercent);

	// Gets the direction towards the sun. The sun rises in the west and sets in the east.
	Double3 getSunDirection(const Matrix4d &timeRotation, double latitude);

	// Gets the color of the sun for shading with.
	Double3 getSunColor(const Double3 &sunDirection, bool isExterior);

	// Generates the sky colors from the horizon to the top of the sky. The number of colors to
	// make is variable but five seems enough for clean gradient generation across the screen.
	void writeSkyColors(const std::vector<Double3> &skyPalette,
		BufferView<Double3> &skyColors, double daytimePercent);

	// Gets the ambient percent applied to distant sky as a function of global ambient.
	double getDistantAmbientPercent(double ambientPercent);

	// Returns whether the given percent through the day is before noon. This affects
	// the sliding window direction of the sky palette.
	bool isBeforeNoon(double daytimePercent);
}

#endif
