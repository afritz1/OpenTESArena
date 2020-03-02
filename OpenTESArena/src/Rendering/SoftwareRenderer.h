#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../Entities/EntityManager.h"
#include "../Game/Options.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../World/DistantSky.h"
#include "../World/LevelData.h"
#include "../World/VoxelDefinition.h"

#include "components/utilities/BufferView.h"

// This class runs the CPU-based 3D rendering for the application.

class Entity;
class Palette;
class VoxelGrid;

class SoftwareRenderer
{
public:
	// Profiling info gathered from internal renderer state.
	struct ProfilerData
	{
		int width, height;
		int visFlatCount, visLightCount;
	};
private:
	struct VoxelTexel
	{
		double r, g, b, emission;
		bool transparent; // Voxel texels only support alpha testing, not alpha blending.

		VoxelTexel();

		static VoxelTexel makeFrom8Bit(uint8_t texel, const Palette &palette);
	};

	struct FlatTexel
	{
		double r, g, b, a;

		FlatTexel();

		static FlatTexel makeFrom8Bit(uint8_t texel, const Palette &palette);
	};

	// For distant sky objects (mountains, clouds, etc.). Although most distant objects
	// only need alpha-testing, some clouds have special case texels for a simple form
	// of transparency.
	struct SkyTexel
	{
		double r, g, b, a;

		SkyTexel();

		static SkyTexel makeFrom8Bit(uint8_t texel, const Palette &palette);
	};

	struct ChasmTexel
	{
		double r, g, b;

		ChasmTexel();

		static ChasmTexel makeFrom8Bit(uint8_t texel, const Palette &palette);
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

	struct ChasmTexture
	{
		static const int WIDTH = 320;
		static const int HEIGHT = 100;
		static const int TEXEL_COUNT = ChasmTexture::WIDTH * ChasmTexture::HEIGHT;

		std::array<ChasmTexel, ChasmTexture::TEXEL_COUNT> texels;
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
		double fovX, fovY; // Horizontal and vertical field of view.
		double zoom, aspect;
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

		// Sky gradient brightness when stars become visible.
		static constexpr double STAR_VIS_THRESHOLD = 64.0 / 255.0;

		// Rotation matrices for distant space objects.
		Matrix4d timeRotation, latitudeRotation;

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

		// Percent through the chasm animation.
		double chasmAnimPercent;

		// Whether the current clock time is before noon.
		bool isAM;

		// Whether street lights and building lights are on.
		bool nightLightsAreActive;

		// Whether the current location is strictly outdoors (does not count outdoor dungeons).
		bool isExterior;

		ShadingInfo(const std::vector<Double3> &skyPalette, double daytimePercent, double latitude,
			double ambient, double fogDistance, double chasmAnimPercent, bool nightLightsAreActive,
			bool isExterior);

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

	// Each .INF flat index has a set of animation state type mappings to groups of texture
	// lists ordered by entity angle.
	class FlatTextureGroup
	{
	public:
		// Angle ID maps to texture list. Only used for insertion, not reading during rendering.
		using TextureList = std::vector<FlatTexture>;
		using AngleGroup = std::vector<std::pair<int, TextureList>>;
		using StateTypeMapping = std::pair<EntityAnimationData::StateType, AngleGroup>;
	private:
		std::vector<StateTypeMapping> stateTypeMappings;

		StateTypeMapping *findMapping(EntityAnimationData::StateType stateType);
		const StateTypeMapping *findMapping(EntityAnimationData::StateType stateType) const;

		static int anglePercentToIndex(const AngleGroup &angleGroup, double anglePercent);

		// Only for inserting textures at initialization.
		static TextureList *findTextureList(AngleGroup &angleGroup, int angleID);
	public:
		// Looks up a texture list by state type and 0->1 angle percent of the entity's direction,
		// where 0 is forward and 1 is all the way around clockwise.
		const TextureList *getTextureList(EntityAnimationData::StateType stateType,
			double anglePercent) const;

		// Adds a texture to the given state type mapping (adding if missing) and angle group.
		void addTexture(EntityAnimationData::StateType stateType, int angleID, bool flipped,
			const uint8_t *srcTexels, int width, int height, const Palette &palette);
	};

	// Each chasm texture group contains one animation's worth of textures.
	using ChasmTextureGroup = std::vector<ChasmTexture>;
	using ChasmTextureGroups = std::unordered_map<int, ChasmTextureGroup>;

	// Visible flat data. A flat is a 2D surface always facing perpendicular to the Y axis,
	// and opposite to the camera's XZ direction.
	struct VisibleFlat
	{
		// Corner points in world space relative to the camera. For texture coordinates, left
		// is inclusive, right is exclusive.
		Double3 topLeft, topRight;
		Double3 bottomLeft, bottomRight;

		// Projected screen-space coordinates.
		double startX, endX;
		double startY, endY;

		// Camera Z for depth sorting.
		double z;

		// Flat texture state. The animation state type determines which texture list to
		// use the texture ID with.
		int flatIndex; // @todo: remove dependency on this. Tightly coupled with .INF flat.
		int textureID;
		double anglePercent; // @todo: remove dependency on this and just use textureID directly.
		EntityAnimationData::StateType animStateType; // @todo: remove dependency on this.
	};

	// Pairs together a distant sky object with its render texture index. If it's an animation,
	// then the index points to the start of its textures.
	template <typename T>
	struct DistantObject
	{
		const T &obj;
		int textureIndex;

		DistantObject(const T &obj, int textureIndex);
	};

	// Collection of all distant objects.
	struct DistantObjects
	{
		// Default index if no sun exists in the world.
		static const int NO_SUN;

		std::vector<DistantObject<DistantSky::LandObject>> lands;
		std::vector<DistantObject<DistantSky::AnimatedLandObject>> animLands;
		std::vector<DistantObject<DistantSky::AirObject>> airs;
		std::vector<DistantObject<DistantSky::MoonObject>> moons;
		std::vector<DistantObject<DistantSky::StarObject>> stars;
		int sunTextureIndex; // Points into skyTextures if the sun exists, or NO_SUN if it doesn't.

		DistantObjects();

		void init(const DistantSky &distantSky, std::vector<SkyTexture> &skyTextures,
			const Palette &palette);

		void clear();
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

	struct VisDistantObjects
	{
		std::vector<VisDistantObject> objs;

		// Need to store start and end indices for each range so we can call different 
		// shading methods on some of them. End indices are exclusive.
		int landStart, landEnd, animLandStart, animLandEnd, airStart, airEnd, moonStart, moonEnd,
			sunStart, sunEnd, starStart, starEnd;

		VisDistantObjects();

		void clear();
	};

	// Instance of an entity light in the world.
	struct VisibleLight
	{
		Double3 position;
		double radius;

		void init(const Double3 &position, double radius);
	};

	// Data about a light and whether it's visible. Used with visible light determination.
	struct LightVisibilityData
	{
		Double3 position;
		double radius;
		bool intersectsFrustum;

		void init(const Double3 &position, double radius, bool intersectsFrustum);
	};

	// Data owned by the main thread that is referenced by render threads.
	struct RenderThreadData
	{
		struct SkyGradient
		{
			int threadsDone;
			std::vector<Double3> *rowCache;
			double projectedYTop, projectedYBottom; // Projected Y range of sky gradient.
			std::atomic<bool> shouldDrawStars; // True if the sky is dark enough.

			void init(double projectedYTop, double projectedYBottom,
				std::vector<Double3> &rowCache);
		};

		struct DistantSky
		{
			int threadsDone;
			const VisDistantObjects *visDistantObjs;
			const std::vector<SkyTexture> *skyTextures;
			bool parallaxSky;
			bool doneVisTesting; // True when render threads can start rendering distant sky.

			void init(bool parallaxSky, const VisDistantObjects &visDistantObjs,
				const std::vector<SkyTexture> &skyTextures);
		};

		struct Voxels
		{
			int threadsDone;
			const std::vector<LevelData::DoorState> *openDoors;
			const std::vector<LevelData::FadeState> *fadingVoxels;
			const std::vector<VisibleLight> *visibleLights;
			const VoxelGrid *voxelGrid;
			const std::vector<VoxelTexture> *voxelTextures;
			const ChasmTextureGroups *chasmTextureGroups;
			std::vector<OcclusionData> *occlusion;
			double ceilingHeight;
			bool doneLightVisTesting; // True when render threads can start rendering voxels.

			void init(double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
				const std::vector<LevelData::FadeState> &fadingVoxels,
				const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
				const std::vector<VoxelTexture> &voxelTextures,
				const ChasmTextureGroups &chasmTextureGroups, std::vector<OcclusionData> &occlusion);
		};

		struct Flats
		{
			int threadsDone;
			const Double3 *flatNormal;
			const std::vector<VisibleFlat> *visibleFlats;
			const std::vector<VisibleLight> *visibleLights;
			const std::unordered_map<int, FlatTextureGroup> *flatTextureGroups;
			bool doneSorting; // True when render threads can start rendering flats.

			void init(const Double3 &flatNormal, const std::vector<VisibleFlat> &visibleFlats,
				const std::vector<VisibleLight> &visibleLights,
				const std::unordered_map<int, FlatTextureGroup> &flatTextureGroups);
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
	//static const int DEFAULT_FLAT_TEXTURE_COUNT;

	// Height ratio between normal pixels and tall pixels.
	static const double TALL_PIXEL_RATIO;

	// Amount of a sliding/raising door that is visible when fully open.
	static const double DOOR_MIN_VISIBLE;

	// Angle of the sky gradient above the horizon, in degrees.
	static const double SKY_GRADIENT_ANGLE;

	// Max angle of distant clouds above the horizon, in degrees.
	static const double DISTANT_CLOUDS_MAX_ANGLE;

	std::vector<double> depthBuffer; // 2D buffer, mostly consists of depth in the XZ plane.
	std::vector<OcclusionData> occlusion; // Min and max Y for each column.
	std::vector<const Entity*> potentiallyVisibleFlats; // Updated every frame.
	std::vector<VisibleFlat> visibleFlats; // Flats to be drawn.
	DistantObjects distantObjects; // Distant sky objects (mountains, clouds, etc.).
	VisDistantObjects visDistantObjs; // Visible distant sky objects.
	std::vector<VisibleLight> visibleLights; // Lights that contribute to the current frame.
	std::vector<VoxelTexture> voxelTextures; // Max 64 voxel textures in original engine.
	std::unordered_map<int, FlatTextureGroup> flatTextureGroups; // Mappings from flat index to textures.
	ChasmTextureGroups chasmTextureGroups; // Mappings from chasm ID to textures.
	std::vector<SkyTexture> skyTextures; // Distant object textures. Size is managed internally.
	std::vector<Double3> skyPalette; // Colors for each time of day.
	std::vector<Double3> skyGradientRowCache; // Contains row colors of most recent sky gradient.
	std::vector<std::thread> renderThreads; // Threads used for rendering the world.
	RenderThreadData threadData; // Managed by main thread, used by render threads.
	double fogDistance; // Distance at which fog is maximum.
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
	void updateVisibleDistantObjects(bool parallaxSky, const ShadingInfo &shadingInfo,
		const Camera &camera, const FrameView &frame);

	// Refreshes the list of flats to be drawn.
	void updateVisibleFlats(const Camera &camera, const ShadingInfo &shadingInfo,
		double ceilingHeight, const VoxelGrid &voxelGrid, const EntityManager &entityManager);
	
	// Gets the facing value for the far side of a chasm.
	static VoxelFacing getInitialChasmFarFacing(int voxelX, int voxelZ,
		const Double2 &eye, const Ray &ray);
	static VoxelFacing getChasmFarFacing(int voxelX, int voxelZ,
		VoxelFacing nearFacing, const Camera &camera, const Ray &ray);

	// Converts the given chasm type to its chasm ID.
	static int getChasmIdFromType(VoxelDefinition::ChasmData::Type chasmType);

	// Returns whether the chasm type is emissive and ignores ambient shading.
	static bool isChasmEmissive(VoxelDefinition::ChasmData::Type chasmType);

	// Tries to convert the chasm animation percent to the associated texture within the chasm
	// texture group for the given chasm type.
	static void getChasmTextureGroupTexture(const ChasmTextureGroups &textureGroups,
		VoxelDefinition::ChasmData::Type chasmType, double chasmAnimPercent,
		const ChasmTexture **outTexture);

	// Gets the percent open of a door, or zero if there's no open door at the given voxel.
	static double getDoorPercentOpen(int voxelX, int voxelZ,
		const std::vector<LevelData::DoorState> &openDoors);

	// Gets the percent fade of a voxel, or 1 if the given voxel is not fading.
	static double getFadingVoxelPercent(int voxelX, int voxelY, int voxelZ,
		const std::vector<LevelData::FadeState> &fadingVoxels);

	// Gets the y-shear value of the camera based on the Y angle relative to the horizon
	// and the zoom of the camera (dependent on vertical field of view).
	static double getYShear(double angleRadians, double zoom);

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

	// Creates a rotation matrix for drawing latitude-correct distant space objects.
	static Matrix4d getLatitudeRotation(double latitude);

	// Creates a rotation matrix for drawing distant space objects relative to the time of day.
	static Matrix4d getTimeOfDayRotation(double daytimePercent);

	// Fills in the two reference parameters with the projected top and bottom Y percent of the sky
	// gradient.
	static void getSkyGradientProjectedYRange(const Camera &camera, double &projectedYTop,
		double &projectedYBottom);

	// Gets the percent of the given projected Y between the top and bottom projected Y values
	// of the sky gradient.
	static double getSkyGradientPercent(double projectedY, double projectedYTop,
		double projectedYBottom);

	// Gets the color of a row in the sky gradient at some percent between the top and bottom.
	static Double3 getSkyGradientRowColor(double gradientPercent, const ShadingInfo &shadingInfo);

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
	static bool findInitialEdgeIntersection(int voxelX, int voxelZ, VoxelFacing edgeFacing,
		bool flipped, const Double2 &nearPoint, const Double2 &farPoint, const Camera &camera,
		const Ray &ray, RayHit &hit);

	// Gathers potential intersection data from a voxel containing an edge ID. The facing
	// determines which edge of the voxel an intersection can occur on. This function is separate
	// from the initial case since it's a trivial solution when the edge and near facings match.
	static bool findEdgeIntersection(int voxelX, int voxelZ, VoxelFacing edgeFacing,
		bool flipped, VoxelFacing nearFacing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearU, const Camera &camera, const Ray &ray, RayHit &hit);

	// Helper method for findInitialDoorIntersection() for swinging doors.
	static bool findInitialSwingingDoorIntersection(int voxelX, int voxelZ, double percentOpen,
		const Double2 &nearPoint, const Double2 &farPoint, bool xAxis, const Camera &camera,
		const Ray &ray, RayHit &hit);
	
	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection.
	static bool findInitialDoorIntersection(int voxelX, int voxelZ,
		VoxelDefinition::DoorData::Type doorType, double percentOpen, const Double2 &nearPoint,
		const Double2 &farPoint, const Camera &camera, const Ray &ray, const VoxelGrid &voxelGrid,
		RayHit &hit);

	// Helper method for findDoorIntersection() for swinging doors.
	static bool findSwingingDoorIntersection(int voxelX, int voxelZ, double percentOpen,
		VoxelFacing nearFacing, const Double2 &nearPoint, const Double2 &farPoint,
		double nearU, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection. Raising doors
	// are always hit, so they do not need a specialized method.
	static bool findDoorIntersection(int voxelX, int voxelZ,
		VoxelDefinition::DoorData::Type doorType, double percentOpen, VoxelFacing nearFacing,
		const Double2 &nearPoint, const Double2 &farPoint, double nearU, RayHit &hit);

	// Calculates light visibility data for a given entity.
	static void getLightVisibilityData(const EntityManager::EntityVisibilityData &visData,
		int lightIntensity, const Double2 &eye2D, const Double2 &cameraDir, double fovX,
		double viewDistance, LightVisibilityData *outVisData);

	// Temporary placeholder until each voxel has its own light list given to it when drawing.
	static UncheckedBufferView<const VisibleLight> getVisibleLightsView(
		const std::vector<VisibleLight> &visLights);

	// Gets the amount of light at a point. Capped at 100% intensity if not unlimited.
	// @todo: replace MaxLights template param with actual given lights so the for loop can be
	// completely inlined for each light count.
	template <int MaxLights, bool CappedSum>
	static double getLightContributionAtPoint(const Double2 &point,
		const UncheckedBufferView<const VisibleLight> &lights);

	// Low-level texture sampling function.
	template <int FilterMode, bool Transparency>
	static void sampleVoxelTexture(const VoxelTexture &texture, double u, double v,
		double *r, double *g, double *b, double *emission, bool *transparent);

	// Low-level screen-space chasm texture sampling function.
	static void sampleChasmTexture(const ChasmTexture &texture, double screenXPercent,
		double screenYPercent, double *r, double *g, double *b);

	// Low-level shader for wall pixel rendering. Template parameters are used for
	// compile-time generation of shader permutations.
	template <bool Fading>
	static void drawPixelsShader(int x, const DrawRange &drawRange, double depth, double u,
		double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
		double fadePercent, double lightContributionPercent, const ShadingInfo &shadingInfo,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels with no perspective or transparency.
	static void drawPixels(int x, const DrawRange &drawRange, double depth, double u,
		double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
		double fadePercent, double lightContributionPercent, const ShadingInfo &shadingInfo,
		OcclusionData &occlusion, const FrameView &frame);

	// Low-level shader for perspective pixel rendering.
	template <bool Fading>
	static void drawPerspectivePixelsShader(int x, const DrawRange &drawRange,
		const Double2 &startPoint, const Double2 &endPoint, double depthStart, double depthEnd,
		const Double3 &normal, const VoxelTexture &texture, double fadePercent,
		const UncheckedBufferView<const VisibleLight> &lights, const ShadingInfo &shadingInfo,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels with perspective but no transparency. The pixel drawing order is 
	// top to bottom, so the start and end values should be passed with that in mind.
	static void drawPerspectivePixels(int x, const DrawRange &drawRange, const Double2 &startPoint,
		const Double2 &endPoint, double depthStart, double depthEnd, const Double3 &normal,
		const VoxelTexture &texture, double fadePercent,
		const UncheckedBufferView<const VisibleLight> &lights, const ShadingInfo &shadingInfo,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels with transparency but no perspective.
	static void drawTransparentPixels(int x, const DrawRange &drawRange, double depth, double u,
		double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
		double lightContributionPercent, const ShadingInfo &shadingInfo,
		const OcclusionData &occlusion, const FrameView &frame);

	// Low-level shader for chasm pixel rendering.
	// @todo: consider template bool for treating screen-space texels as regular texels.
	template <bool AmbientShading, bool TrueDepth>
	static void drawChasmPixelsShader(int x, const DrawRange &drawRange, double depth, double u,
		double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
		const ChasmTexture &chasmTexture, double lightContributionPercent,
		const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of chasm pixels that can either be a wall texture or screen-space texture.
	static void drawChasmPixels(int x, const DrawRange &drawRange, double depth, double u,
		double vStart, double vEnd, const Double3 &normal, bool emissive, const VoxelTexture &texture,
		const ChasmTexture &chasmTexture, double lightContributionPercent,
		const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame);

	// Low-level shader for perspective chasm pixel rendering. This shader only cares about sampling
	// the screen-space texture instead of branching on chasm wall texels.
	// @todo: consider template bool for treating screen-space texels as regular texels.
	template <bool AmbientShading, bool TrueDepth>
	static void drawPerspectiveChasmPixelsShader(int x, const DrawRange &drawRange,
		const Double2 &startPoint, const Double2 &endPoint, double depthStart, double depthEnd,
		const Double3 &normal, const ChasmTexture &texture, const ShadingInfo &shadingInfo,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of chasm pixels with perspective and no transparency. The pixel drawing order
	// is top to bottom, so the start and end values should be passed with that in mind.
	static void drawPerspectiveChasmPixels(int x, const DrawRange &drawRange,
		const Double2 &startPoint, const Double2 &endPoint, double depthStart, double depthEnd,
		const Double3 &normal, bool emissive, const ChasmTexture &texture,
		const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels for a distant sky object (mountain, cloud, etc.). The 'emissive'
	// parameter is for animated objects like volcanoes.
	static void drawDistantPixels(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, bool emissive, const ShadingInfo &shadingInfo,
		const FrameView &frame);
	static void drawDistantPixelsSSE(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, bool emissive, const ShadingInfo &shadingInfo,
		const FrameView &frame);
	/*static void drawDistantPixelsAVX(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, bool emissive, const ShadingInfo &shadingInfo,
		const FrameView &frame);*/

	// Draws a column of pixels for a moon. This is its own pixel-rendering method because of
	// the unique method of shading required for moons.
	static void drawMoonPixels(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, const ShadingInfo &shadingInfo,
		const FrameView &frame);

	// Draws a column of pixels for a star. This is its own pixel-rendering method because of
	// the unique method of shading required for stars.
	static void drawStarPixels(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, const std::vector<Double3> &skyGradientRowCache,
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Helper functions for drawing the initial voxel column.
	static void drawInitialVoxelSameFloor(int x, int voxelX, int voxelY, int voxelZ,
		const Camera &camera, const Ray &ray, VoxelFacing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
		const ShadingInfo &shadingInfo, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);
	static void drawInitialVoxelAbove(int x, int voxelX, int voxelY, int voxelZ,
		const Camera &camera, const Ray &ray, VoxelFacing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
		const ShadingInfo &shadingInfo, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);
	static void drawInitialVoxelBelow(int x, int voxelX, int voxelY, int voxelZ,
		const Camera &camera, const Ray &ray, VoxelFacing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
		const ShadingInfo &shadingInfo, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);

	// Manages drawing voxels in the column that the player is in.
	static void drawInitialVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelFacing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, const ShadingInfo &shadingInfo,
		double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);

	// Helper functions for drawing a voxel column.
	static void drawVoxelSameFloor(int x, int voxelX, int voxelY, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelFacing facing, const Double2 &nearPoint, const Double2 &farPoint,
		double nearZ, double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo,
		double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);
	static void drawVoxelAbove(int x, int voxelX, int voxelY, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelFacing facing, const Double2 &nearPoint, const Double2 &farPoint,
		double nearZ, double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo,
		double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);
	static void drawVoxelBelow(int x, int voxelX, int voxelY, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelFacing facing, const Double2 &nearPoint, const Double2 &farPoint,
		double nearZ, double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo,
		double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);

	// Manages drawing voxels in the column of the given XZ coordinate in the voxel grid.
	static void drawVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
		const Ray &ray, VoxelFacing facing, const Double2 &nearPoint,
		const Double2 &farPoint, double nearZ, double farZ, const ShadingInfo &shadingInfo,
		double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws the portion of a flat contained within the given X range of the screen. The end
	// X value is exclusive.
	static void drawFlat(int startX, int endX, const VisibleFlat &flat,
		const Double3 &normal, const Double2 &eye, const ShadingInfo &shadingInfo, 
		const FlatTexture &texture, const UncheckedBufferView<const VisibleLight> &lights,
		const FrameView &frame);

	// Casts a 2D ray that steps through the current floor, rendering all voxels
	// in the XZ column of each voxel.
	static void rayCast2D(int x, const Camera &camera, const Ray &ray,
		const ShadingInfo &shadingInfo, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &textures, const ChasmTextureGroups &chasmTextureGroups,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws a portion of the sky gradient. The start and end Y are determined from current
	// threading settings.
	static void drawSkyGradient(int startY, int endY, double gradientProjYTop,
		double gradientProjYBottom, std::vector<Double3> &skyGradientRowCache,
		std::atomic<bool> &shouldDrawStars, const ShadingInfo &shadingInfo,
		const FrameView &frame);

	// Draws some columns of distant sky objects (mountains, clouds, etc.). The start and end X
	// are determined from current threading settings.
	static void drawDistantSky(int startX, int endX, bool parallaxSky,
		const VisDistantObjects &visDistantObjs, const std::vector<SkyTexture> &skyTextures,
		const std::vector<Double3> &skyGradientRowCache, bool shouldDrawStars,
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Handles drawing all voxels for the current frame.
	static void drawVoxels(int startX, int stride, const Camera &camera, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const std::vector<VisibleLight> &visibleLights, const VoxelGrid &voxelGrid,
		const std::vector<VoxelTexture> &voxelTextures, const ChasmTextureGroups &chasmTextureGroups,
		std::vector<OcclusionData> &occlusion, const ShadingInfo &shadingInfo, const FrameView &frame);

	// Handles drawing all flats for the current frame.
	static void drawFlats(int startX, int endX, const Camera &camera, const Double3 &flatNormal,
		const std::vector<VisibleFlat> &visibleFlats,
		const std::unordered_map<int, FlatTextureGroup> &flatTextureGroups,
		const ShadingInfo &shadingInfo, const std::vector<VisibleLight> &visibleLights,
		const FrameView &frame);

	// Thread loop for each render thread. All threads are initialized in the constructor and
	// wait for a go signal at the beginning of each render(). If the renderer is destructing,
	// then each render thread still gets a go signal, but they immediately leave their loop
	// and terminate. Non-thread-data parameters are for start/end column/row for each thread.
	static void renderThreadLoop(RenderThreadData &threadData, int threadIndex, int startX,
		int endX, int startY, int endY);
public:
	SoftwareRenderer();
	~SoftwareRenderer();

	bool isInited() const;

	// Gets profiling information about renderer internals.
	ProfilerData getProfilerData() const;

	// Tries to write out selection data for the given entity. Returns whether selection data was
	// successfully written.
	bool tryGetEntitySelectionData(const Double2 &uv, int flatIndex, int textureID,
		double anglePercent, EntityAnimationData::StateType animStateType, bool pixelPerfect,
		bool *outIsSelected) const;

	// Converts a screen point to a ray into the game world.
	static Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
		double fovY, double aspect);

	// Sets the render threads mode to use (low, medium, high, etc.).
	void setRenderThreadsMode(int mode);

	// Adds a light. Causes an error if the ID exists.
	void addLight(int id, const Double3 &point, const Double3 &color, double intensity);

	// Updates various data for a light. If a value doesn't need updating, pass null.
	// Causes an error if no ID matches.
	void updateLight(int id, const Double3 *point, const Double3 *color,
		const double *intensity);

	// Sets the distance at which the fog is maximum.
	void setFogDistance(double fogDistance);

	// Sets textures for the distant sky (mountains, clouds, etc.).
	void setDistantSky(const DistantSky &distantSky, const Palette &palette);

	// Sets the sky palette to use with sky colors based on the time of day.
	// For dungeons, this would probably just be one black pixel.
	void setSkyPalette(const uint32_t *colors, int count);

	// Adds a screen-space chasm texture to the given chasm type's texture list.
	void addChasmTexture(VoxelDefinition::ChasmData::Type chasmType, const uint8_t *colors,
		int width, int height, const Palette &palette);

	// Overwrites the selected voxel texture's data with the given 64x64 set of texels.
	void setVoxelTexture(int id, const uint8_t *srcTexels, const Palette &palette);

	// Adds a flat texture to the given flat's animation texture list at the specified angle
	// group. 8-bit colors with a palette is required here since some palette indices have
	// special behavior for transparency.
	void addFlatTexture(int flatIndex, EntityAnimationData::StateType stateType, int angleID,
		bool flipped, const uint8_t *srcTexels, int width, int height, const Palette &palette);

	// Sets whether night lights and night textures are active. This only needs to be set for
	// exterior locations (i.e., cities and wilderness) because those are the only places
	// with time-dependent light sources and textures.
	void setNightLightsActive(bool active);

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
		double ambient, double daytimePercent, double chasmAnimPercent, double latitude,
		bool parallaxSky, bool nightLightsAreActive, bool isExterior, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels, const VoxelGrid &voxelGrid,
		const EntityManager &entityManager, uint32_t *colorBuffer);
};

#endif
