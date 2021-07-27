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

#include "RendererSystem3D.h"
#include "../Assets/ArenaTypes.h"
#include "../Entities/EntityManager.h"
#include "../Game/Options.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Media/Palette.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelUtils.h"

#include "components/utilities/Buffer2D.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView2D.h"

// CPU-based 2.5D rendering.

class Chunk;
class ChunkManager;
class Entity;

enum class VoxelFacing2D;

class SoftwareRenderer : public RendererSystem3D
{
private:
	struct VoxelTexel
	{
		double r, g, b, emission;
		bool transparent; // Only supports alpha testing, not alpha blending.

		void init(double r, double g, double b, double emission, bool transparent);
	};

	struct FlatTexel
	{
		uint8_t value;

		void init(uint8_t value);
	};

	// For distant sky objects (mountains, clouds, etc.). Although most distant objects
	// only need alpha-testing, some clouds have special case texels for a simple form
	// of transparency.
	struct SkyTexel
	{
		double r, g, b, a;

		void init(double r, double g, double b, double a);
	};

	struct ChasmTexel
	{
		double r, g, b;

		void init(double r, double g, double b);
	};

	struct VoxelTexture
	{
		std::vector<VoxelTexel> texels;
		std::vector<Int2> lightTexels; // Black during the day, yellow at night.
		// @todo: replace lightTexels with two VoxelTextures: one for day, one for night.
		int width, height;

		VoxelTexture();

		void init(int width, int height, const uint8_t *srcTexels, const Palette &palette);
		void setLightTexelsActive(bool active, const Palette &palette);
	};

	struct FlatTexture
	{
		std::vector<FlatTexel> texels;
		int width, height;
		bool reflective;

		FlatTexture();

		void init(int width, int height, const uint8_t *srcTexels, bool flipped, bool reflective);
	};

	struct SkyTexture
	{
		std::vector<SkyTexel> texels;
		int width, height;

		SkyTexture();

		void init(int width, int height, const uint8_t *srcTexels, const Palette &palette);
	};

	struct ChasmTexture
	{
		std::vector<ChasmTexel> texels;
		int width, height;

		ChasmTexture();

		void init(int width, int height, const uint8_t *srcTexels, const Palette &palette);
	};

	// @temp: this is a temporary solution to voxel texture allocation management -- ideally the renderer
	// would take texture builders and return texture handles and those would be bound to instance voxel
	// geometry.
	struct VoxelTextureMapping
	{
		TextureAssetReference textureAssetRef;
		int textureIndex; // Index in voxel textures list.

		VoxelTextureMapping(TextureAssetReference &&textureAssetRef, int textureIndex);
	};

	struct VoxelTextures
	{
		std::vector<VoxelTexture> textures;
		std::vector<VoxelTextureMapping> mappings;

		void addTexture(VoxelTexture &&texture, TextureAssetReference &&textureAssetRef);

		const VoxelTexture &getTexture(const TextureAssetReference &textureAssetRef) const;

		void clear();
	};

	// @temp: this is a temporary solution to entity texture allocation management -- ideally the renderer
	// would take texture builders and return texture handles and those would be bound to instance entity
	// geometry.
	struct EntityTextureMapping
	{
		TextureAssetReference textureAssetRef;
		bool flipped;
		bool reflective;
		int textureIndex; // Index in entity textures list.

		EntityTextureMapping(TextureAssetReference &&textureAssetRef, bool flipped, bool reflective, int textureIndex);
	};

	struct EntityTextures
	{
		std::vector<FlatTexture> textures;
		std::vector<EntityTextureMapping> mappings;

		void addTexture(FlatTexture &&texture, TextureAssetReference &&textureAssetRef, bool flipped, bool reflective);

		const FlatTexture &getTexture(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective) const;

		void clear();
	};

	// Camera for 2.5D ray casting (with some pre-calculated values to avoid duplicating work).
	struct Camera
	{
		// Eye position values. Slight redundancy in cached values for convenience.
		CoordDouble3 eye; // Camera position.
		CoordDouble3 eyeVoxelReal; // 'eye' with each component floored.
		CoordInt3 eyeVoxel; // 'eyeVoxelReal' converted to integers.
		VoxelDouble3 direction; // 3D direction the camera is facing.
		Matrix4d transform; // Perspective transformation matrix.
		
		// Forward components.
		SNDouble forwardX;
		WEDouble forwardZ;

		// Forward * zoom components.
		SNDouble forwardZoomedX;
		WEDouble forwardZoomedZ;

		// Right components.
		SNDouble rightX;
		WEDouble rightZ;

		// Right * aspect components.
		SNDouble rightAspectedX;
		WEDouble rightAspectedZ;

		// Components of left edge of 2D frustum.
		SNDouble frustumLeftX;
		WEDouble frustumLeftZ;

		// Components of right edge of 2D frustum.
		SNDouble frustumRightX;
		WEDouble frustumRightZ;

		Degrees fovX, fovY; // Horizontal and vertical field of view.
		double zoom, aspect;
		Radians yAngleRadians; // Angle of the camera above or below the horizon.
		double yShear; // Projected Y-coordinate translation.
		double horizonProjY; // Projected Y coordinate of horizon.

		Camera(const CoordDouble3 &eye, const VoxelDouble3 &direction, Degrees fovY, double aspect,
			double projectionModifier);

		// Gets the angle of the camera's 2D forward vector. 0 is -Z, pi/2 is -X.
		Radians getXZAngleRadians() const;

		// Gets the camera's Y voxel coordinate after compensating for ceiling height.
		int getAdjustedEyeVoxelY(double ceilingScale) const;
	};

	// Ray for 2.5D ray casting. The start point is always at the camera's eye.
	struct Ray
	{
		// Normalized components in XZ plane.
		SNDouble dirX;
		WEDouble dirZ;

		Ray(SNDouble dirX, WEDouble dirZ);
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
		NewDouble2 point; // @todo: change to CoordDouble2
		Double3 normal;
	};

	// Helper struct for keeping shading data organized in the renderer. These values are
	// computed once per frame.
	struct ShadingInfo
	{
		static constexpr int SKY_COLOR_COUNT = 5;
		static constexpr int THUNDERSTORM_COLOR_COUNT = 3;

		// Sky gradient brightness when stars become visible.
		static constexpr double STAR_VIS_THRESHOLD = 64.0 / 255.0;

		// The palette used for converting 8-bit texels to true color.
		Palette palette;

		// Sky colors for the horizon and zenith to interpolate between. Index 0 is the
		// horizon color. For interiors, every color in the array is the same.
		std::array<Double3, SKY_COLOR_COUNT> skyColors;

		// Thunderstorm flash colors for the sky. Index 0 is the start of the flash.
		std::array<Double3, THUNDERSTORM_COLOR_COUNT> thunderstormColors;
		std::optional<double> thunderstormFlashPercent;

		// Global ambient light percent.
		double ambient;

		// Ambient light percent used with distant sky objects.
		double distantAmbient;

		// Distance at which fog is maximum.
		double fogDistance;

		// Percent through the chasm animation.
		double chasmAnimPercent;

		// Whether street lights and building lights are on.
		bool nightLightsAreActive;

		// Whether the current location is strictly outdoors (does not count outdoor dungeons).
		bool isExterior;

		// Whether the player has a light attached like the original game.
		bool playerHasLight;

		ShadingInfo(const Palette &palette, const std::vector<Double3> &skyColors, const WeatherInstance &weatherInst,
			double daytimePercent, double latitude, double ambient, double fogDistance, double chasmAnimPercent,
			bool nightLightsAreActive, bool isExterior, bool playerHasLight);

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
		double aspectRatio;

		FrameView(uint32_t *colorBuffer, double *depthBuffer, int width, int height);
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
		NewDouble3 topLeft, topRight;
		NewDouble3 bottomLeft, bottomRight;

		// Projected screen-space coordinates.
		double startX, endX;
		double startY, endY;

		// Camera Z for depth sorting.
		double z;

		// Entity texture values.
		const FlatTexture *texture;
		const Palette *overridePalette; // For citizen variations.
	};

	struct DistantObject
	{
		// This distant object's index in the distant objects maps directly to the current sky instance for now.
		// @todo: redesign this once public texture handles are being allocated (a visible distant object will be a
		// direction and texture ID; just a piece of geometry like any other renderable thing).
		int startTextureIndex; // Points into skyTextures list.
		int textureIndexCount; // Greater than 1 for objects with animations.

		DistantObject(int startTextureIndex, int textureIndexCount);
		DistantObject();
	};

	// Collection of all distant objects.
	struct DistantObjects
	{
		// These map 1-to-1 to the active sky instance for now.
		Buffer<DistantObject> objs;
		int landStart, landEnd, airStart, airEnd, moonStart, moonEnd, sunStart, sunEnd, starStart, starEnd,
			lightningStart, lightningEnd;

		void init(const SkyInstance &skyInstance, std::vector<SkyTexture> &skyTextures,
			const Palette &palette, TextureManager &textureManager);

		void clear();
	};

	// A distant object that has been projected on-screen and is at least partially visible.
	struct VisDistantObject
	{
		const SkyTexture *texture;
		DrawRange drawRange;
		double xProjStart, xProjEnd; // Projected screen coordinates.
		int xStart, xEnd; // Pixel coordinates.
		bool emissive; // Only animated lands (i.e., volcanoes) are emissive.

		VisDistantObject(const SkyTexture &texture, DrawRange &&drawRange, double xProjStart,
			double xProjEnd, int xStart, int xEnd, bool emissive);
	};

	struct VisDistantObjects
	{
		std::vector<VisDistantObject> objs;

		// Need to store start and end indices for each range so we can call different 
		// shading methods on some of them. End indices are exclusive.
		int landStart, landEnd, airStart, airEnd, moonStart, moonEnd, sunStart, sunEnd, starStart, starEnd,
			lightningStart, lightningEnd;

		VisDistantObjects();

		void clear();
	};

	// Instance of a light in the world visible to the camera.
	struct VisibleLight
	{
		CoordDouble3 coord; // @todo: this is probably overkill since the visible light list is tied to a chunk.
		double radius;

		void init(const CoordDouble3 &coord, double radius);
	};

	// Data about a light and whether it's visible. Used with visible light determination.
	struct LightVisibilityData
	{
		CoordDouble3 coord;
		double radius;
		bool intersectsFrustum;

		void init(const CoordDouble3 &coord, double radius, bool intersectsFrustum);
	};

	struct VisibleLightList
	{
		using LightID = unsigned int;

		static constexpr int MAX_LIGHTS = 16;

		std::array<LightID, MAX_LIGHTS> lightIDs;
		int count;

		VisibleLightList();

		bool isFull() const;
		void add(LightID lightID);
		void clear();

		// Shading optimization, only useful when the light intensity cap is on for early-out.
		void sortByNearest(const CoordDouble3 &coord, BufferViewReadOnly<VisibleLight> &visLights);
	};

	// Each chunk has a visible light list per voxel column.
	using VisibleLightLists = std::unordered_map<ChunkInt2, Buffer2D<VisibleLightList>>;

	// Data owned by the main thread that is referenced by render threads.
	struct RenderThreadData
	{
		struct SkyGradient
		{
			int threadsDone;
			Buffer<Double3> *rowCache;
			double projectedYTop, projectedYBottom; // Projected Y range of sky gradient.
			std::atomic<bool> shouldDrawStars; // True if the sky is dark enough.

			void init(double projectedYTop, double projectedYBottom, Buffer<Double3> &rowCache);
		};

		struct DistantSky
		{
			int threadsDone;
			const VisDistantObjects *visDistantObjs;
			const std::vector<SkyTexture> *skyTextures;
			bool doneVisTesting; // True when render threads can start rendering distant sky.

			void init(const VisDistantObjects &visDistantObjs,
				const std::vector<SkyTexture> &skyTextures);
		};

		struct Voxels
		{
			int threadsDone;
			const ChunkManager *chunkManager;
			const std::vector<VisibleLight> *visLights;
			const VisibleLightLists *visLightLists;
			const VoxelTextures *voxelTextures;
			const ChasmTextureGroups *chasmTextureGroups;
			Buffer<OcclusionData> *occlusion;
			double ceilingScale;
			int chunkDistance;
			bool doneLightVisTesting; // True when render threads can start rendering voxels.

			void init(int chunkDistance, double ceilingScale, const ChunkManager &chunkManager,
				const std::vector<VisibleLight> &visLights, const VisibleLightLists &visLightLists,
				const VoxelTextures &voxelTextures, const ChasmTextureGroups &chasmTextureGroups,
				Buffer<OcclusionData> &occlusion);
		};

		struct Flats
		{
			int threadsDone;
			const Double3 *flatNormal;
			const std::vector<VisibleFlat> *visibleFlats;
			const std::vector<VisibleLight> *visLights;
			const VisibleLightLists *visLightLists;
			const EntityTextures *entityTextures;
			bool doneSorting; // True when render threads can start rendering flats.

			void init(const VoxelDouble3 &flatNormal, const std::vector<VisibleFlat> &visibleFlats,
				const std::vector<VisibleLight> &visLights, const VisibleLightLists &visLightLists,
				const EntityTextures &entityTextures);
		};

		struct Weather
		{
			int threadsDone;
			const WeatherInstance *weatherInst;
			Random *random;
			bool doneDrawingFlats; // True when render threads can start rendering weather.

			void init(const WeatherInstance &weatherInst, Random &random);
		};

		SkyGradient skyGradient;
		DistantSky distantSky;
		Voxels voxels;
		Flats flats;
		Weather weather;
		const Camera *camera;
		const ShadingInfo *shadingInfo;
		const FrameView *frame;

		std::condition_variable condVar;
		std::mutex mutex;
		int totalThreads;
		bool go; // Initial go signal to start work each frame.
		bool isDestructing; // Helps shut down threads in the renderer destructor.

		RenderThreadData();

		void init(int totalThreads, const Camera &camera, const ShadingInfo &shadingInfo, const FrameView &frame);
	};

	// Clipping planes for Z coordinates.
	static constexpr double NEAR_PLANE = 0.0001;
	static constexpr double FAR_PLANE = 1000.0;

	// Angle of the sky gradient above the horizon, in degrees.
	static constexpr double SKY_GRADIENT_ANGLE = 30.0;

	// Max angle of distant clouds above the horizon, in degrees.
	static constexpr double DISTANT_CLOUDS_MAX_ANGLE = 25.0;

	Buffer2D<double> depthBuffer;
	Buffer<OcclusionData> occlusion; // 1D buffer, min and max Y for each pixel column.
	std::vector<const Entity*> potentiallyVisibleFlats; // Updated every frame.
	std::vector<VisibleFlat> visibleFlats; // Flats to be drawn.
	DistantObjects distantObjects; // Distant sky objects (mountains, clouds, etc.).
	VisDistantObjects visDistantObjs; // Visible distant sky objects.
	VisibleLightLists visLightLists; // Potentially-visible voxel column references to visible lights.
	std::vector<VisibleLight> visibleLights; // Lights that contribute to the current frame.
	VoxelTextures voxelTextures; // Voxel textures and their mappings.
	EntityTextures entityTextures; // Entity textures and their mappings.
	ChasmTextureGroups chasmTextureGroups; // Mappings from chasm ID to textures.
	std::vector<SkyTexture> skyTextures; // Distant object textures. Size is managed internally.
	std::vector<Double3> skyColors; // Colors for each time of day.
	Buffer<Double3> skyGradientRowCache; // Contains row colors of most recent sky gradient.
	Buffer<std::thread> renderThreads; // Threads used for rendering the world.
	RenderThreadData threadData; // Managed by main thread, used by render threads.
	double fogDistance; // Distance at which fog is maximum.
	int width, height; // Dimensions of frame buffer.
	int renderThreadsMode; // Determines number of threads to use for rendering.

	// Initializes render threads that run in the background for the duration of the renderer's
	// lifetime. This can also be used to reset threads after a screen resize.
	void initRenderThreads(int width, int height, int threadCount);

	// Turns off each thread in the render threads list peacefully. The render threads are expected
	// to be at their initial wait condition before being given the go + destruct signals.
	void resetRenderThreads();

	// Refreshes the list of distant objects to be drawn.
	void updateVisibleDistantObjects(const SkyInstance &skyInstance, const ShadingInfo &shadingInfo,
		const Camera &camera, const FrameView &frame);

	// Refreshes the list of potentially visible flats (to be passed to actually-visible flat
	// calculation).
	static void updatePotentiallyVisibleFlats(const Camera &camera,int chunkDistance,
		const EntityManager &entityManager, std::vector<const Entity*> *outPotentiallyVisFlats, int *outEntityCount);

	// Refreshes the list of flats to be drawn.
	void updateVisibleFlats(const Camera &camera, const ShadingInfo &shadingInfo, int chunkDistance,
		double ceilingScale, const ChunkManager &chunkManager, const EntityManager &entityManager,
		const EntityDefinitionLibrary &entityDefLibrary);

	// Refreshes the visible light lists in each voxel column in the view frustum.
	void updateVisibleLightLists(const Camera &camera, int chunkDistance, double ceilingScale);
	
	// Gets the facing value for the far side of a chasm.
	static VoxelFacing2D getInitialChasmFarFacing(const CoordInt2 &coord, const NewDouble2 &eye, const Ray &ray);
	static VoxelFacing2D getChasmFarFacing(const CoordInt2 &coord, VoxelFacing2D nearFacing,
		const Camera &camera, const Ray &ray);

	// Tries to convert the chasm animation percent to the associated texture within the chasm
	// texture group for the given chasm type.
	static void getChasmTextureGroupTexture(const ChasmTextureGroups &textureGroups,
		ArenaTypes::ChasmType chasmType, double chasmAnimPercent, const ChasmTexture **outTexture);

	// Looks up a light in the visible lights list given some light ID.
	static const VisibleLight &getVisibleLightByID(BufferViewReadOnly<VisibleLight> &visLights,
		VisibleLightList::LightID lightID);

	// Gets the visible light list associated with some voxel column if the given chunk is available.
	static const VisibleLightList *getVisibleLightList(
		const VisibleLightLists &visLightLists, const CoordInt2 &coord);

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

	// Gets the blended thunderstorm flash color for a percentage through the flash animation.
	static Double3 getThunderstormFlashColor(double flashPercent, const Double3 *colors, int colorCount);

	// Gathers potential intersection data from a voxel containing a "diagonal 1" ID; the 
	// diagonal starting at (nearX, nearZ) and ending at (farX, farZ). Returns whether an 
	// intersection occurred within the voxel.
	static bool findDiag1Intersection(const CoordInt2 &coord, const NewDouble2 &nearPoint,
		const NewDouble2 &farPoint, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a "diagonal 2" ID; the
	// diagonal starting at (farX, nearZ) and ending at (nearX, farZ). Returns whether an
	// intersection occurred within the voxel.
	static bool findDiag2Intersection(const CoordInt2 &coord, const NewDouble2 &nearPoint,
		const NewDouble2 &farPoint, RayHit &hit);

	// Gathers potential intersection data from an initial voxel containing an edge ID. The
	// facing determines which edge of the voxel an intersection can occur on.
	static bool findInitialEdgeIntersection(const CoordInt2 &coord, VoxelFacing2D edgeFacing,
		bool flipped, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, const Camera &camera,
		const Ray &ray, RayHit &hit);

	// Gathers potential intersection data from a voxel containing an edge ID. The facing
	// determines which edge of the voxel an intersection can occur on. This function is separate
	// from the initial case since it's a trivial solution when the edge and near facings match.
	static bool findEdgeIntersection(const CoordInt2 &coord, VoxelFacing2D edgeFacing,
		bool flipped, VoxelFacing2D nearFacing, const NewDouble2 &nearPoint,
		const NewDouble2 &farPoint, double nearU, const Camera &camera, const Ray &ray, RayHit &hit);

	// Helper method for findInitialDoorIntersection() for swinging doors.
	static bool findInitialSwingingDoorIntersection(const CoordInt2 &coord, double percentOpen,
		const NewDouble2 &nearPoint, const NewDouble2 &farPoint, bool xAxis, const Camera &camera,
		const Ray &ray, RayHit &hit);
	
	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection.
	static bool findInitialDoorIntersection(const CoordInt2 &coord, ArenaTypes::DoorType doorType,
		double percentOpen, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, const Camera &camera,
		const Ray &ray, const ChunkManager &chunkManager, RayHit &hit);

	// Helper method for findDoorIntersection() for swinging doors.
	static bool findSwingingDoorIntersection(const CoordInt2 &coord, double percentOpen,
		VoxelFacing2D nearFacing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
		double nearU, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection. Raising doors
	// are always hit, so they do not need a specialized method.
	static bool findDoorIntersection(const CoordInt2 &coord, ArenaTypes::DoorType doorType,
		double percentOpen, VoxelFacing2D nearFacing, const NewDouble2 &nearPoint,
		const NewDouble2 &farPoint, double nearU, RayHit &hit);

	// Calculates light visibility data for a given entity.
	static void getLightVisibilityData(const CoordDouble3 &flatCoord, double flatHeight,
		double lightRadius, const CoordDouble2 &eyeCoord, const VoxelDouble2 &cameraDir, Degrees fovX,
		double viewDistance, LightVisibilityData *outVisData);

	// Gets the amount of light at a point. Capped at 100% intensity if not unlimited.
	// @todo: give actual visible light count as template parameter so the for loop can be
	// completely inlined for each light count (permutation count depends on visible light
	// list max lights).
	template <bool CappedSum>
	static double getLightContributionAtPoint(const CoordDouble2 &coord,
		BufferViewReadOnly<VisibleLight> &visLights, const VisibleLightList &visLightList);

	// Low-level texture sampling function.
	template <int FilterMode, bool Transparency>
	static void sampleVoxelTexture(const VoxelTexture &texture, double u, double v,
		double *r, double *g, double *b, double *emission, bool *transparent);

	// Low-level screen-space chasm texture sampling function.
	static void sampleChasmTexture(const ChasmTexture &texture, double screenXPercent,
		double screenYPercent, double *r, double *g, double *b);

	// Low-level fog matrix sampling function.
	template <int TextureWidth, int TextureHeight>
	static uint8_t sampleFogMatrixTexture(const ArenaRenderUtils::FogMatrix &fogMatrix, double u, double v);

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
		const NewDouble2 &startPoint, const NewDouble2 &endPoint, double depthStart, double depthEnd,
		const Double3 &normal, const VoxelTexture &texture, double fadePercent,
		BufferViewReadOnly<VisibleLight> &visLights, const VisibleLightList &visLightList,
		const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels with perspective but no transparency. The pixel drawing order is 
	// top to bottom, so the start and end values should be passed with that in mind.
	static void drawPerspectivePixels(int x, const DrawRange &drawRange, const NewDouble2 &startPoint,
		const NewDouble2 &endPoint, double depthStart, double depthEnd, const Double3 &normal,
		const VoxelTexture &texture, double fadePercent, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightList &visLightList, const ShadingInfo &shadingInfo, OcclusionData &occlusion,
		const FrameView &frame);

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
		const NewDouble2 &startPoint, const NewDouble2 &endPoint, double depthStart, double depthEnd,
		const Double3 &normal, const ChasmTexture &texture, const ShadingInfo &shadingInfo,
		OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of chasm pixels with perspective and no transparency. The pixel drawing order
	// is top to bottom, so the start and end values should be passed with that in mind.
	static void drawPerspectiveChasmPixels(int x, const DrawRange &drawRange,
		const NewDouble2 &startPoint, const NewDouble2 &endPoint, double depthStart, double depthEnd,
		const Double3 &normal, bool emissive, const ChasmTexture &texture,
		const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame);

	// Draws a column of pixels for a distant sky object (mountain, cloud, etc.). The 'emissive'
	// parameter is for animated objects like volcanoes.
	static void drawDistantPixels(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, bool emissive, const ShadingInfo &shadingInfo,
		const FrameView &frame);
	/*static void drawDistantPixelsSSE(int x, const DrawRange &drawRange, double u, double vStart,
		double vEnd, const SkyTexture &texture, bool emissive, const ShadingInfo &shadingInfo,
		const FrameView &frame);*/
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
		double vEnd, const SkyTexture &texture, const Buffer<Double3> &skyGradientRowCache,
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Helper functions for drawing the initial voxel column.
	static void drawInitialVoxelSameFloor(int x, const Chunk &chunk, const VoxelInt3 &voxel,
		const Camera &camera, const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint,
		const NewDouble2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
		const ShadingInfo &shadingInfo, int chunkDistance, double ceilingScale,
		const ChunkManager &chunkManager, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const VoxelTextures &textures,
		const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame);
	static void drawInitialVoxelAbove(int x, const Chunk &chunk, const VoxelInt3 &voxel,
		const Camera &camera, const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint,
		const NewDouble2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
		const ShadingInfo &shadingInfo, int chunkDistance, double ceilingScale,
		const ChunkManager &chunkManager, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const VoxelTextures &textures,
		const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame);
	static void drawInitialVoxelBelow(int x, const Chunk &chunk, const VoxelInt3 &voxel,
		const Camera &camera, const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint,
		const NewDouble2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
		const ShadingInfo &shadingInfo, int chunkDistance, double ceilingScale,
		const ChunkManager &chunkManager, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const VoxelTextures &textures,
		const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame);

	// Manages drawing voxels in the column that the player is in.
	static void drawInitialVoxelColumn(int x, const CoordInt2 &coord, const Camera &camera,
		const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
		double nearZ, double farZ, const ShadingInfo &shadingInfo, int chunkDistance, double ceilingScale,
		const ChunkManager &chunkManager, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const VoxelTextures &textures,
		const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame);

	// Helper functions for drawing a voxel column.
	static void drawVoxelSameFloor(int x, const Chunk &chunk, const VoxelInt3 &voxel, const Camera &camera,
		const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
		double nearZ, double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo,
		int chunkDistance, double ceilingScale, const ChunkManager &chunkManager,
		BufferViewReadOnly<VisibleLight> &visLights, const VisibleLightLists &visLightLists,
		const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion,
		const FrameView &frame);
	static void drawVoxelAbove(int x, const Chunk &chunk, const VoxelInt3 &voxel, const Camera &camera,
		const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
		double nearZ, double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo,
		int chunkDistance, double ceilingScale, const ChunkManager &chunkManager,
		BufferViewReadOnly<VisibleLight> &visLights, const VisibleLightLists &visLightLists,
		const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion,
		const FrameView &frame);
	static void drawVoxelBelow(int x, const Chunk &chunk, const VoxelInt3 &voxel, const Camera &camera,
		const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
		double nearZ, double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo,
		int chunkDistance, double ceilingScale, const ChunkManager &chunkManager,
		BufferViewReadOnly<VisibleLight> &visLights, const VisibleLightLists &visLightLists,
		const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion,
		const FrameView &frame);

	// Manages drawing voxels in the column of the given XZ coordinate in the voxel grid.
	static void drawVoxelColumn(int x, const CoordInt2 &coord, const Camera &camera,
		const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
		double nearZ, double farZ, const ShadingInfo &shadingInfo, int chunkDistance, double ceilingScale,
		const ChunkManager &chunkManager, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const VoxelTextures &textures,
		const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame);

	// Draws the portion of a flat contained within the given X range of the screen. The end
	// X value is exclusive.
	static void drawFlat(int startX, int endX, const VisibleFlat &flat, const Double3 &normal,
		const NewDouble2 &eye, const NewInt2 &eyeVoxelXZ, double horizonProjY, const ShadingInfo &shadingInfo,
		const Palette *overridePalette, int chunkDistance, const FlatTexture &texture,
		BufferViewReadOnly<VisibleLight> &visLights, const VisibleLightLists &visLightLists,
		const FrameView &frame);

	// Casts a 2D ray that steps through the current floor, rendering all voxels in the XZ column of each voxel.
	template <bool NonNegativeDirX, bool NonNegativeDirZ>
	static void rayCast2DInternal(int x, const Camera &camera, const Ray &ray,
		const ShadingInfo &shadingInfo, int chunkDistance, double ceilingScale,
		const ChunkManager &chunkManager, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const VoxelTextures &textures,
		const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame);

	// Helper method for internal ray casting function that takes template parameters for better
	// code generation.
	static void rayCast2D(int x, const Camera &camera, const Ray &ray, const ShadingInfo &shadingInfo,
		int chunkDistance, double ceilingScale, const ChunkManager &chunkManager,
		BufferViewReadOnly<VisibleLight> &visLights, const VisibleLightLists &visLightLists,
		const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion,
		const FrameView &frame);

	// Draws a portion of the sky gradient. The start and end Y are determined from current
	// threading settings.
	static void drawSkyGradient(int startY, int endY, double gradientProjYTop, double gradientProjYBottom,
		Buffer<Double3> &skyGradientRowCache, std::atomic<bool> &shouldDrawStars,
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Draws some columns of distant sky objects (mountains, clouds, etc.). The start and end X
	// are determined from current threading settings.
	static void drawDistantSky(int startX, int endX, const VisDistantObjects &visDistantObjs,
		const std::vector<SkyTexture> &skyTextures, const Buffer<Double3> &skyGradientRowCache,
		bool shouldDrawStars, const ShadingInfo &shadingInfo, const FrameView &frame);

	// Handles drawing all voxels for the current frame.
	static void drawVoxels(int startX, int stride, const Camera &camera, int chunkDistance,
		double ceilingScale, const ChunkManager &chunkManager, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const VoxelTextures &voxelTextures,
		const ChasmTextureGroups &chasmTextureGroups, Buffer<OcclusionData> &occlusion,
		const ShadingInfo &shadingInfo, const FrameView &frame);

	// Handles drawing all flats for the current frame.
	static void drawFlats(int startX, int endX, const Camera &camera, const Double3 &flatNormal,
		const std::vector<VisibleFlat> &visibleFlats, const EntityTextures &entityTextures,
		const ShadingInfo &shadingInfo, int chunkDistance, BufferViewReadOnly<VisibleLight> &visLights,
		const VisibleLightLists &visLightLists, const FrameView &frame);

	// Handles drawing the current weather (if any).
	static void drawWeather(int threadStartX, int threadEndX, const WeatherInstance &weatherInst, const Camera &camera,
		const ShadingInfo &shadingInfo, Random &random, const FrameView &frame);

	// Thread loop for each render thread. All threads are initialized in the constructor and
	// wait for a go signal at the beginning of each render(). If the renderer is destructing,
	// then each render thread still gets a go signal, but they immediately leave their loop
	// and terminate. Non-thread-data parameters are for start/end column/row for each thread.
	static void renderThreadLoop(RenderThreadData &threadData, int threadIndex, int startX,
		int endX, int startY, int endY);
public:
	SoftwareRenderer();
	~SoftwareRenderer() override;

	bool isInited() const override;

	// Gets profiling information about renderer internals.
	ProfilerData getProfilerData() const override;

	// Tries to write out selection data for the given entity. Returns whether selection data was
	// successfully written.
	bool tryGetEntitySelectionData(const Double2 &uv, const TextureAssetReference &textureAssetRef,
		bool flipped, bool reflective, bool pixelPerfect, const Palette &palette, bool *outIsSelected) const override;

	// Converts a screen point to a ray into the game world.
	Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
		Degrees fovY, double aspect) const override;

	// Sets the render threads mode to use (low, medium, high, etc.).
	void setRenderThreadsMode(int mode) override;

	// Sets the distance at which the fog is maximum.
	void setFogDistance(double fogDistance) override;

	// Sets textures for the distant sky (mountains, clouds, etc.).
	void setSky(const SkyInstance &skyInstance, const Palette &palette, TextureManager &textureManager) override;

	// Sets the sky palette to use with sky colors based on the time of day.
	// For dungeons, this would probably just be one black pixel.
	void setSkyColors(const uint32_t *colors, int count) override;

	// Adds a screen-space chasm texture to the given chasm type's texture list.
	void addChasmTexture(ArenaTypes::ChasmType chasmType, const uint8_t *colors,
		int width, int height, const Palette &palette) override;

	// Sets whether night lights and night textures are active. This only needs to be set for
	// exterior locations (i.e., cities and wilderness) because those are the only places
	// with time-dependent light sources and textures.
	void setNightLightsActive(bool active, const Palette &palette) override;

	// Zeroes out all renderer textures and entity render ID mappings to textures.
	void clearTextures() override;

	// Removes all sky objects.
	void clearSky() override;

	void init(const RenderInitSettings &settings) override;
	void shutdown() override;
	void resize(int width, int height) override;

	bool tryCreateVoxelTexture(const TextureAssetReference &textureAssetRef,
		TextureManager &textureManager) override;
	bool tryCreateEntityTexture(const TextureAssetReference &textureAssetRef, bool flipped,
		bool reflective, TextureManager &textureManager) override;
	bool tryCreateSkyTexture(const TextureAssetReference &textureAssetRef,
		TextureManager &textureManager) override;

	void freeVoxelTexture(const TextureAssetReference &textureAssetRef) override;
	void freeEntityTexture(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective) override;
	void freeSkyTexture(const TextureAssetReference &textureAssetRef) override;

	// Draws the scene to the output color buffer in ARGB8888 format.
	// @todo: move everything to RenderCamera and RenderFrameSettings temporarily until design is finished.
	void render(const CoordDouble3 &eye, const Double3 &direction, double fovY, double ambient,
		double daytimePercent, double chasmAnimPercent, double latitude, bool nightLightsAreActive,
		bool isExterior, bool playerHasLight, int chunkDistance, double ceilingScale, const LevelInstance &levelInst,
		const SkyInstance &skyInst, const WeatherInstance &weatherInst, Random &random,
		const EntityDefinitionLibrary &entityDefLibrary, const Palette &palette, uint32_t *colorBuffer) override;

	// @todo: might want to simplify the various set() function lifetimes of the renderer from
	// at-init/occasional/every-frame to just at-init/every-frame. Things like the sky palette or render
	// threads mode could be set every frame for simplicity. Just do it at the start of this method.
	void submitFrame(const RenderDefinitionGroup &defGroup, const RenderInstanceGroup &instGroup,
		const RenderCamera &camera, const RenderFrameSettings &settings) override;
	void present() override;
};

#endif
