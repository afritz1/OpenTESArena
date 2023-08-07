#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include <array>
#include <optional>
#include <vector>

#include "../Assets/ArenaTypes.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"
#include "../Utilities/Color.h"
#include "../Utilities/Palette.h"
#include "../Voxels/VoxelUtils.h"

#include "components/utilities/BufferView.h"

class VoxelChunk;
class WeatherInstance;

struct RenderCamera;

namespace RendererUtils
{
	constexpr double NEAR_PLANE = 0.001;
	constexpr double FAR_PLANE = 1000.0;

	// Vertices used with fog geometry in screen-space around the player.
	constexpr int FOG_GEOMETRY_VERTEX_COUNT = 8;
	constexpr int FOG_GEOMETRY_INDEX_COUNT = 16;
	constexpr int FOG_GEOMETRY_INDICES_PER_QUAD = 4;
	using FogVertexArray = std::array<Double3, FOG_GEOMETRY_VERTEX_COUNT>; // Corners of a cube.
	using FogIndexArray = std::array<int, FOG_GEOMETRY_INDEX_COUNT>; // 4 faces.

	RenderCamera makeCamera(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction,
		Degrees fovY, double aspectRatio, bool tallPixelCorrection);

	// Gets the number of render threads to use based on the given mode.
	int getRenderThreadsFromMode(int mode);

	// Converts the given chasm type to its chasm ID.
	int getChasmIdFromType(ArenaTypes::ChasmType chasmType);

	// Returns whether the chasm type is emissive and ignores ambient shading.
	bool isChasmEmissive(ArenaTypes::ChasmType chasmType);

	// Gets the world space coordinates of the given voxel's four corners from a top-down perspective.
	void getVoxelCorners2D(SNInt voxelX, WEInt voxelZ, WorldDouble2 *outTopLeftCorner,
		WorldDouble2 *outTopRightCorner, WorldDouble2 *outBottomLeftCorner, WorldDouble2 *outBottomRightCorner);

	// Gets the world space coordinates of the given diagonal voxel's three points (beginning, middle,
	// and end point). Diag1 includes the top right and bottom left corners, diag2 includes the top left
	// and bottom right corners.
	void getDiag1Points2D(SNInt voxelX, WEInt voxelZ, WorldDouble2 *outStart,
		WorldDouble2 *outMiddle, WorldDouble2 *outEnd);
	void getDiag2Points2D(SNInt voxelX, WEInt voxelZ, WorldDouble2 *outStart,
		WorldDouble2 *outMiddle, WorldDouble2 *outEnd);

	// Gets the percent open of a door, or zero if there's no open door at the given voxel.
	double getDoorPercentOpen(SNInt voxelX, WEInt voxelZ, const VoxelChunk &chunk);

	// Gets the percent fade of a voxel, or 1 if the given voxel is not fading.
	double getFadingVoxelPercent(SNInt voxelX, int voxelY, WEInt voxelZ, const VoxelChunk &chunk);

	// Gets the y-shear value of the camera based on the Y angle relative to the horizon
	// and the zoom of the camera (dependent on vertical field of view).
	double getYShear(Radians angleRadians, double zoom);

	// Converts a 3D point or vector in world space to camera space (where Z distance to vertices is relevant).
	// The W component of the point/vector matters (point=1, vector=0)!
	Double4 worldSpaceToCameraSpace(const Double4 &point, const Matrix4d &view);

	// Projects a 3D point or vector in camera space to clip space (homogeneous coordinates; does not divide by W).
	Double4 cameraSpaceToClipSpace(const Double4 &point, const Matrix4d &perspective);

	// Projects a 3D point or vector in world space to clip space (homogeneous coordinates; does not divide by W).
	// The given transformation matrix is the product of a model, view, and perspective matrix. This function
	// combines the camera space step for convenience.
	Double4 worldSpaceToClipSpace(const Double4 &point, const Matrix4d &transform);

	// Converts a point in homogeneous coordinates to normalized device coordinates by dividing by W.
	Double3 clipSpaceToNDC(const Double4 &point);

	// Converts a point in normalized device coordinates to screen space (pixel coordinates with fractions in
	// the decimals; the space expected by pixel shading). In other 3D engines this extra step might not be needed
	// but I think I'm doing something different, can't remember.
	// - In theory, this would return an Int2+double later on if sub-pixel precision worked with integers instead.
	Double3 ndcToScreenSpace(const Double3 &point, double yShear, double frameWidth, double frameHeight);

	// Modifies the given clip space line segment so it fits in the frustum. Returns whether the line
	// segment is visible at all (false means that the line segment was completely clipped away).
	// The output parameters are the revised p1 and p2 points, and revisions to the 0->1 percent
	// of the line connecting the original points together.
	bool clipLineSegment(Double4 *p1, Double4 *p2, double *outStart, double *outEnd);

	// Gets the pixel coordinate with the nearest available pixel center based on the projected
	// value and some bounding rule. This is used to keep integer drawing ranges clamped in such
	// a way that they never allow sampling of texture coordinates outside of the 0->1 range.
	int getLowerBoundedPixel(double projected, int frameDim);
	int getUpperBoundedPixel(double projected, int frameDim);

	// Creates a rotation matrix for drawing latitude-correct distant space objects.
	Matrix4d getLatitudeRotation(double latitude);

	// Creates a rotation matrix for drawing distant space objects relative to the time of day.
	Matrix4d getTimeOfDayRotation(double daytimePercent);

	// Gets the color of the sun for shading with.
	Double3 getSunColor(const Double3 &sunDirection, bool isExterior);

	// Generates the sky colors from the horizon to the top of the sky. The number of colors to
	// make is variable but five seems enough for clean gradient generation across the screen.
	void writeSkyColors(BufferView<const Double3> skyColors, BufferView<Double3> &outSkyColorsView, double daytimePercent);

	// Returns whether the given percent through the day is before noon. This affects
	// the sliding window direction of the sky palette.
	bool isBeforeNoon(double daytimePercent);

	// Gets the thunderstorm flash percent if it's raining and a thunderstorm is present.
	std::optional<double> getThunderstormFlashPercent(const WeatherInstance &weatherInst);

	// Gets the lightning bolt percent if it's a thunderstorm and a lightning bolt is present.
	std::optional<double> getLightningBoltPercent(const WeatherInstance &weatherInst);

	// Gets the palette index of the color that most closely matches the given one.
	int getNearestPaletteColorIndex(const Color &color, const Palette &palette);

	// Gets the vertex buffer and index buffer for screen-space fog. Every quad is ordered in
	// UV space, where (0, 0) is the top left and (1, 0) is the top right.
	void getFogGeometry(FogVertexArray *outVertices, FogIndexArray *outIndices);
}

#endif
