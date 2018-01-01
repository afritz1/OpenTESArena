#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../World/VoxelData.h"

// This class runs the CPU-based 3D rendering for the application.

class VoxelGrid;

class SoftwareRenderer
{
private:
	struct VoxelTexel
	{
		double r, g, b, a, emission;

		VoxelTexel();
	};

	struct FlatTexel
	{
		double r, g, b, a;

		FlatTexel();
	};

	struct VoxelTexture
	{
		static const int WIDTH = 64;
		static const int HEIGHT = VoxelTexture::WIDTH;
		static const int TEXEL_COUNT = VoxelTexture::WIDTH * VoxelTexture::HEIGHT;

		std::array<VoxelTexel, VoxelTexture::TEXEL_COUNT> texels;
		std::vector<Int2> lightTexels; // Black during the day, yellow at night.
	};

	struct FlatTexture
	{
		std::vector<FlatTexel> texels;
		int width, height;
	};

	// Camera for 2.5D ray casting (with some pre-calculated values to avoid duplicating work).
	struct Camera
	{
		Double3 eye; // Camera position.
		Double3 eyeVoxelReal; // 'eye' with each component floored.
		Int3 eyeVoxel; // 'eyeVoxelReal' converted to integers.
		Matrix4d transform; // Perspective transformation matrix.
		double forwardX, forwardZ; // Forward components.
		double rightX, rightZ; // Right components.
		double fovY, zoom, aspect;
		double yShear; // Projected Y-coordinate translation.

		Camera(const Double3 &eye, const Double3 &direction, double fovY, double aspect);
	};

	// Ray for 2.5D ray casting. The start point is always at the camera's eye.
	struct Ray
	{
		double dirX, dirZ; // Normalized components in XZ plane.

		Ray(double dirX, double dirZ);
	};

	// Occlusion defines a "drawing window" that shrinks as opaque pixels are drawn in each 
	// column of the screen. Drawing ranges lying within an occluded area are thrown out, 
	// and partially occluded ranges are clipped. If an entire column is occluded, the ray 
	// casting loop can return early.
	struct OcclusionData
	{
		// Min is inclusive, max is exclusive.
		int yMin, yMax;

		OcclusionData(int yMin, int yMax);
		OcclusionData();

		// Modifies the given start and end pixel coordinates based on the current occlusion.
		// This will either keep (yEnd - yStart) the same or less than it was before.
		void clipRange(int *yStart, int *yEnd) const;

		// Updates the occlusion range given some range of opaque pixels.
		void update(int yStart, int yEnd);
	};

	// Helper struct for ray search operations (i.e., finding if a ray intersects a 
	// diagonal line segment).
	struct RayHit
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

	// Helper struct for values related to the frame buffer. The pointers are owned
	// elsewhere; they are copied here simply for convenience.
	struct FrameView
	{
		uint32_t *colorBuffer;
		double *depthBuffer;
		int width, height;
		double widthReal, heightReal;

		FrameView(uint32_t *colorBuffer, double *depthBuffer, int width, int height);
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

	typedef std::array<VoxelTexture, 64> VoxelTextureArray;
	typedef std::array<FlatTexture, 256> FlatTextureArray;

	// Clipping planes for Z coordinates.
	static const double NEAR_PLANE;
	static const double FAR_PLANE;

	std::vector<double> depthBuffer; // 2D buffer, mostly consists of depth in the XZ plane.
	std::vector<OcclusionData> occlusion; // Min and max Y for each column.
	std::unordered_map<int, Flat> flats; // All flats in world.
	std::vector<std::pair<const Flat*, Flat::Frame>> visibleFlats; // Flats to be drawn.
	VoxelTextureArray voxelTextures;
	FlatTextureArray flatTextures;
	std::vector<Double3> skyPalette; // Colors for each time of day.
	double fogDistance; // Distance at which fog is maximum.
	int width, height; // Dimensions of frame buffer.
	int renderThreadCount; // Number of threads to use for rendering.

	// Gets the fog color (based on the time of day). It returns a value instead of
	// a reference because it interpolates between two colors for a smoother transition.
	Double3 getFogColor(double daytimePercent) const;

	// Gets the current sun direction based on the time of day.
	Double3 getSunDirection(double daytimePercent) const;

	// A variant of atan2() with a range of [0, 2pi] instead of [-pi, pi].
	static double fullAtan2(double y, double x);

	// Gets the normal associated with a facing.
	static Double3 getNormal(VoxelData::Facing facing);

	// Gets the facing value for the far side of a chasm.
	static VoxelData::Facing getInitialChasmFarFacing(int voxelX, int voxelZ,
		const Double2 &eye, const Ray &ray);
	static VoxelData::Facing getChasmFarFacing(int voxelX, int voxelZ,
		VoxelData::Facing nearFacing, const Camera &camera, const Ray &ray);

	// Calculates the projected Y coordinate of a 3D point given a transform and Y-shear value.
	static double getProjectedY(const Double3 &point, const Matrix4d &transform, double yShear);

	// Gets the pixel coordinate with the nearest available pixel center based on the projected
	// value and some bounding rule. This is used to keep integer drawing ranges clamped in such
	// a way that they never allow sampling of texture coordinates outside of the 0->1 range.
	static int getLowerBoundedPixel(double projected, int frameDim);
	static int getUpperBoundedPixel(double projected, int frameDim);

	// Gathers potential intersection data from a voxel containing a "diagonal 1" ID; the 
	// diagonal starting at (nearX, nearZ) and ending at (farX, farZ). Returns whether an 
	// intersection occurred within the voxel.
	static bool findDiag1Intersection(int voxelX, int voxelZ, const Double2 &nearPoint,
		const Double2 &farPoint, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a "diagonal 2" ID; the
	// diagonal starting at (farX, nearZ) and ending at (nearX, farZ). Returns whether an
	// intersection occurred within the voxel.
	static bool findDiag2Intersection(int voxelX, int voxelZ, const Double2 &nearPoint,
		const Double2 &farPoint, RayHit &hit);

	// Gathers potential intersection data from an initial voxel containing an edge ID. The
	// facing determines which edge of the voxel an intersection can occur on.
	static bool findInitialEdgeIntersection(int voxelX, int voxelZ, VoxelData::Facing edgeFacing,
		const Double2 &nearPoint, const Double2 &farPoint, const Camera &camera,
		const Ray &ray, RayHit &hit);

	// Gathers potential intersection data from a voxel containing an edge ID. The facing
	// determines which edge of the voxel an intersection can occur on.. This function is separate
	// from the initial case since it's a trivial solution when the edge and near facings match.
	static bool findEdgeIntersection(int voxelX, int voxelZ, VoxelData::Facing edgeFacing,
		VoxelData::Facing nearFacing, const Double2 &nearPoint, const Double2 &farPoint,
		double nearU, const Camera &camera, const Ray &ray, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for intersection.
	static bool findDoorIntersection(int voxelX, int voxelZ, VoxelData::DoorData::Type doorType,
		const Double2 &nearPoint, const Double2 &farPoint, RayHit &hit);

	// Calculates all the projection data for a diagonal wall after successful intersection
	// and assigns it to various reference variables. This method assumes that all diagonals
	// appear only on the main floor.
	static void diagProjection(double voxelYReal, double voxelHeight, const Double2 &point,
		const Matrix4d &transform, double yShear, int frameHeight, double heightReal, 
		double &diagTopScreenY, double &diagBottomScreenY, int &diagStart, int &diagEnd);

	// Casts a 3D ray from the default start point (eye) and returns the color.
	// (Unused for now; keeping for reference).
	//Double3 castRay(const Double3 &direction, const VoxelGrid &voxelGrid) const;

	// Draws a column of pixels with no perspective or transparency.
	static void drawPixels(int x, int yStart, int yEnd, double projectedYStart,
		double projectedYEnd, double depth, double u, double vStart, double vEnd,
		const Double3 &normal, const VoxelTexture &texture, const ShadingInfo &shadingInfo, 
		OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels with perspective but no transparency. The pixel drawing order is 
	// top to bottom, so the start and end values should be passed with that in mind.
	static void drawPerspectivePixels(int x, int yStart, int yEnd, double projectedYStart,
		double projectedYEnd, const Double2 &startPoint, const Double2 &endPoint,
		double depthStart, double depthEnd, const Double3 &normal, const VoxelTexture &texture,
		const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels with transparency but no perspective.
	static void drawTransparentPixels(int x, int yStart, int yEnd, double projectedYStart,
		double projectedYEnd, double depth, double u, double vStart, double vEnd,
		const Double3 &normal, const VoxelTexture &texture, const ShadingInfo &shadingInfo,
		const OcclusionData &occlusion, const FrameView &frame);

	// Manages drawing voxels in the column that the player is in.
	static void drawInitialVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelData::Facing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, const ShadingInfo &shadingInfo,
		double ceilingHeight, const VoxelGrid &voxelGrid,
		const VoxelTextureArray &textures, OcclusionData &occlusion, const FrameView &frame);

	// Manages drawing voxels in the column of the given XZ coordinate in the voxel grid.
	static void drawVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelData::Facing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, const ShadingInfo &shadingInfo,
		double ceilingHeight, const VoxelGrid &voxelGrid, const VoxelTextureArray &textures,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws the portion of a flat contained within the given X range of the screen. The end
	// X value is exclusive.
	static void drawFlat(int startX, int endX, const Flat::Frame &flatFrame, 
		const Double3 &normal, bool flipped, const Double2 &eye, const ShadingInfo &shadingInfo, 
		const FlatTexture &texture, const FrameView &frame);

	// To do: drawAlphaFlat(...), for flats with partial transparency.
	// - Must be back to front.

	// Casts a 2D ray that steps through the current floor, rendering all voxels
	// in the XZ column of each voxel.
	static void rayCast2D(int x, const Camera &camera, const Ray &ray,
		const ShadingInfo &shadingInfo, double ceilingHeight, const VoxelGrid &voxelGrid, 
		const VoxelTextureArray &textures, OcclusionData &occlusion, const FrameView &frame);

	// Refreshes the list of flats to be drawn.
	void updateVisibleFlats(const Camera &camera);
public:
	SoftwareRenderer(int width, int height);
	~SoftwareRenderer();

	// Adds a flat. Causes an error if the ID exists.
	void addFlat(int id, const Double3 &position, double width, double height, int textureID);

	// Adds a light. Causes an error if the ID exists.
	void addLight(int id, const Double3 &point, const Double3 &color, double intensity);

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

	// Overwrites the selected voxel texture's data with the given 64x64 set of texels.
	void setVoxelTexture(int id, const uint32_t *srcTexels);

	// Overwrites the selected flat texture's data with the given set of texels and dimensions.
	void setFlatTexture(int id, const uint32_t *srcTexels, int width, int height);

	// Sets whether night lights and night textures are active.
	void setNightLightsActive(bool active);

	// Removes a flat. Causes an error if no ID matches.
	void removeFlat(int id);

	// Removes a light. Causes an error if no ID matches.
	void removeLight(int id);

	// Zeroes out all voxel and flat textures.
	void clearTextures();

	// Resizes the frame buffer and related values.
	void resize(int width, int height);

	// Draws the scene to the output color buffer in ARGB8888 format.
	void render(const Double3 &eye, const Double3 &direction, double fovY, 
		double ambient, double daytimePercent, double ceilingHeight,
		const VoxelGrid &voxelGrid, uint32_t *colorBuffer);
};

#endif
