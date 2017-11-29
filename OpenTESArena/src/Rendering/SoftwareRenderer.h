#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"

// This class runs the CPU-based 3D rendering for the application.

class VoxelData;
class VoxelGrid;

class SoftwareRenderer
{
private:
	// This determines which axis a wall side is facing towards on the outside. Only necessary 
	// for the sides of walls because floor and ceiling normals can be inferred trivially.
	enum class WallFacing { PositiveX, NegativeX, PositiveZ, NegativeZ };

	struct SoftwareTexture
	{
		std::vector<Double4> pixels;
		int width, height;
		bool containsTransparency; // For occlusion culling.
	};

	struct DiagonalHit
	{
		// Inner Z is the distance from the near point to the intersection point.
		double innerZ, u;
		Double2 point;
		Double3 normal;
	};

	// Helper struct for keeping shading data organized in the renderer. These values are
	// computed once per frame.
	struct ShadingInfo
	{
		// Sky colors for the horizon and zenith to interpolate between. Also used for fog.
		Double3 horizonSkyColor, zenithSkyColor;

		// Light and direction of the sun.
		Double3 sunColor, sunDirection;

		// Global ambient light percent.
		double ambient;

		// Distance at which fog is maximum.
		double fogDistance;

		ShadingInfo(const Double3 &horizonSkyColor, const Double3 &zenithSkyColor,
			const Double3 &sunColor, const Double3 &sunDirection, double ambient,
			double fogDistance);
	};

	// A flat is a 2D surface always facing perpendicular to the Y axis, and opposite to
	// the camera's XZ direction.
	struct Flat
	{
		Double3 position; // Center of bottom edge.
		double width, height;
		int textureID;
		bool flipped;

		// A flat's frame consists of their four corner points in world space, and some
		// screen-space values.
		struct Frame
		{
			// Each "start" is the flat's right or top, and each "end" is the opposite side.
			// For texture coordinates, start is inclusive, end is exclusive.
			Double3 topStart, topEnd, bottomStart, bottomEnd;

			// Screen-space coordinates of the flat. The X values determine which columns the 
			// flat occupies on-screen, and the Y values determine which rows.
			double startX, endX, startY, endY;

			// Depth of the flat in camera space. Intended only for depth sorting, since the 
			// renderer uses true XZ depth with each pixel column instead.
			double z;
		};
	};

	// Clipping planes for Z coordinates.
	static const double NEAR_PLANE;
	static const double FAR_PLANE;

	// A value just below one for keeping texture coordinates from overflowing.
	static const double JUST_BELOW_ONE;

	std::vector<double> depthBuffer; // 2D buffer, mostly consists of depth in the XZ plane.
	std::unordered_map<int, Flat> flats; // All flats in world.
	std::vector<std::pair<const Flat*, Flat::Frame>> visibleFlats; // Flats to be drawn.
	std::vector<SoftwareTexture> textures;
	std::vector<Double3> skyPalette; // Colors for each time of day.
	double fogDistance; // Distance at which fog is maximum.
	int width, height; // Dimensions of frame buffer.
	int renderThreadCount; // Number of threads to use for rendering.

	// Gets the fog color (based on the time of day). It returns a value instead of
	// a reference because it interpolates between two colors for a smoother transition.
	Double3 getFogColor(double daytimePercent) const;

	// Gets the current sun direction based on the time of day.
	Double3 getSunDirection(double daytimePercent) const;

	// Gets the wall normal associated with a wall facing.
	static Double3 getWallNormal(WallFacing wallFacing);

	// Calculates the projected Y coordinate of a 3D point given a transform and Y-shear value.
	static double getProjectedY(const Double3 &point, const Matrix4d &transform, double yShear);

	// Use this to gather potential intersection data from a voxel containing a non-zero ID for
	// "diagonal 1"; the diagonal starting at (nearX, nearZ) and ending at (farX, farZ). Returns 
	// whether an intersection occurred within the voxel.
	static bool findDiag1Intersection(int voxelX, int voxelZ, const Double2 &nearPoint,
		const Double2 &farPoint, DiagonalHit &hit);

	// Use this to gather potential intersection data from a voxel containing a non-zero ID for
	// "diagonal 2"; the diagonal starting at (farX, nearZ) and ending at (nearX, farZ). Returns 
	// whether an intersection occurred within the voxel.
	static bool findDiag2Intersection(int voxelX, int voxelZ, const Double2 &nearPoint,
		const Double2 &farPoint, DiagonalHit &hit);

	// Calculates all the projection data for a diagonal wall after successful intersection
	// and assigns it to various reference variables.
	static void diagonalProjection(double voxelYReal, const VoxelData &voxelData, 
		const Double2 &point, const Matrix4d &transform, double yShear, int frameHeight, 
		double heightReal, double &diagTopScreenY, double &diagBottomScreenY, int &diagStart, 
		int &diagEnd);

	// Casts a 3D ray from the default start point (eye) and returns the color.
	// (Unused for now; keeping for reference).
	//Double3 castRay(const Double3 &direction, const VoxelGrid &voxelGrid) const;

	// Draws a column of wall pixels.
	static void drawWall(int x, int yStart, int yEnd, double projectedYStart, 
		double projectedYEnd, double z, double u, double topV, double bottomV,
		const Double3 &normal, const SoftwareTexture &texture, const ShadingInfo &shadingInfo,
		int frameWidth, int frameHeight, double *depthBuffer, uint32_t *colorBuffer);

	// Draws a column of floor or ceiling pixels. The pixel drawing order is always
	// top to bottom, so the start and end points should be passed with that in mind.
	static void drawFloorOrCeiling(int x, int yStart, int yEnd, double projectedYStart, 
		double projectedYEnd, const Double2 &startPoint, const Double2 &endPoint, 
		double startZ, double endZ, const Double3 &normal, const SoftwareTexture &texture,
		const ShadingInfo &shadingInfo, int frameWidth, int frameHeight, double *depthBuffer, 
		uint32_t *colorBuffer);
	
	// Manages drawing voxels in the column that the player is in.
	static void drawInitialVoxelColumn(int x, int voxelX, int voxelZ, double playerY,
		WallFacing wallFacing, const Double2 &nearPoint, const Double2 &farPoint, double nearZ,
		double farZ, const Matrix4d &transform, double yShear, const ShadingInfo &shadingInfo, 
		const VoxelGrid &voxelGrid, const std::vector<SoftwareTexture> &textures, int frameWidth,
		int frameHeight, double *depthBuffer, uint32_t *colorBuffer);

	// Manages drawing voxels in the column of the given XZ coordinate in the voxel grid.
	static void drawVoxelColumn(int x, int voxelX, int voxelZ, double playerY,
		WallFacing wallFacing, const Double2 &nearPoint, const Double2 &farPoint, double nearZ,
		double farZ, const Matrix4d &transform, double yShear, const ShadingInfo &shadingInfo, 
		const VoxelGrid &voxelGrid, const std::vector<SoftwareTexture> &textures, int frameWidth,
		int frameHeight, double *depthBuffer, uint32_t *colorBuffer);

	// Draws the portion of a flat contained within the given X range of the screen. The end
	// X value is exclusive.
	static void drawFlat(int startX, int endX, const Flat::Frame &flatFrame, 
		const Double3 &normal, bool flipped, const Double2 &eye, const ShadingInfo &shadingInfo, 
		const SoftwareTexture &texture, int frameWidth, int frameHeight, 
		double *depthBuffer, uint32_t *colorBuffer);

	// Casts a 2D ray that steps through the current floor, rendering all voxels
	// in the XZ column of each voxel.
	void rayCast2D(int x, const Double3 &eye, const Double2 &direction,
		const Matrix4d &transform, double yShear, const ShadingInfo &shadingInfo, 
		const VoxelGrid &voxelGrid, uint32_t *colorBuffer);

	// Refreshes the list of flats to be drawn.
	void updateVisibleFlats(const Double2 &eye, const Double2 &direction,
		const Matrix4d &transform, double yShear, double aspect, double zoom);
public:
	SoftwareRenderer(int width, int height);
	~SoftwareRenderer();

	// Adds a flat. Causes an error if the ID exists.
	void addFlat(int id, const Double3 &position, double width, double height, int textureID);

	// Adds a light. Causes an error if the ID exists.
	void addLight(int id, const Double3 &point, const Double3 &color, double intensity);

	// Adds a texture and returns its assigned ID (index).
	int addTexture(const uint32_t *pixels, int width, int height);

	// Updates various data for a flat. If a value doesn't need updating, pass null.
	// Causes an error if no ID matches.
	void updateFlat(int id, const Double3 *position, const double *width, 
		const double *height, const int *textureID, const bool *flipped);

	// Updates various data for a light. If a value doesn't need updating, pass null.
	// Causes an error if no ID matches.
	void updateLight(int id, const Double3 *point, const Double3 *color,
		const double *intensity);

	// Sets the distance at which the fog is maximum.
	void setFogDistance(double fogDistance);

	// Sets the sky palette to use with sky colors based on the time of day.
	// For dungeons, this would probably just be one black pixel.
	void setSkyPalette(const uint32_t *colors, int count);

	// Removes a flat. Causes an error if no ID matches.
	void removeFlat(int id);

	// Removes a light. Causes an error if no ID matches.
	void removeLight(int id);

	// Removes all textures from the renderer. Useful when changing to a new map.
	// (Individual textures can't be removed due to the simple array implementation
	// and small API).
	void removeAllTextures();

	// Resizes the frame buffer and related values.
	void resize(int width, int height);

	// Draws the scene to the output color buffer in ARGB8888 format.
	void render(const Double3 &eye, const Double3 &direction, double fovY, 
		double ambient, double daytimePercent, const VoxelGrid &voxelGrid, 
		uint32_t *colorBuffer);
};

#endif
