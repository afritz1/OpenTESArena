#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <array>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../World/DistantSky.h"
#include "../World/LevelData.h"
#include "../World/VoxelData.h"

// This class runs the CPU-based 3D rendering for the application.

class VoxelGrid;

class SoftwareRenderer
{
private:
	struct VoxelTexel
	{
		double r, g, b, emission;
		bool transparent; // Voxel texels only support alpha testing, not alpha blending.

		VoxelTexel();
	};

	struct FlatTexel
	{
		double r, g, b, a;

		FlatTexel();
	};

	// For distant sky objects (mountains, clouds, etc.).
	struct SkyTexel
	{
		double r, g, b;
		bool transparent;

		SkyTexel();
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

		FlatTexture();
	};

	struct SkyTexture
	{
		std::vector<SkyTexel> texels;
		int width, height;

		SkyTexture();
	};

	// Camera for 2.5D ray casting (with some pre-calculated values to avoid duplicating work).
	struct Camera
	{
		Double3 eye; // Camera position.
		Double3 eyeVoxelReal; // 'eye' with each component floored.
		Double3 direction; // 3D direction the camera is facing.
		Int3 eyeVoxel; // 'eyeVoxelReal' converted to integers.
		Matrix4d transform; // Perspective transformation matrix.
		double forwardX, forwardZ; // Forward components.
		double forwardZoomedX, forwardZoomedZ; // Forward * zoom components.
		double rightX, rightZ; // Right components.
		double rightAspectedX, rightAspectedZ; // Right * aspect components.
		double frustumLeftX, frustumLeftZ; // Components of left edge of 2D frustum.
		double frustumRightX, frustumRightZ; // Components of right edge of 2D frustum.
		double fovY, zoom, aspect;
		double yAngleRadians; // Angle of the camera above or below the horizon.
		double yShear; // Projected Y-coordinate translation.

		Camera(const Double3 &eye, const Double3 &direction, double fovY, double aspect,
			double projectionModifier);

		// Gets the angle of the camera's 2D forward vector. 0 is +Z, pi/2 is +X.
		double getXZAngleRadians() const;

		// Gets the camera's Y voxel coordinate after compensating for ceiling height.
		int getAdjustedEyeVoxelY(double ceilingHeight) const;
	};

	// Ray for 2.5D ray casting. The start point is always at the camera's eye.
	struct Ray
	{
		double dirX, dirZ; // Normalized components in XZ plane.

		Ray(double dirX, double dirZ);
	};

	// A draw range contains data for the vertical range that two projected vertices
	// define in screen space.
	struct DrawRange
	{
		double yProjStart, yProjEnd;
		int yStart, yEnd;

		DrawRange(double yProjStart, double yProjEnd, int yStart, int yEnd);
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
		static constexpr int SKY_COLOR_COUNT = 5;

		// Sky colors for the horizon and zenith to interpolate between. Index 0 is the
		// horizon color. For interiors, every color in the array is the same.
		std::array<Double3, SKY_COLOR_COUNT> skyColors;

		// Light and direction of the sun.
		Double3 sunColor, sunDirection;

		// Global ambient light percent.
		double ambient;

		// Ambient light percent used with distant sky objects.
		double distantAmbient;

		// Distance at which fog is maximum.
		double fogDistance;

		// Returns whether the current clock time is before noon.
		bool isAM;

		ShadingInfo(const std::vector<Double3> &skyPalette, double daytimePercent,
			double ambient, double fogDistance);

		const Double3 &getFogColor() const;
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

	// Helper class for visible flat data.
	class VisibleFlat
	{
	private:
		const Flat *flat;
		Flat::Frame frame;
	public:
		VisibleFlat(const Flat &flat, Flat::Frame &&frame);

		const Flat &getFlat() const;
		const Flat::Frame &getFrame() const;
	};

	// Pairs together a distant sky object with its render texture index. If it's an animation,
	// then the index points to the start of its textures.
	struct DistantObject
	{
		enum class Type { Land, AnimatedLand, Air, Space };

		int textureIndex;
		DistantObject::Type type;
		
		union
		{
			const DistantSky::LandObject *land;
			const DistantSky::AnimatedLandObject *animLand;
			const DistantSky::AirObject *air;
			const DistantSky::SpaceObject *space;
		};

		DistantObject(int textureIndex, DistantObject::Type type, const void *obj);
	};

	// A distant object that has been projected on-screen and is at least partially visible.
	struct VisDistantObject
	{
		struct ParallaxData
		{
			// Visible angles (in radians) are the object's angle range clamped within the camera's
			// angle range.
			double xVisAngleStart, xVisAngleEnd;
			double uStart, uEnd;

			ParallaxData();
			ParallaxData(double xVisAngleStart, double xVisAngleEnd, double uStart, double uEnd);
		};

		const SkyTexture *texture;
		DrawRange drawRange;
		ParallaxData parallax;
		double xProjStart, xProjEnd; // Projected screen coordinates.
		int xStart, xEnd; // Pixel coordinates.
		bool emissive; // Only animated lands (i.e., volcanoes) are emissive.

		// Parallax constructor.
		VisDistantObject(const SkyTexture &texture, DrawRange &&drawRange, ParallaxData &&parallax,
			double xProjStart, double xProjEnd, int xStart, int xEnd, bool emissive);

		// Non-parallax constructor.
		VisDistantObject(const SkyTexture &texture, DrawRange &&drawRange, double xProjStart,
			double xProjEnd, int xStart, int xEnd, bool emissive);
	};

	// Data owned by the main thread that is referenced by render threads.
	struct RenderThreadData
	{
		struct SkyGradient
		{
			int threadsDone;

			void init();
		};

		struct DistantSky
		{
			int threadsDone;
			const std::vector<VisDistantObject> *visDistantObjs;
			const std::vector<SkyTexture> *skyTextures;
			bool parallaxSky;
			bool doneVisTesting; // True when render threads can start rendering distant sky.

			void init(bool parallaxSky, const std::vector<VisDistantObject> &visDistantObjs,
				const std::vector<SkyTexture> &skyTextures);
		};

		struct Voxels
		{
			int threadsDone;
			const std::vector<LevelData::DoorState> *openDoors;
			const VoxelGrid *voxelGrid;
			const std::vector<VoxelTexture> *voxelTextures;
			std::vector<OcclusionData> *occlusion;
			double ceilingHeight;

			void init(double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
				const VoxelGrid &voxelGrid, const std::vector<VoxelTexture> &voxelTextures,
				std::vector<OcclusionData> &occlusion);
		};

		struct Flats
		{
			int threadsDone;
			const Double3 *flatNormal;
			const std::vector<VisibleFlat> *visibleFlats;
			const std::vector<FlatTexture> *flatTextures;
			bool doneSorting; // True when render threads can start rendering flats.

			void init(const Double3 &flatNormal, const std::vector<VisibleFlat> &visibleFlats,
				const std::vector<FlatTexture> &flatTextures);
		};

		SkyGradient skyGradient;
		DistantSky distantSky;
		Voxels voxels;
		Flats flats;
		const Camera *camera;
		const ShadingInfo *shadingInfo;
		const FrameView *frame;

		std::condition_variable condVar;
		std::mutex mutex;
		int totalThreads;
		bool go; // Initial go signal to start work each frame.
		bool isDestructing; // Helps shut down threads in the renderer destructor.

		RenderThreadData();

		void init(int totalThreads, const Camera &camera, const ShadingInfo &shadingInfo,
			const FrameView &frame);
	};

	// Clipping planes for Z coordinates.
	static const double NEAR_PLANE;
	static const double FAR_PLANE;

	// Default texture array sizes (using vector instead of array to avoid stack overflow).
	static const int DEFAULT_VOXEL_TEXTURE_COUNT;
	static const int DEFAULT_FLAT_TEXTURE_COUNT;

	// Amount of a sliding/raising door that is visible when fully open.
	static const double DOOR_MIN_VISIBLE;

	// Default index if no sun exists in the world.
	static const int NO_SUN;

	// Angle of the sky gradient above the horizon, in degrees.
	static const double SKY_GRADIENT_ANGLE;

	// Max angle of distant clouds above the horizon, in degrees.
	static const double DISTANT_CLOUDS_MAX_ANGLE;

	std::vector<double> depthBuffer; // 2D buffer, mostly consists of depth in the XZ plane.
	std::vector<OcclusionData> occlusion; // Min and max Y for each column.
	std::unordered_map<int, Flat> flats; // All flats in world.
	std::vector<VisibleFlat> visibleFlats; // Flats to be drawn.
	std::vector<DistantObject> distantObjects; // Distant sky objects (mountains, clouds, etc.).
	std::vector<VisDistantObject> visDistantObjs; // Visible distant sky objects.
	std::vector<VoxelTexture> voxelTextures; // Max 64 voxel textures in original engine.
	std::vector<FlatTexture> flatTextures; // Max 256 flat textures in original engine.
	std::vector<SkyTexture> skyTextures; // Distant object textures. Size is managed internally.
	std::vector<Double3> skyPalette; // Colors for each time of day.
	std::vector<std::thread> renderThreads; // Threads used for rendering the world.
	RenderThreadData threadData; // Managed by main thread, used by render threads.
	double fogDistance; // Distance at which fog is maximum.
	int sunTextureIndex; // Points into skyTextures if the sun exists, or -1 if it doesn't.
	int width, height; // Dimensions of frame buffer.
	int renderThreadsMode; // Determines number of threads to use for rendering.

	// Gets the number of render threads to use based on the given mode.
	static int getRenderThreadsFromMode(int mode);

	// Initializes render threads that run in the background for the duration of the renderer's
	// lifetime. This can also be used to reset threads after a screen resize.
	void initRenderThreads(int width, int height, int threadCount);

	// Turns off each thread in the render threads list peacefully. The render threads are expected
	// to be at their initial wait condition before being given the go + destruct signals.
	void resetRenderThreads();

	// Refreshes the list of distant objects to be drawn.
	void updateVisibleDistantObjects(bool parallaxSky, const Double3 &sunDirection,
		const Camera &camera, const FrameView &frame);

	// Refreshes the list of flats to be drawn.
	void updateVisibleFlats(const Camera &camera);
	
	// Gets the facing value for the far side of a chasm.
	static VoxelData::Facing getInitialChasmFarFacing(int voxelX, int voxelZ,
		const Double2 &eye, const Ray &ray);
	static VoxelData::Facing getChasmFarFacing(int voxelX, int voxelZ,
		VoxelData::Facing nearFacing, const Camera &camera, const Ray &ray);

	// Gets the percent open of a door, or zero if there's no open door at the given voxel.
	static double getDoorPercentOpen(int voxelX, int voxelZ,
		const std::vector<LevelData::DoorState> &openDoors);

	// Calculates the projected Y coordinate of a 3D point given a transform and Y-shear value.
	static double getProjectedY(const Double3 &point, const Matrix4d &transform, double yShear);

	// Gets the pixel coordinate with the nearest available pixel center based on the projected
	// value and some bounding rule. This is used to keep integer drawing ranges clamped in such
	// a way that they never allow sampling of texture coordinates outside of the 0->1 range.
	static int getLowerBoundedPixel(double projected, int frameDim);
	static int getUpperBoundedPixel(double projected, int frameDim);

	// Generates a vertical draw range on-screen from two vertices in world space.
	static DrawRange makeDrawRange(const Double3 &startPoint, const Double3 &endPoint,
		const Camera &camera, const FrameView &frame);

	// Generates two vertical draw ranges on-screen from three vertices in world space,
	// sharing some calculations between them and preventing gaps.
	static std::array<DrawRange, 2> makeDrawRangeTwoPart(const Double3 &startPoint,
		const Double3 &midPoint, const Double3 &endPoint, const Camera &camera,
		const FrameView &frame);

	// Generates three vertical draw ranges on-screen from four vertices in world space,
	// sharing some calculations between them and preventing gaps.
	static std::array<DrawRange, 3> makeDrawRangeThreePart(const Double3 &startPoint,
		const Double3 &midPoint1, const Double3 &midPoint2, const Double3 &endPoint,
		const Camera &camera, const FrameView &frame);

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
		bool flipped, const Double2 &nearPoint, const Double2 &farPoint, const Camera &camera,
		const Ray &ray, RayHit &hit);

	// Gathers potential intersection data from a voxel containing an edge ID. The facing
	// determines which edge of the voxel an intersection can occur on. This function is separate
	// from the initial case since it's a trivial solution when the edge and near facings match.
	static bool findEdgeIntersection(int voxelX, int voxelZ, VoxelData::Facing edgeFacing,
		bool flipped, VoxelData::Facing nearFacing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearU, const Camera &camera, const Ray &ray, RayHit &hit);

	// Helper method for findInitialDoorIntersection() for swinging doors.
	static bool findInitialSwingingDoorIntersection(int voxelX, int voxelZ, double percentOpen,
		const Double2 &nearPoint, const Double2 &farPoint, bool xAxis, const Camera &camera,
		const Ray &ray, RayHit &hit);
	
	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection.
	static bool findInitialDoorIntersection(int voxelX, int voxelZ,
		VoxelData::DoorData::Type doorType, double percentOpen, const Double2 &nearPoint,
		const Double2 &farPoint, const Camera &camera, const Ray &ray, const VoxelGrid &voxelGrid,
		RayHit &hit);

	// Helper method for findDoorIntersection() for swinging doors.
	static bool findSwingingDoorIntersection(int voxelX, int voxelZ, double percentOpen,
		VoxelData::Facing nearFacing, const Double2 &nearPoint, const Double2 &farPoint,
		double nearU, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection. Raising doors
	// are always hit, so they do not need a specialized method.
	static bool findDoorIntersection(int voxelX, int voxelZ, VoxelData::DoorData::Type doorType,
		double percentOpen, VoxelData::Facing nearFacing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearU, RayHit &hit);

	// Casts a 3D ray from the default start point (eye) and returns the color.
	// (Unused for now; keeping for reference).
	//Double3 castRay(const Double3 &direction, const VoxelGrid &voxelGrid) const;

	// Draws a column of pixels with no perspective or transparency.
	static void drawPixels(int x, const DrawRange &drawRange, double depth, double u,
		double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
		const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels with perspective but no transparency. The pixel drawing order is 
	// top to bottom, so the start and end values should be passed with that in mind.
	static void drawPerspectivePixels(int x, const DrawRange &drawRange, const Double2 &startPoint,
		const Double2 &endPoint, double depthStart, double depthEnd, const Double3 &normal,
		const VoxelTexture &texture, const ShadingInfo &shadingInfo, OcclusionData &occlusion,
		const FrameView &frame);

	// Draws a column of pixels with transparency but no perspective.
	static void drawTransparentPixels(int x, const DrawRange &drawRange, double depth, double u,
		double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
		const ShadingInfo &shadingInfo, const OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels for a distant sky object (mountain, cloud, etc.). The 'emissive'
	// parameter is for animated objects like volcanoes.
	static void drawDistantPixels(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, bool emissive, const ShadingInfo &shadingInfo,
		const FrameView &frame);

	// Manages drawing voxels in the column that the player is in.
	static void drawInitialVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelData::Facing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, const ShadingInfo &shadingInfo,
		double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
		const VoxelGrid &voxelGrid, const std::vector<VoxelTexture> &textures,
		OcclusionData &occlusion, const FrameView &frame);

	// Manages drawing voxels in the column of the given XZ coordinate in the voxel grid.
	static void drawVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelData::Facing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, const ShadingInfo &shadingInfo,
		double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
		const VoxelGrid &voxelGrid, const std::vector<VoxelTexture> &textures,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws the portion of a flat contained within the given X range of the screen. The end
	// X value is exclusive.
	static void drawFlat(int startX, int endX, const Flat::Frame &flatFrame, 
		const Double3 &normal, bool flipped, const Double2 &eye, const ShadingInfo &shadingInfo, 
		const FlatTexture &texture, const FrameView &frame);

	// @todo: drawAlphaFlat(...), for flats with partial transparency.
	// - Must be back to front.

	// Casts a 2D ray that steps through the current floor, rendering all voxels
	// in the XZ column of each voxel.
	static void rayCast2D(int x, const Camera &camera, const Ray &ray,
		const ShadingInfo &shadingInfo, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, OcclusionData &occlusion,
		const FrameView &frame);

	// Draws a portion of the sky gradient. The start and end Y are determined from current
	// threading settings.
	static void drawSkyGradient(int startY, int endY, const Camera &camera, 
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Draws some columns of distant sky objects (mountains, clouds, etc.). The start and end X
	// are determined from current threading settings.
	static void drawDistantSky(int startX, int endX, bool parallaxSky,
		const std::vector<VisDistantObject> &visDistantObjs, const Camera &camera,
		const std::vector<SkyTexture> &skyTextures, const ShadingInfo &shadingInfo,
		const FrameView &frame);

	// Handles drawing all voxels for the current frame.
	static void drawVoxels(int startX, int stride, const Camera &camera, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &voxelTextures, std::vector<OcclusionData> &occlusion,
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Handles drawing all flats for the current frame.
	static void drawFlats(int startX, int endX, const Camera &camera, const Double3 &flatNormal,
		const std::vector<VisibleFlat> &visibleFlats, const std::vector<FlatTexture> &flatTextures,
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Thread loop for each render thread. All threads are initialized in the constructor and
	// wait for a go signal at the beginning of each render(). If the renderer is destructing,
	// then each render thread still gets a go signal, but they immediately leave their loop
	// and terminate. Non-thread-data parameters are for start/end column/row for each thread.
	static void renderThreadLoop(RenderThreadData &threadData, int threadIndex, int startX,
		int endX, int startY, int endY);
public:
	SoftwareRenderer();
	~SoftwareRenderer();

	// Height ratio between normal pixels and tall pixels.
	static const double TALL_PIXEL_RATIO;

	bool isInited() const;

	// Sets the render threads mode to use (low, medium, high, etc.).
	void setRenderThreadsMode(int mode);

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

	// Sets textures for the distant sky (mountains, clouds, etc.).
	void setDistantSky(const DistantSky &distantSky);

	// Sets the sky palette to use with sky colors based on the time of day.
	// For dungeons, this would probably just be one black pixel.
	void setSkyPalette(const uint32_t *colors, int count);

	// Overwrites the selected voxel texture's data with the given 64x64 set of texels.
	void setVoxelTexture(int id, const uint32_t *srcTexels);

	// Overwrites the selected flat texture's data with the given texels and dimensions.
	void setFlatTexture(int id, const uint32_t *srcTexels, int width, int height);

	// Sets whether night lights and night textures are active. This only needs to be set for
	// exterior locations (i.e., cities and wilderness) because those are the only places
	// with time-dependent light sources and textures.
	void setNightLightsActive(bool active);

	// Removes a flat. Causes an error if no ID matches.
	void removeFlat(int id);

	// Removes a light. Causes an error if no ID matches.
	void removeLight(int id);

	// Zeroes out all renderer textures.
	void clearTextures();

	// Removes all distant sky objects.
	void clearDistantSky();

	// Initializes software renderer with the given frame buffer dimensions. This can be called
	// on first start or to reset the software renderer.
	void init(int width, int height, int renderThreadsMode);

	// Resizes the frame buffer and related values.
	void resize(int width, int height);

	// Draws the scene to the output color buffer in ARGB8888 format.
	void render(const Double3 &eye, const Double3 &direction, double fovY,
		double ambient, double daytimePercent, bool parallaxSky, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
		uint32_t *colorBuffer);
};

#endif
