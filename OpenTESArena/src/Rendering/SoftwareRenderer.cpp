#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

#include "SoftwareRenderer.h"
#include "Surface.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Media/Color.h"
#include "../Utilities/Debug.h"
#include "../Utilities/Platform.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelGrid.h"

SoftwareRenderer::VoxelTexel::VoxelTexel()
{
	this->r = 0.0;
	this->g = 0.0;
	this->b = 0.0;
	this->emission = 0.0;
	this->transparent = false;
}

SoftwareRenderer::FlatTexel::FlatTexel()
{
	this->r = 0.0;
	this->g = 0.0;
	this->b = 0.0;
	this->a = 0.0;
}

SoftwareRenderer::SkyTexel::SkyTexel()
{
	this->r = 0.0;
	this->g = 0.0;
	this->b = 0.0;
	this->transparent = false;
}

SoftwareRenderer::FlatTexture::FlatTexture()
{
	this->width = 0;
	this->height = 0;
}

SoftwareRenderer::SkyTexture::SkyTexture()
{
	this->width = 0;
	this->height = 0;
}

SoftwareRenderer::Camera::Camera(const Double3 &eye, const Double3 &direction,
	double fovY, double aspect, double projectionModifier)
	: eye(eye), direction(direction)
{
	// Variations of eye position for certain voxel calculations.
	this->eyeVoxelReal = Double3(
		std::floor(eye.x),
		std::floor(eye.y),
		std::floor(eye.z));
	this->eyeVoxel = Int3(
		static_cast<int>(this->eyeVoxelReal.x),
		static_cast<int>(this->eyeVoxelReal.y),
		static_cast<int>(this->eyeVoxelReal.z));

	// Camera axes. We trick the 2.5D ray caster into thinking the player is always looking
	// straight forward, but we use the Y component of the player's direction to offset 
	// projected coordinates via Y-shearing.
	const Double3 forwardXZ = Double3(direction.x, 0.0, direction.z).normalized();
	const Double3 rightXZ = forwardXZ.cross(Double3::UnitY).normalized();

	// Transformation matrix (model matrix isn't required because it's just the identity).
	this->transform = [&eye, &forwardXZ, &rightXZ, fovY, aspect, projectionModifier]()
	{
		// Global up vector, scaled by the projection modifier (i.e., to account for tall pixels).
		const Double3 up = Double3::UnitY * projectionModifier;

		const Matrix4d view = Matrix4d::view(eye, forwardXZ, rightXZ, up);
		const Matrix4d projection = Matrix4d::perspective(fovY, aspect,
			SoftwareRenderer::NEAR_PLANE, SoftwareRenderer::FAR_PLANE);
		return projection * view;
	}();

	this->forwardX = forwardXZ.x;
	this->forwardZ = forwardXZ.z;
	this->rightX = rightXZ.x;
	this->rightZ = rightXZ.z;

	this->fovY = fovY;

	// Zoom of the camera, based on vertical field of view.
	this->zoom = MathUtils::verticalFovToZoom(fovY);
	this->aspect = aspect;

	// Forward and right modifiers, for interpolating 3D vectors across the screen and
	// so vertical FOV and aspect ratio are taken into consideration.
	this->forwardZoomedX = this->forwardX * this->zoom;
	this->forwardZoomedZ = this->forwardZ * this->zoom;
	this->rightAspectedX = this->rightX * this->aspect;
	this->rightAspectedZ = this->rightZ * this->aspect;

	// Left and right 2D vectors of the view frustum (at left and right edges of the screen).
	const Double2 frustumLeft = Double2(
		this->forwardZoomedX - this->rightAspectedX,
		this->forwardZoomedZ - this->rightAspectedZ).normalized();
	const Double2 frustumRight = Double2(
		this->forwardZoomedX + this->rightAspectedX,
		this->forwardZoomedZ + this->rightAspectedZ).normalized();
	this->frustumLeftX = frustumLeft.x;
	this->frustumLeftZ = frustumLeft.y;
	this->frustumRightX = frustumRight.x;
	this->frustumRightZ = frustumRight.y;

	// Vertical angle of the camera relative to the horizon.
	this->yAngleRadians = direction.getYAngleRadians();

	// Y-shearing is the distance that projected Y coordinates are translated by based on the 
	// player's 3D direction and field of view. First get the player's angle relative to the 
	// horizon, then get the tangent of that angle. The Y component of the player's direction
	// must be clamped less than 1 because 1 would imply they are looking straight up or down, 
	// which is impossible in 2.5D rendering (the vertical line segment of the view frustum 
	// would be infinitely high or low). The camera code should take care of the clamping for us.

	// Get the number of screen heights to translate all projected Y coordinates by, relative to
	// the current zoom. As a reference, this should be some value roughly between -1.0 and 1.0
	// for "acceptable skewing" at a vertical FOV of 90.0. If the camera is not clamped, this
	// could theoretically be between -infinity and infinity, but it would result in far too much
	// skewing.
	this->yShear = std::tan(this->yAngleRadians) * this->zoom;
}

double SoftwareRenderer::Camera::getXZAngleRadians() const
{
	return MathUtils::fullAtan2(this->forwardX, this->forwardZ);
}

int SoftwareRenderer::Camera::getAdjustedEyeVoxelY(double ceilingHeight) const
{
	return static_cast<int>(this->eye.y / ceilingHeight);
}

SoftwareRenderer::Ray::Ray(double dirX, double dirZ)
{
	this->dirX = dirX;
	this->dirZ = dirZ;
}

SoftwareRenderer::DrawRange::DrawRange(double yProjStart, double yProjEnd, int yStart, int yEnd)
{
	this->yProjStart = yProjStart;
	this->yProjEnd = yProjEnd;
	this->yStart = yStart;
	this->yEnd = yEnd;
}

SoftwareRenderer::OcclusionData::OcclusionData(int yMin, int yMax)
{
	this->yMin = yMin;
	this->yMax = yMax;
}

SoftwareRenderer::OcclusionData::OcclusionData()
	: OcclusionData(0, 0) { }

void SoftwareRenderer::OcclusionData::clipRange(int *yStart, int *yEnd) const
{
	const bool occluded = (*yEnd <= this->yMin) || (*yStart >= this->yMax);

	if (occluded)
	{
		// The drawing range is completely hidden.
		*yStart = *yEnd;
	}
	else
	{
		// Clip the drawing range.
		*yStart = std::max(*yStart, this->yMin);
		*yEnd = std::min(*yEnd, this->yMax);
	}
}

void SoftwareRenderer::OcclusionData::update(int yStart, int yEnd)
{
	// Slightly different than clipRange() because values just needs to be adjacent
	// rather than overlap.
	const bool canIncreaseMin = yStart <= this->yMin;
	const bool canDecreaseMax = yEnd >= this->yMax;

	// Determine how to update the occlusion ranges.
	if (canIncreaseMin && canDecreaseMax)
	{
		// The drawing range touches the top and bottom occlusion values, so the 
		// entire column is occluded.
		this->yMin = this->yMax;
	}
	else if (canIncreaseMin)
	{
		// Move the top of the window downward.
		this->yMin = std::max(yEnd, this->yMin);
	}
	else if (canDecreaseMax)
	{
		// Move the bottom of the window upward.
		this->yMax = std::min(yStart, this->yMax);
	}
}

SoftwareRenderer::ShadingInfo::ShadingInfo(const std::vector<Double3> &skyPalette,
	double daytimePercent, double ambient, double fogDistance)
{
	// The "sliding window" of sky colors is backwards in the AM (horizon is latest in the palette)
	// and forwards in the PM (horizon is earliest in the palette).
	this->isAM = daytimePercent < 0.50;
	const int slideDirection = this->isAM ? -1 : 1;

	// Get the real index (not the integer index) of the color for the current time as a
	// reference point so each sky color can be interpolated between two samples via 'percent'.
	const double realIndex = static_cast<double>(skyPalette.size()) * daytimePercent;
	const double percent = realIndex - std::floor(realIndex);

	// Lambda for keeping the given index within the palette range.
	auto wrapIndex = [&skyPalette](int index)
	{
		const int paletteCount = static_cast<int>(skyPalette.size());

		while (index >= paletteCount)
		{
			index -= paletteCount;
		}

		while (index < 0)
		{
			index += paletteCount;
		}

		return index;
	};

	// Calculate sky colors based on the time of day.
	for (int i = 0; i < static_cast<int>(this->skyColors.size()); i++)
	{
		const int indexDiff = slideDirection * i;
		const int index = wrapIndex(static_cast<int>(realIndex) + indexDiff);
		const int nextIndex = wrapIndex(index + slideDirection);
		const Double3 &color = skyPalette.at(index);
		const Double3 &nextColor = skyPalette.at(nextIndex);

		this->skyColors.at(i) = color.lerp(nextColor, this->isAM ? (1.0 - percent) : percent);
	}

	// The sun rises in the west (-Z) and sets in the east (+Z).
	this->sunDirection = [daytimePercent]()
	{
		const double radians = daytimePercent * Constants::TwoPi;
		return Double3(0.0, -std::cos(radians), -std::sin(radians)).normalized();
	}();
	
	this->sunColor = [this]()
	{
		const Double3 baseSunColor(0.90, 0.875, 0.85);

		// Darken the sun color if it's below the horizon so wall faces aren't lit 
		// as much during the night. This is just an artistic value to compensate
		// for the lack of shadows.
		return (this->sunDirection.y >= 0.0) ? baseSunColor :
			(baseSunColor * (1.0 - (5.0 * std::abs(this->sunDirection.y)))).clamped();
	}();

	this->ambient = ambient;

	// At their darkest, distant objects are ~1/4 of their intensity.
	this->distantAmbient = MathUtils::clamp(ambient, 0.25, 1.0);

	this->fogDistance = fogDistance;
}

const Double3 &SoftwareRenderer::ShadingInfo::getFogColor() const
{
	// The fog color is the same as the horizon color.
	return this->skyColors.front();
}

SoftwareRenderer::FrameView::FrameView(uint32_t *colorBuffer, double *depthBuffer, 
	int width, int height)
{
	this->colorBuffer = colorBuffer;
	this->depthBuffer = depthBuffer;
	this->width = width;
	this->height = height;
	this->widthReal = static_cast<double>(width);
	this->heightReal = static_cast<double>(height);
}

SoftwareRenderer::VisibleFlat::VisibleFlat(const Flat &flat, Flat::Frame &&frame)
{
	this->flat = &flat;
	this->frame = std::move(frame);
}

const SoftwareRenderer::Flat &SoftwareRenderer::VisibleFlat::getFlat() const
{
	return *this->flat;
}

const SoftwareRenderer::Flat::Frame &SoftwareRenderer::VisibleFlat::getFrame() const
{
	return this->frame;
}

SoftwareRenderer::DistantObject::DistantObject(int textureIndex, DistantObject::Type type, const void *obj)
{
	this->textureIndex = textureIndex;
	this->type = type;

	// Assign based on the object type.
	if (type == DistantObject::Type::Land)
	{
		this->land = static_cast<const DistantSky::LandObject*>(obj);
	}
	else if (type == DistantObject::Type::AnimatedLand)
	{
		this->animLand = static_cast<const DistantSky::AnimatedLandObject*>(obj);
	}
	else if (type == DistantObject::Type::Air)
	{
		this->air = static_cast<const DistantSky::AirObject*>(obj);
	}
	else if (type == DistantObject::Type::Space)
	{
		this->space = static_cast<const DistantSky::SpaceObject*>(obj);
	}
	else
	{
		throw DebugException("Invalid distant object type \"" +
			std::to_string(static_cast<int>(type)) + "\".");
	}
}

SoftwareRenderer::VisDistantObject::ParallaxData::ParallaxData()
{
	this->xVisAngleStart = 0.0;
	this->xVisAngleEnd = 0.0;
	this->uStart = 0.0;
	this->uEnd = 0.0;
}

SoftwareRenderer::VisDistantObject::ParallaxData::ParallaxData(double xVisAngleStart,
	double xVisAngleEnd, double uStart, double uEnd)
{
	this->xVisAngleStart = xVisAngleStart;
	this->xVisAngleEnd = xVisAngleEnd;
	this->uStart = uStart;
	this->uEnd = uEnd;
}

SoftwareRenderer::VisDistantObject::VisDistantObject(const SkyTexture &texture,
	DrawRange &&drawRange, ParallaxData &&parallax, double xProjStart, double xProjEnd,
	int xStart, int xEnd, bool emissive)
	: drawRange(std::move(drawRange)), parallax(std::move(parallax))
{
	this->texture = &texture;
	this->xProjStart = xProjStart;
	this->xProjEnd = xProjEnd;
	this->xStart = xStart;
	this->xEnd = xEnd;
	this->emissive = emissive;
}

SoftwareRenderer::VisDistantObject::VisDistantObject(const SkyTexture &texture,
	DrawRange &&drawRange, double xProjStart, double xProjEnd, int xStart, int xEnd, bool emissive)
	: VisDistantObject(texture, std::move(drawRange), ParallaxData(), xProjStart, xProjEnd,
		xStart, xEnd, emissive) { }

void SoftwareRenderer::RenderThreadData::SkyGradient::init()
{
	this->threadsDone = 0;
}

void SoftwareRenderer::RenderThreadData::DistantSky::init(bool parallaxSky,
	const std::vector<VisDistantObject> &visDistantObjs,
	const std::vector<SkyTexture> &skyTextures)
{
	this->threadsDone = 0;
	this->visDistantObjs = &visDistantObjs;
	this->skyTextures = &skyTextures;
	this->parallaxSky = parallaxSky;
	this->doneVisTesting = false;
}

void SoftwareRenderer::RenderThreadData::Voxels::init(double ceilingHeight,
	const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
	const std::vector<VoxelTexture> &voxelTextures, std::vector<OcclusionData> &occlusion)
{
	this->threadsDone = 0;
	this->ceilingHeight = ceilingHeight;
	this->openDoors = &openDoors;
	this->voxelGrid = &voxelGrid;
	this->voxelTextures = &voxelTextures;
	this->occlusion = &occlusion;
}

void SoftwareRenderer::RenderThreadData::Flats::init(const Double3 &flatNormal,
	const std::vector<VisibleFlat> &visibleFlats, const std::vector<FlatTexture> &flatTextures)
{
	this->threadsDone = 0;
	this->flatNormal = &flatNormal;
	this->visibleFlats = &visibleFlats;
	this->flatTextures = &flatTextures;
	this->doneSorting = false;
}

SoftwareRenderer::RenderThreadData::RenderThreadData()
{
	// Make sure 'go' is initialized to false.
	this->totalThreads = 0;
	this->go = false;
	this->isDestructing = false;
	this->camera = nullptr;
	this->shadingInfo = nullptr;
	this->frame = nullptr;
}

void SoftwareRenderer::RenderThreadData::init(int totalThreads, const Camera &camera,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	this->totalThreads = totalThreads;
	this->camera = &camera;
	this->shadingInfo = &shadingInfo;
	this->frame = &frame;
	this->go = false;
	this->isDestructing = false;
}

const double SoftwareRenderer::NEAR_PLANE = 0.0001;
const double SoftwareRenderer::FAR_PLANE = 1000.0;
const int SoftwareRenderer::DEFAULT_VOXEL_TEXTURE_COUNT = 64;
const int SoftwareRenderer::DEFAULT_FLAT_TEXTURE_COUNT = 256;
const double SoftwareRenderer::DOOR_MIN_VISIBLE = 0.10;
const int SoftwareRenderer::NO_SUN = -1;
const double SoftwareRenderer::SKY_GRADIENT_ANGLE = 30.0;
const double SoftwareRenderer::DISTANT_CLOUDS_MAX_ANGLE = 25.0;
const double SoftwareRenderer::TALL_PIXEL_RATIO = 1.20;

SoftwareRenderer::SoftwareRenderer()
{
	// Initialize values to empty.
	this->width = 0;
	this->height = 0;
	this->renderThreadsMode = 0;
	this->sunTextureIndex = SoftwareRenderer::NO_SUN;
	this->fogDistance = 0.0;
}

SoftwareRenderer::~SoftwareRenderer()
{
	this->resetRenderThreads();
}

bool SoftwareRenderer::isInited() const
{
	// Frame buffer area must be positive.
	return (this->width > 0) && (this->height > 0);
}

void SoftwareRenderer::init(int width, int height, int renderThreadsMode)
{
	// Initialize 2D frame buffer.
	const int pixelCount = width * height;
	this->depthBuffer = std::vector<double>(pixelCount,
		std::numeric_limits<double>::infinity());

	// Initialize occlusion columns.
	this->occlusion = std::vector<OcclusionData>(width, OcclusionData(0, height));

	// Initialize texture vectors to default sizes.
	this->voxelTextures = std::vector<VoxelTexture>(SoftwareRenderer::DEFAULT_VOXEL_TEXTURE_COUNT);
	this->flatTextures = std::vector<FlatTexture>(SoftwareRenderer::DEFAULT_FLAT_TEXTURE_COUNT);

	this->width = width;
	this->height = height;
	this->renderThreadsMode = renderThreadsMode;

	// Fog distance is zero by default.
	this->fogDistance = 0.0;

	// Initialize render threads.
	const int threadCount = SoftwareRenderer::getRenderThreadsFromMode(renderThreadsMode);
	this->initRenderThreads(width, height, threadCount);
}

void SoftwareRenderer::setRenderThreadsMode(int mode)
{
	this->renderThreadsMode = mode;

	// Re-initialize render threads.
	const int threadCount = SoftwareRenderer::getRenderThreadsFromMode(renderThreadsMode);
	this->initRenderThreads(this->width, this->height, threadCount);
}

void SoftwareRenderer::addFlat(int id, const Double3 &position, double width, 
	double height, int textureID)
{
	// Verify that the ID is not already in use.
	DebugAssertMsg(this->flats.find(id) == this->flats.end(),
		"Flat ID \"" + std::to_string(id) + "\" already taken.");

	SoftwareRenderer::Flat flat;
	flat.position = position;
	flat.width = width;
	flat.height = height;
	flat.textureID = textureID;
	flat.flipped = false; // The initial value doesn't matter; it's updated frequently.

	// Add the flat (sprite, door, store sign, etc.).
	this->flats.insert(std::make_pair(id, flat));
}

void SoftwareRenderer::addLight(int id, const Double3 &point, const Double3 &color, 
	double intensity)
{
	DebugNotImplemented();
}

void SoftwareRenderer::setVoxelTexture(int id, const uint32_t *srcTexels)
{
	// Clear the selected texture.
	VoxelTexture &texture = this->voxelTextures.at(id);
	std::fill(texture.texels.begin(), texture.texels.end(), VoxelTexel());
	texture.lightTexels.clear();

	for (int y = 0; y < VoxelTexture::HEIGHT; y++)
	{
		for (int x = 0; x < VoxelTexture::WIDTH; x++)
		{
			// @todo: change this calculation for rotated textures. Make sure to have a 
			// source index and destination index.
			// - "dstX" and "dstY" should be calculated, and also used with lightTexels.
			const int index = x + (y * VoxelTexture::WIDTH);

			// Convert ARGB color from integer to double-precision format. This does waste
			// an extreme amount of memory (32 bytes per pixel!), but it's not a big deal
			// for Arena's textures (eight textures is a megabyte).
			const Double4 srcTexel = Double4::fromARGB(srcTexels[index]);
			VoxelTexel &dstTexel = texture.texels[index];
			dstTexel.r = srcTexel.x;
			dstTexel.g = srcTexel.y;
			dstTexel.b = srcTexel.z;
			dstTexel.transparent = srcTexel.w == 0.0;

			// If it's a white texel, it's used with night lights (i.e., yellow at night).
			const bool isWhite = (srcTexel.x == 1.0) && (srcTexel.y == 1.0) && (srcTexel.z == 1.0);

			if (isWhite)
			{
				texture.lightTexels.push_back(Int2(x, y));
			}
		}
	}
}

void SoftwareRenderer::setFlatTexture(int id, const uint32_t *srcTexels, int width, int height)
{
	const int texelCount = width * height;

	// Reset the selected texture.
	FlatTexture &texture = this->flatTextures.at(id);
	texture.texels = std::vector<FlatTexel>(texelCount);
	texture.width = width;
	texture.height = height;

	for (int i = 0; i < texelCount; i++)
	{
		const Double4 srcTexel = Double4::fromARGB(srcTexels[i]);
		FlatTexel &dstTexel = texture.texels[i];
		dstTexel.r = srcTexel.x;
		dstTexel.g = srcTexel.y;
		dstTexel.b = srcTexel.z;
		dstTexel.a = srcTexel.w;
	}
}

void SoftwareRenderer::updateFlat(int id, const Double3 *position, const double *width, 
	const double *height, const int *textureID, const bool *flipped)
{
	const auto flatIter = this->flats.find(id);
	DebugAssertMsg(flatIter != this->flats.end(),
		"Cannot update a non-existent flat (" + std::to_string(id) + ").");

	SoftwareRenderer::Flat &flat = flatIter->second;

	// Check which values requested updating and update them.
	if (position != nullptr)
	{
		flat.position = *position;
	}

	if (width != nullptr)
	{
		flat.width = *width;
	}

	if (height != nullptr)
	{
		flat.height = *height;
	}

	if (textureID != nullptr)
	{
		flat.textureID = *textureID;
	}

	if (flipped != nullptr)
	{
		flat.flipped = *flipped;
	}
}

void SoftwareRenderer::updateLight(int id, const Double3 *point,
	const Double3 *color, const double *intensity)
{
	DebugNotImplemented();
}

void SoftwareRenderer::setFogDistance(double fogDistance)
{
	this->fogDistance = fogDistance;
}

void SoftwareRenderer::setDistantSky(const DistantSky &distantSky)
{
	// Clear old distant sky data.
	this->distantObjects.clear();
	this->skyTextures.clear();

	// Creates a render texture from the given surface, adds it to the sky textures list, and
	// returns its index in the sky textures list.
	auto addSkyTexture = [this](const Surface &surface)
	{
		const int width = surface.getWidth();
		const int height = surface.getHeight();
		const uint32_t *texels = static_cast<const uint32_t*>(surface.getPixels());
		const int texelCount = width * height;

		this->skyTextures.push_back(SkyTexture());
		SkyTexture &texture = this->skyTextures.back();
		texture.texels = std::vector<SkyTexel>(texelCount);
		texture.width = width;
		texture.height = height;

		for (int i = 0; i < texelCount; i++)
		{
			const Double4 srcTexel = Double4::fromARGB(texels[i]);
			SkyTexel &dstTexel = texture.texels[i];
			dstTexel.r = srcTexel.x;
			dstTexel.g = srcTexel.y;
			dstTexel.b = srcTexel.z;
			dstTexel.transparent = srcTexel.w == 0.0;
		}

		return static_cast<int>(this->skyTextures.size()) - 1;
	};

	// Reverse iterate through each distant object type in the distant sky, creating associations
	// between the distant sky object and its render texture.
	for (int i = distantSky.getLandObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::LandObject &landObject = distantSky.getLandObject(i);
		const int textureIndex = addSkyTexture(landObject.getSurface());
		this->distantObjects.push_back(DistantObject(
			textureIndex, DistantObject::Type::Land, &landObject));
	}

	for (int i = distantSky.getAnimatedLandObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::AnimatedLandObject &animLandObject = distantSky.getAnimatedLandObject(i);

		// Add first texture to get the start index of the animated textures.
		const int textureIndex = addSkyTexture(animLandObject.getSurface(0));

		for (int j = 1; j < animLandObject.getSurfaceCount(); j++)
		{
			addSkyTexture(animLandObject.getSurface(j));
		}

		this->distantObjects.push_back(DistantObject(
			textureIndex, DistantObject::Type::AnimatedLand, &animLandObject));
	}

	for (int i = distantSky.getAirObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::AirObject &airObject = distantSky.getAirObject(i);
		const int textureIndex = addSkyTexture(airObject.getSurface());
		this->distantObjects.push_back(DistantObject(
			textureIndex, DistantObject::Type::Air, &airObject));
	}

	for (int i = distantSky.getSpaceObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::SpaceObject &spaceObject = distantSky.getSpaceObject(i);
		const int textureIndex = addSkyTexture(spaceObject.getSurface());
		this->distantObjects.push_back(DistantObject(
			textureIndex, DistantObject::Type::Space, &spaceObject));
	}

	// Add the sun to the sky textures and assign its texture index. It isn't added to
	// the list of distant objects because its position is a function of time of day,
	// and that's handled in the render method.
	this->sunTextureIndex = addSkyTexture(distantSky.getSunSurface());
}

void SoftwareRenderer::setSkyPalette(const uint32_t *colors, int count)
{
	this->skyPalette = std::vector<Double3>(count);

	for (size_t i = 0; i < this->skyPalette.size(); i++)
	{
		this->skyPalette[i] = Double3::fromRGB(colors[i]);
	}
}

void SoftwareRenderer::setNightLightsActive(bool active)
{
	// @todo: activate lights (don't worry about textures).

	// Change voxel texels based on whether it's night.
	const Double4 texelColor = Double4::fromARGB(
		(active ? Color(255, 166, 0) : Color::Black).toARGB());
	const double texelEmission = active ? 1.0 : 0.0;

	for (auto &voxelTexture : this->voxelTextures)
	{
		auto &texels = voxelTexture.texels;

		for (const auto &lightTexels : voxelTexture.lightTexels)
		{
			const int index = lightTexels.x + (lightTexels.y * VoxelTexture::WIDTH);

			VoxelTexel &texel = texels.at(index);
			texel.r = texelColor.x;
			texel.g = texelColor.y;
			texel.b = texelColor.z;
			texel.transparent = texelColor.w == 0.0;
			texel.emission = texelEmission;
		}
	}
}

void SoftwareRenderer::removeFlat(int id)
{
	// Make sure the flat exists before removing it.
	const auto flatIter = this->flats.find(id);
	DebugAssertMsg(flatIter != this->flats.end(),
		"Cannot remove a non-existent flat (" + std::to_string(id) + ").");

	this->flats.erase(flatIter);
}

void SoftwareRenderer::removeLight(int id)
{
	DebugNotImplemented();
}

void SoftwareRenderer::clearTextures()
{
	for (auto &texture : this->voxelTextures)
	{
		std::fill(texture.texels.begin(), texture.texels.end(), VoxelTexel());
		texture.lightTexels.clear();
	}

	for (auto &texture : this->flatTextures)
	{
		std::fill(texture.texels.begin(), texture.texels.end(), FlatTexel());
		texture.width = 0;
		texture.height = 0;
	}

	// Distant sky textures are cleared because the vector size is managed internally.
	this->skyTextures.clear();
	this->sunTextureIndex = SoftwareRenderer::NO_SUN;
}

void SoftwareRenderer::clearDistantSky()
{
	this->distantObjects.clear();
}

void SoftwareRenderer::resize(int width, int height)
{
	const int pixelCount = width * height;
	this->depthBuffer.resize(pixelCount);
	std::fill(this->depthBuffer.begin(), this->depthBuffer.end(), 
		std::numeric_limits<double>::infinity());

	this->occlusion.resize(width);
	std::fill(this->occlusion.begin(), this->occlusion.end(), OcclusionData(0, height));

	this->width = width;
	this->height = height;

	// Restart render threads with new dimensions.
	const int threadCount = SoftwareRenderer::getRenderThreadsFromMode(this->renderThreadsMode);
	this->initRenderThreads(width, height, threadCount);
}

void SoftwareRenderer::initRenderThreads(int width, int height, int threadCount)
{
	// If there are existing threads, reset them.
	if (this->renderThreads.size() > 0)
	{
		this->resetRenderThreads();
	}

	// If more or fewer threads are requested, re-allocate the render thread list.
	if (this->renderThreads.size() != threadCount)
	{
		this->renderThreads.resize(threadCount);
	}

	// Block width and height are the approximate number of columns and rows per thread,
	// respectively.
	const double blockWidth = static_cast<double>(width) / static_cast<double>(threadCount);
	const double blockHeight = static_cast<double>(height) / static_cast<double>(threadCount);

	// Start thread loop for each render thread. Rounding is involved so the start and stop
	// coordinates are correct for all resolutions.
	for (size_t i = 0; i < this->renderThreads.size(); i++)
	{
		const int threadIndex = static_cast<int>(i);
		const int startX = static_cast<int>(std::round(static_cast<double>(i) * blockWidth));
		const int endX = static_cast<int>(std::round(static_cast<double>(i + 1) * blockWidth));
		const int startY = static_cast<int>(std::round(static_cast<double>(i) * blockHeight));
		const int endY = static_cast<int>(std::round(static_cast<double>(i + 1) * blockHeight));

		// Make sure the rounding is correct.
		assert(startX >= 0);
		assert(endX <= width);
		assert(startY >= 0);
		assert(endY <= height);

		this->renderThreads.at(i) = std::thread(SoftwareRenderer::renderThreadLoop,
			std::ref(this->threadData), threadIndex, startX, endX, startY, endY);
	}
}

void SoftwareRenderer::resetRenderThreads()
{
	// Tell each render thread it needs to terminate.
	std::unique_lock<std::mutex> lk(this->threadData.mutex);
	this->threadData.go = true;
	this->threadData.isDestructing = true;
	lk.unlock();
	this->threadData.condVar.notify_all();

	for (auto &thread : this->renderThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	// Set signal variables back to defaults, in case the render threads are used again.
	this->threadData.go = false;
	this->threadData.isDestructing = false;
}

void SoftwareRenderer::updateVisibleDistantObjects(bool parallaxSky, const Double3 &sunDirection,
	const Camera &camera, const FrameView &frame)
{
	this->visDistantObjs.clear();

	// Directions forward and along the edges of the 2D frustum.
	const Double2 forward(camera.forwardX, camera.forwardZ);
	const Double2 frustumLeft(camera.frustumLeftX, camera.frustumLeftZ);
	const Double2 frustumRight(camera.frustumRightX, camera.frustumRightZ);

	// Directions perpendicular to frustum vectors, for determining what points
	// are inside the frustum. Both directions point towards the inside.
	const Double2 frustumLeftPerp = frustumLeft.rightPerp();
	const Double2 frustumRightPerp = frustumRight.leftPerp();

	// Determines the vertical offset of the rendered object's origin on-screen. Most
	// objects have their origin at the bottom, but the sun has its origin at the top so
	// that when it's 6am or 6pm, its top edge will be at the horizon.
	enum class Orientation { Top, Bottom };

	// Lambda for checking if the given object properties make it appear on-screen, and if
	// so, adding it to the visible objects list.
	auto tryAddObject = [this, parallaxSky, &camera, &frame, &forward, &frustumLeftPerp,
		&frustumRightPerp](const SkyTexture &texture, double xAngleRadians, double yAngleRadians,
			bool emissive, Orientation orientation)
	{
		// The size of textures in world space is based on 320px being 1 unit, and a 320px
		// wide texture spans a screen's worth of horizontal FOV.
		constexpr double identityDim = 320.0;
		constexpr double identityAngleRadians = 90.0 * Constants::DegToRad;
		const double objWidth = static_cast<double>(texture.width) / identityDim;
		const double objHeight = static_cast<double>(texture.height) / identityDim;
		const double objHalfWidth = objWidth * 0.50;

		// Y position on-screen is the same regardless of parallax.
		DrawRange drawRange = [yAngleRadians, orientation, objHeight, &camera, &frame]()
		{
			// Project the bottom first then add the object's height above it in screen-space
			// to get the top. This keeps objects from appearing squished the higher they are
			// in the sky. Don't need to worry about cases when the Y angle is at an extreme;
			// the start and end projections will both be off-screen (i.e., +inf or -inf).
			const Double3 objDirBottom = Double3(
				camera.forwardX,
				std::tan(yAngleRadians),
				camera.forwardZ).normalized();

			const Double3 objPointBottom = camera.eye + objDirBottom;

			const double yProjEnd = SoftwareRenderer::getProjectedY(
				objPointBottom, camera.transform, camera.yShear);
			const double yProjStart = yProjEnd - (objHeight * camera.zoom);

			const double yProjBias = (orientation == Orientation::Top) ?
				(yProjEnd - yProjStart) : 0.0;

			const double yProjScreenStart = (yProjStart + yProjBias) * frame.heightReal;
			const double yProjScreenEnd = (yProjEnd + yProjBias) * frame.heightReal;

			const int yStart = SoftwareRenderer::getLowerBoundedPixel(
				yProjScreenStart, frame.height);
			const int yEnd = SoftwareRenderer::getUpperBoundedPixel(
				yProjScreenEnd, frame.height);

			return DrawRange(yProjScreenStart, yProjScreenEnd, yStart, yEnd);
		}();

		// The position of the object's left and right edges depends on whether parallax
		// is enabled.
		if (parallaxSky)
		{
			// Get X angles for left and right edges based on object half width.
			const double xDeltaRadians = objHalfWidth * identityAngleRadians;
			const double xAngleRadiansLeft = xAngleRadians + xDeltaRadians;
			const double xAngleRadiansRight = xAngleRadians - xDeltaRadians;
			
			// Camera's horizontal field of view.
			const double cameraHFov = MathUtils::verticalFovToHorizontalFov(
				camera.fovY, camera.aspect);
			const double halfCameraHFovRadians = (cameraHFov * 0.50) * Constants::DegToRad;

			// Angles of the camera's forward vector and frustum edges.
			const double cameraAngleRadians = camera.getXZAngleRadians();
			const double cameraAngleLeft = cameraAngleRadians + halfCameraHFovRadians;
			const double cameraAngleRight = cameraAngleRadians - halfCameraHFovRadians;

			// Distant object visible angle range and texture coordinates, set by onScreen.
			double xVisAngleLeft, xVisAngleRight;
			double uStart, uEnd;

			// Determine if the object is at least partially on-screen. The angle range of the
			// object must be at least partially within the angle range of the camera.
			const bool onScreen = [&camera, xAngleRadiansLeft, xAngleRadiansRight, cameraAngleLeft,
				cameraAngleRight, &xVisAngleLeft, &xVisAngleRight, &uStart, &uEnd]()
			{
				// Need to handle special cases where the angle ranges span 0.
				const bool cameraIsGeneralCase = cameraAngleLeft < Constants::TwoPi;
				const bool objectIsGeneralCase = xAngleRadiansLeft < Constants::TwoPi;

				if (cameraIsGeneralCase == objectIsGeneralCase)
				{
					// Both are either general case or special case; no extra behavior necessary.
					xVisAngleLeft = std::min(xAngleRadiansLeft, cameraAngleLeft);
					xVisAngleRight = std::max(xAngleRadiansRight, cameraAngleRight);
				}
				else if (!cameraIsGeneralCase)
				{
					// Camera special case.
					// @todo: cut into two parts?
					xVisAngleLeft = std::min(
						xAngleRadiansLeft, (cameraAngleLeft - Constants::TwoPi));
					xVisAngleRight = std::max(
						xAngleRadiansRight, (cameraAngleRight - Constants::TwoPi));
				}
				else
				{
					// Object special case.
					// @todo: cut into two parts?
					xVisAngleLeft = std::min(
						(xAngleRadiansLeft - Constants::TwoPi), cameraAngleLeft);
					xVisAngleRight = std::max(
						(xAngleRadiansRight - Constants::TwoPi), cameraAngleRight);
				}

				uStart = 1.0 - ((xVisAngleLeft - xAngleRadiansRight) /
					(xAngleRadiansLeft - xAngleRadiansRight));
				uEnd = Constants::JustBelowOne - ((xAngleRadiansRight - xVisAngleRight) /
					(xAngleRadiansRight - xAngleRadiansLeft));

				return (xAngleRadiansLeft >= cameraAngleRight) &&
					(xAngleRadiansRight <= cameraAngleLeft);
			}();

			if (onScreen)
			{
				// Data for parallax texture sampling.
				VisDistantObject::ParallaxData parallax(
					xVisAngleLeft, xVisAngleRight, uStart, uEnd);

				const Double2 objDirLeft2D(
					std::sin(xAngleRadiansLeft),
					std::cos(xAngleRadiansLeft));
				const Double2 objDirRight2D(
					std::sin(xAngleRadiansRight),
					std::cos(xAngleRadiansRight));

				// Project vertical edges.
				const Double3 objDirLeft(objDirLeft2D.x, 0.0, objDirLeft2D.y);
				const Double3 objDirRight(objDirRight2D.x, 0.0, objDirRight2D.y);

				const Double3 objPointLeft = camera.eye + objDirLeft;
				const Double3 objPointRight = camera.eye + objDirRight;

				const Double4 objProjPointLeft = camera.transform * Double4(objPointLeft, 1.0);
				const Double4 objProjPointRight = camera.transform * Double4(objPointRight, 1.0);

				const double xProjStart = 0.50 + ((objProjPointLeft.x / objProjPointLeft.w) * 0.50);
				const double xProjEnd = 0.50 + ((objProjPointRight.x / objProjPointRight.w) * 0.50);

				// Get the start and end X pixel coordinates.
				const int xDrawStart = SoftwareRenderer::getLowerBoundedPixel(
					xProjStart * frame.widthReal, frame.width);
				const int xDrawEnd = SoftwareRenderer::getUpperBoundedPixel(
					xProjEnd * frame.widthReal, frame.width);

				this->visDistantObjs.push_back(VisDistantObject(
					texture, std::move(drawRange), std::move(parallax), xProjStart, xProjEnd,
					xDrawStart, xDrawEnd, emissive));
			}
		}
		else
		{
			// Classic rendering. Render the object based on its midpoint.
			const Double3 objDir(
				std::sin(xAngleRadians),
				0.0,
				std::cos(xAngleRadians));

			// Create a point arbitrarily far away for the object's center in world space.
			const Double3 objPoint = camera.eye + objDir;

			// Project the center point on-screen and get its projected X coordinate.
			const Double4 objProjPoint = camera.transform * Double4(objPoint, 1.0);
			const double xProjCenter = 0.50 + ((objProjPoint.x / objProjPoint.w) * 0.50);

			// Calculate the projected width of the object so we can get the left and right X
			// coordinates on-screen.
			const double objProjWidth = (objWidth * camera.zoom) /
				(camera.aspect * SoftwareRenderer::TALL_PIXEL_RATIO);
			const double objProjHalfWidth = objProjWidth * 0.50;

			// Left and right coordinates of the object in screen space.
			const double xProjStart = xProjCenter - objProjHalfWidth;
			const double xProjEnd = xProjCenter + objProjHalfWidth;

			const Double2 objDir2D(objDir.x, objDir.z);
			const bool onScreen = (objDir2D.dot(forward) > 0.0) &&
				(xProjStart <= 1.0) && (xProjEnd >= 0.0);

			if (onScreen)
			{
				// Get the start and end X pixel coordinates.
				const int xDrawStart = SoftwareRenderer::getLowerBoundedPixel(
					xProjStart * frame.widthReal, frame.width);
				const int xDrawEnd = SoftwareRenderer::getUpperBoundedPixel(
					xProjEnd * frame.widthReal, frame.width);

				this->visDistantObjs.push_back(VisDistantObject(
					texture, std::move(drawRange), xProjStart, xProjEnd, xDrawStart,
					xDrawEnd, emissive));
			}
		}
	};

	// Iterate all distant objects and gather up the visible ones.
	for (const auto &obj : this->distantObjects)
	{
		const SkyTexture *texture = nullptr;
		double xAngleRadians, yAngleRadians;
		bool emissive;
		Orientation orientation;

		if (obj.type == DistantObject::Type::Land)
		{
			const DistantSky::LandObject &land = *obj.land;
			texture = &skyTextures.at(obj.textureIndex);
			xAngleRadians = land.getAngleRadians();
			yAngleRadians = 0.0;
			emissive = false;
			orientation = Orientation::Bottom;
		}
		else if (obj.type == DistantObject::Type::AnimatedLand)
		{
			const DistantSky::AnimatedLandObject &animLand = *obj.animLand;
			texture = &skyTextures.at(obj.textureIndex + animLand.getIndex());
			xAngleRadians = animLand.getAngleRadians();
			yAngleRadians = 0.0;
			emissive = true;
			orientation = Orientation::Bottom;
		}
		else if (obj.type == DistantObject::Type::Air)
		{
			const DistantSky::AirObject &air = *obj.air;
			texture = &skyTextures.at(obj.textureIndex);
			xAngleRadians = air.getAngleRadians();
			yAngleRadians = [&air]()
			{
				// 0 is at horizon, 1 is at top of distant cloud height limit.
				const double gradientPercent = air.getHeight();
				return gradientPercent *
					(SoftwareRenderer::DISTANT_CLOUDS_MAX_ANGLE * Constants::DegToRad);
			}();

			emissive = false;
			orientation = Orientation::Bottom;
		}
		else if (obj.type == DistantObject::Type::Space)
		{
			const DistantSky::SpaceObject &space = *obj.space;
			DebugNotImplemented();
		}
		else
		{
			throw DebugException("Invalid distant object type \"" +
				std::to_string(static_cast<int>(obj.type)) + "\".");
		}

		tryAddObject(*texture, xAngleRadians, yAngleRadians, emissive, orientation);
	}

	// Try to add the sun to the visible distant objects.
	if (this->sunTextureIndex != SoftwareRenderer::NO_SUN)
	{
		const SkyTexture &sunTexture = this->skyTextures.at(this->sunTextureIndex);
		const double sunXAngleRadians = MathUtils::fullAtan2(sunDirection.x, sunDirection.z);

		// When the sun is directly above or below, it might cause the X angle to be undefined.
		// We want to filter this out before we try projecting it on-screen.
		if (std::isfinite(sunXAngleRadians))
		{
			const double sunYAngleRadians = sunDirection.getYAngleRadians();
			const bool sunEmissive = true;
			const Orientation sunOrientation = Orientation::Top;
			tryAddObject(sunTexture, sunXAngleRadians, sunYAngleRadians,
				sunEmissive, sunOrientation);
		}
	}
}

void SoftwareRenderer::updateVisibleFlats(const Camera &camera)
{
	this->visibleFlats.clear();

	// Each flat shares the same axes. The forward direction always faces opposite to 
	// the camera direction.
	const Double3 flatForward = Double3(-camera.forwardX, 0.0, -camera.forwardZ).normalized();
	const Double3 flatUp = Double3::UnitY;
	const Double3 flatRight = flatForward.cross(flatUp).normalized();

	const Double2 eye2D(camera.eye.x, camera.eye.z);
	const Double2 direction(camera.forwardX, camera.forwardZ);

	// This is the visible flat determination algorithm. It goes through all flats and sees 
	// which ones would be at least partially visible in the view frustum.
	for (const auto &pair : this->flats)
	{
		const Flat &flat = pair.second;

		// Scaled axes based on flat dimensions.
		const Double3 flatRightScaled = flatRight * (flat.width * 0.50);
		const Double3 flatUpScaled = flatUp * flat.height;

		// Calculate each corner of the flat in world space.
		Flat::Frame flatFrame;
		flatFrame.bottomStart = flat.position + flatRightScaled;
		flatFrame.bottomEnd = flat.position - flatRightScaled;
		flatFrame.topStart = flatFrame.bottomStart + flatUpScaled;
		flatFrame.topEnd = flatFrame.bottomEnd + flatUpScaled;

		// If the flat is somewhere in front of the camera, do further checks.
		const Double2 flatPosition2D(flat.position.x, flat.position.z);
		const Double2 flatEyeDiff = (flatPosition2D - eye2D).normalized();
		const bool inFrontOfCamera = direction.dot(flatEyeDiff) > 0.0;

		if (inFrontOfCamera)
		{
			// Now project two of the flat's opposing corner points into camera space.
			// The Z value is used with flat sorting (not rendering), and the X and Y values 
			// are used to find where the flat is on-screen.
			Double4 projStart = camera.transform * Double4(flatFrame.topStart, 1.0);
			Double4 projEnd = camera.transform * Double4(flatFrame.bottomEnd, 1.0);

			// Normalize coordinates.
			projStart = projStart / projStart.w;
			projEnd = projEnd / projEnd.w;

			// Assign each screen value to the flat frame data.
			flatFrame.startX = 0.50 + (projStart.x * 0.50);
			flatFrame.endX = 0.50 + (projEnd.x * 0.50);
			flatFrame.startY = (0.50 + camera.yShear) - (projStart.y * 0.50);
			flatFrame.endY = (0.50 + camera.yShear) - (projEnd.y * 0.50);
			flatFrame.z = projStart.z;

			// Check that the Z value is within the clipping planes.
			const bool inPlanes = (flatFrame.z >= SoftwareRenderer::NEAR_PLANE) &&
				(flatFrame.z <= SoftwareRenderer::FAR_PLANE);

			if (inPlanes)
			{
				// Add the flat data to the draw list.
				this->visibleFlats.push_back(VisibleFlat(flat, std::move(flatFrame)));
			}
		}
	}

	// Sort the visible flats farthest to nearest (relevant for transparencies).
	std::sort(this->visibleFlats.begin(), this->visibleFlats.end(),
		[](const VisibleFlat &a, const VisibleFlat &b)
	{
		return a.getFrame().z > b.getFrame().z;
	});
}

/*Double3 SoftwareRenderer::castRay(const Double3 &direction,
	const VoxelGrid &voxelGrid) const
{
	// This is an extension of Lode Vandevenne's DDA algorithm from 2D to 3D.
	// Technically, it could be considered a "3D-DDA" algorithm. It will eventually 
	// have some additional features so all of Arena's geometry can be represented.

	// @todo:
	// - Figure out proper DDA lengths for variable-height voxels, and why using
	//   voxelHeight squared instead of 1.0 in deltaDist.y looks weird (sideDist.y?).
	// - Cuboids within voxels (bridges, beds, shelves) with variable Y offset and size.
	// - Sprites (SpriteGrid? Sprite data, and list of sprite IDs per voxel).
	// - Transparent textures (check texel alpha in DDA loop).
	// - Sky (if hitID == 0).
	// - Shading (shadows from the sun, point lights).

	// Some floating point behavior assumptions:
	// -> (value / 0.0) == infinity
	// -> (value / infinity) == 0.0
	// -> (int)(-0.8) == 0
	// -> (int)floor(-0.8) == -1
	// -> (int)ceil(-0.8) == 0

	const Double3 dirSquared(
		direction.x * direction.x,
		direction.y * direction.y,
		direction.z * direction.z);

	// Height (Y size) of each voxel in the voxel grid. Some levels in Arena have
	// "tall" voxels, so the voxel height must be a variable.
	const double voxelHeight = voxelGrid.getVoxelHeight();

	// A custom variable that represents the Y "floor" of the current voxel.
	const double eyeYRelativeFloor = std::floor(this->eye.y / voxelHeight) * voxelHeight;

	// Calculate delta distances along each axis. These determine how far
	// the ray has to go until the next X, Y, or Z side is hit, respectively.
	const Double3 deltaDist(
		std::sqrt(1.0 + (dirSquared.y / dirSquared.x) + (dirSquared.z / dirSquared.x)),
		std::sqrt(1.0 + (dirSquared.x / dirSquared.y) + (dirSquared.z / dirSquared.y)),
		std::sqrt(1.0 + (dirSquared.x / dirSquared.z) + (dirSquared.y / dirSquared.z)));

	// Booleans for whether a ray component is non-negative. Used with step directions 
	// and texture coordinates.
	const bool nonNegativeDirX = direction.x >= 0.0;
	const bool nonNegativeDirY = direction.y >= 0.0;
	const bool nonNegativeDirZ = direction.z >= 0.0;

	// Calculate step directions and initial side distances.
	Int3 step;
	Double3 sideDist;
	if (nonNegativeDirX)
	{
		step.x = 1;
		sideDist.x = (this->startCellReal.x + 1.0 - this->eye.x) * deltaDist.x;
	}
	else
	{
		step.x = -1;
		sideDist.x = (this->eye.x - this->startCellReal.x) * deltaDist.x;
	}

	if (nonNegativeDirY)
	{
		step.y = 1;
		sideDist.y = (eyeYRelativeFloor + voxelHeight - this->eye.y) * deltaDist.y;
	}
	else
	{
		step.y = -1;
		sideDist.y = (this->eye.y - eyeYRelativeFloor) * deltaDist.y;
	}

	if (nonNegativeDirZ)
	{
		step.z = 1;
		sideDist.z = (this->startCellReal.z + 1.0 - this->eye.z) * deltaDist.z;
	}
	else
	{
		step.z = -1;
		sideDist.z = (this->eye.z - this->startCellReal.z) * deltaDist.z;
	}

	// Make a copy of the initial side distances. They are used for the special case 
	// of the ray ending in the same voxel it started in.
	const Double3 initialSideDist = sideDist;

	// Make a copy of the step magnitudes, converted to doubles.
	const Double3 stepReal(
		static_cast<double>(step.x),
		static_cast<double>(step.y),
		static_cast<double>(step.z));

	// Get initial voxel coordinates.
	Int3 cell = this->startCell;

	// ID of a hit voxel. Zero (air) by default.
	char hitID = 0;

	// Axis of a hit voxel's side. X by default.
	enum class Axis { X, Y, Z };
	Axis axis = Axis::X;

	// Distance squared (in voxels) that the ray has stepped. Square roots are
	// too slow to use in the DDA loop, so this is used instead.
	// - When using variable-sized voxels, this may be calculated differently.
	double cellDistSquared = 0.0;

	// Offset values for which corner of a voxel to compare the distance 
	// squared against. The correct corner to use is important when culling
	// shapes at max view distance.
	const Double3 startCellWithOffset(
		this->startCellReal.x + ((1.0 + stepReal.x) / 2.0),
		eyeYRelativeFloor + (((1.0 + stepReal.y) / 2.0) * voxelHeight),
		this->startCellReal.z + ((1.0 + stepReal.z) / 2.0));
	const Double3 cellOffset(
		(1.0 - stepReal.x) / 2.0,
		((1.0 - stepReal.y) / 2.0) * voxelHeight,
		(1.0 - stepReal.z) / 2.0);

	// Get dimensions of the voxel grid.
	const int gridWidth = voxelGrid.getWidth();
	const int gridHeight = voxelGrid.getHeight();
	const int gridDepth = voxelGrid.getDepth();

	// Check world bounds on the start voxel. Bounds are partially recalculated 
	// for axes that the DDA loop is stepping through.
	bool voxelIsValid = (cell.x >= 0) && (cell.y >= 0) && (cell.z >= 0) &&
		(cell.x < gridWidth) && (cell.y < gridHeight) && (cell.z < gridDepth);

	// Step through the voxel grid while the current coordinate is valid and
	// the total voxel distance stepped is less than the view distance.
	// (Note that the "voxel distance" is not the same as "actual" distance.)
	const char *voxels = voxelGrid.getVoxels();
	while (voxelIsValid && (cellDistSquared < this->viewDistSquared))
	{
		// Get the index of the current voxel in the voxel grid.
		const int gridIndex = cell.x + (cell.y * gridWidth) +
			(cell.z * gridWidth * gridHeight);

		// Check if the current voxel is solid.
		const char voxelID = voxels[gridIndex];

		if (voxelID > 0)
		{
			hitID = voxelID;
			break;
		}

		if ((sideDist.x < sideDist.y) && (sideDist.x < sideDist.z))
		{
			sideDist.x += deltaDist.x;
			cell.x += step.x;
			axis = Axis::X;
			voxelIsValid &= (cell.x >= 0) && (cell.x < gridWidth);
		}
		else if (sideDist.y < sideDist.z)
		{
			sideDist.y += deltaDist.y;
			cell.y += step.y;
			axis = Axis::Y;
			voxelIsValid &= (cell.y >= 0) && (cell.y < gridHeight);
		}
		else
		{
			sideDist.z += deltaDist.z;
			cell.z += step.z;
			axis = Axis::Z;
			voxelIsValid &= (cell.z >= 0) && (cell.z < gridDepth);
		}

		// Refresh how far the current cell is from the start cell, squared.
		// The "offsets" move each point to the correct corner for each voxel
		// so that the stepping stops correctly at max view distance.
		const Double3 cellDiff(
			(static_cast<double>(cell.x) + cellOffset.x) - startCellWithOffset.x,
			(static_cast<double>(cell.y) + cellOffset.y) - startCellWithOffset.y,
			(static_cast<double>(cell.z) + cellOffset.z) - startCellWithOffset.z);
		cellDistSquared = (cellDiff.x * cellDiff.x) + (cellDiff.y * cellDiff.y) +
			(cellDiff.z * cellDiff.z);
	}

	// Boolean for whether the ray ended in the same voxel it started in.
	const bool stoppedInFirstVoxel = cell == this->startCell;

	// Get the distance from the camera to the hit point. It is a special case
	// if the ray stopped in the first voxel.
	double distance;
	if (stoppedInFirstVoxel)
	{
		if ((initialSideDist.x < initialSideDist.y) &&
			(initialSideDist.x < initialSideDist.z))
		{
			distance = initialSideDist.x;
			axis = Axis::X;
		}
		else if (initialSideDist.y < initialSideDist.z)
		{
			distance = initialSideDist.y;
			axis = Axis::Y;
		}
		else
		{
			distance = initialSideDist.z;
			axis = Axis::Z;
		}
	}
	else
	{
		const size_t axisIndex = static_cast<size_t>(axis);

		// Assign to distance based on which axis was hit.
		if (axis == Axis::X)
		{
			distance = (static_cast<double>(cell.x) - this->eye.x +
				((1.0 - stepReal.x) / 2.0)) / direction.x;
		}
		else if (axis == Axis::Y)
		{
			distance = ((static_cast<double>(cell.y) * voxelHeight) - this->eye.y +
				(((1.0 - stepReal.y) / 2.0) * voxelHeight)) / direction.y;
		}
		else
		{
			distance = (static_cast<double>(cell.z) - this->eye.z +
				((1.0 - stepReal.z) / 2.0)) / direction.z;
		}
	}

	// If there was a hit, get the shaded color.
	if (hitID > 0)
	{
		// Intersection point on the voxel.
		const Double3 hitPoint = this->eye + (direction * distance);

		// Boolean for whether the hit point is on the back of a voxel face.
		const bool backFace = stoppedInFirstVoxel;

		// Texture coordinates. U and V are affected by which side is hit (near, far),
		// and whether the hit point is on the front or back of the voxel face.
		// - Note, for edge cases where {u,v}Val == 1.0, the texture coordinate is
		//   out of bounds by one pixel, so instead of 1.0, something like 0.9999999
		//   should be used instead. std::nextafter(1.0, -INFINITY)?
		double u, v;
		if (axis == Axis::X)
		{
			const double uVal = hitPoint.z - std::floor(hitPoint.z);

			u = (nonNegativeDirX ^ backFace) ? uVal : (1.0 - uVal);
			//v = 1.0 - (hitPoint.y - std::floor(hitPoint.y));
			v = 1.0 - (std::fmod(hitPoint.y, voxelHeight) / voxelHeight);
		}
		else if (axis == Axis::Y)
		{
			const double vVal = hitPoint.x - std::floor(hitPoint.x);

			u = hitPoint.z - std::floor(hitPoint.z);
			v = (nonNegativeDirY ^ backFace) ? vVal : (1.0 - vVal);
		}
		else
		{
			const double uVal = hitPoint.x - std::floor(hitPoint.x);

			u = (nonNegativeDirZ ^ backFace) ? (1.0 - uVal) : uVal;
			//v = 1.0 - (hitPoint.y - std::floor(hitPoint.y));
			v = 1.0 - (std::fmod(hitPoint.y, voxelHeight) / voxelHeight);
		}

		// -- temp --
		// Display bad texture coordinates as magenta. If any of these is true, it
		// means something above is wrong.
		if ((u < 0.0) || (u >= 1.0) || (v < 0.0) || (v >= 1.0))
		{
			return Double3(1.0, 0.0, 1.0);
		}
		// -- end temp --

		// Get the voxel data associated with the ID. Subtract 1 because the first
		// entry is at index 0 but the lowest hitID is 1.
		const VoxelData &voxelData = voxelGrid.getVoxelData(hitID - 1);

		// Get the texture depending on which face was hit.
		const TextureData &texture = (axis == Axis::Y) ?
			this->textures[voxelData.floorAndCeilingID] :
			this->textures[voxelData.sideID];

		// Calculate position in texture.
		const int textureX = static_cast<int>(u * texture.width);
		const int textureY = static_cast<int>(v * texture.height);

		// Get the texel color at the hit point.
		// - Later, the alpha component can be used for transparency and ignoring
		//   intersections (in the DDA loop).
		const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

		// Convert the texel to a 3-component color.
		const Double3 color(texel.x, texel.y, texel.z);

		// Linearly interpolate with some depth.
		const double depth = std::min(distance, this->viewDistance) / this->viewDistance;
		return color.lerp(this->fogColor, depth);
	}
	else
	{
		// No intersection. Return sky color.
		return this->fogColor;
	}
}*/

int SoftwareRenderer::getRenderThreadsFromMode(int mode)
{
	if (mode == 0)
	{
		// Low.
		return 1;
	}
	else if (mode == 1)
	{
		// Medium.
		return std::max(Platform::getThreadCount() / 2, 1);
	}
	else if (mode == 2)
	{
		// High.
		return std::max(Platform::getThreadCount() - 1, 1);
	}
	else if (mode == 3)
	{
		// Max.
		return Platform::getThreadCount();
	}
	else
	{
		throw DebugException("Invalid render threads mode \"" +
			std::to_string(mode) + "\".");
	}
}

VoxelData::Facing SoftwareRenderer::getInitialChasmFarFacing(int voxelX, int voxelZ,
	const Double2 &eye, const Ray &ray)
{
	// Angle of the ray from the camera eye.
	const double angle = MathUtils::fullAtan2(ray.dirX, ray.dirZ);

	// Corners in world space.
	const Double2 bottomLeftCorner(
		static_cast<double>(voxelX),
		static_cast<double>(voxelZ));
	const Double2 topLeftCorner(
		bottomLeftCorner.x + 1.0,
		bottomLeftCorner.y);
	const Double2 bottomRightCorner(
		bottomLeftCorner.x,
		bottomLeftCorner.y + 1.0);
	const Double2 topRightCorner(
		topLeftCorner.x,
		bottomRightCorner.y);

	const Double2 upLeft = (topLeftCorner - eye).normalized();
	const Double2 upRight = (topRightCorner - eye).normalized();
	const Double2 downLeft = (bottomLeftCorner - eye).normalized();
	const Double2 downRight = (bottomRightCorner - eye).normalized();
	const double upLeftAngle = MathUtils::fullAtan2(upLeft.x, upLeft.y);
	const double upRightAngle = MathUtils::fullAtan2(upRight.x, upRight.y);
	const double downLeftAngle = MathUtils::fullAtan2(downLeft.x, downLeft.y);
	const double downRightAngle = MathUtils::fullAtan2(downRight.x, downRight.y);

	// Find which range the ray's angle lies within.
	if ((angle < upRightAngle) || (angle > downRightAngle))
	{
		return VoxelData::Facing::PositiveZ;
	}
	else if (angle < upLeftAngle)
	{
		return VoxelData::Facing::PositiveX;
	}
	else if (angle < downLeftAngle)
	{
		return VoxelData::Facing::NegativeZ;
	}
	else
	{
		return VoxelData::Facing::NegativeX;
	}
}

VoxelData::Facing SoftwareRenderer::getChasmFarFacing(int voxelX, int voxelZ, 
	VoxelData::Facing nearFacing, const Camera &camera, const Ray &ray)
{
	const Double2 eye2D(camera.eye.x, camera.eye.z);
	
	// Angle of the ray from the camera eye.
	const double angle = MathUtils::fullAtan2(ray.dirX, ray.dirZ);

	// Corners in world space.
	const Double2 bottomLeftCorner(
		static_cast<double>(voxelX),
		static_cast<double>(voxelZ));
	const Double2 topLeftCorner(
		bottomLeftCorner.x + 1.0,
		bottomLeftCorner.y);
	const Double2 bottomRightCorner(
		bottomLeftCorner.x,
		bottomLeftCorner.y + 1.0);
	const Double2 topRightCorner(
		topLeftCorner.x,
		bottomRightCorner.y);

	const Double2 upLeft = (topLeftCorner - eye2D).normalized();
	const Double2 upRight = (topRightCorner - eye2D).normalized();
	const Double2 downLeft = (bottomLeftCorner - eye2D).normalized();
	const Double2 downRight = (bottomRightCorner - eye2D).normalized();
	const double upLeftAngle = MathUtils::fullAtan2(upLeft.x, upLeft.y);
	const double upRightAngle = MathUtils::fullAtan2(upRight.x, upRight.y);
	const double downLeftAngle = MathUtils::fullAtan2(downLeft.x, downLeft.y);
	const double downRightAngle = MathUtils::fullAtan2(downRight.x, downRight.y);

	// Find which side it starts on, then do some checks against line angles.
	// When the ray origin is at a diagonal to the voxel, ignore the corner
	// closest to that origin.
	if (nearFacing == VoxelData::Facing::PositiveX)
	{
		// Starts on (1.0, z).
		const bool onRight = camera.eyeVoxel.z > voxelZ;
		const bool onLeft = camera.eyeVoxel.z < voxelZ;
		
		if (onRight)
		{
			// Ignore top-right corner.
			if (angle < downLeftAngle)
			{
				return VoxelData::Facing::NegativeZ;
			}
			else
			{
				return VoxelData::Facing::NegativeX;
			}
		}
		else if (onLeft)
		{
			// Ignore top-left corner.
			if ((angle > downLeftAngle) && (angle < downRightAngle))
			{
				return VoxelData::Facing::NegativeX;
			}
			else
			{
				return VoxelData::Facing::PositiveZ;
			}
		}
		else
		{
			if (angle > downRightAngle)
			{
				return VoxelData::Facing::PositiveZ;
			}
			else if (angle > downLeftAngle)
			{
				return VoxelData::Facing::NegativeX;
			}
			else
			{
				return VoxelData::Facing::NegativeZ;
			}
		}
	}
	else if (nearFacing == VoxelData::Facing::NegativeX)
	{
		// Starts on (0.0, z).
		const bool onRight = camera.eyeVoxel.z > voxelZ;
		const bool onLeft = camera.eyeVoxel.z < voxelZ;

		if (onRight)
		{
			// Ignore bottom-right corner.
			if (angle < upLeftAngle)
			{
				return VoxelData::Facing::PositiveX;
			}
			else
			{
				return VoxelData::Facing::NegativeZ;
			}
		}
		else if (onLeft)
		{
			// Ignore bottom-left corner.
			if (angle < upRightAngle)
			{
				return VoxelData::Facing::PositiveZ;
			}
			else
			{
				return VoxelData::Facing::PositiveX;
			}
		}
		else
		{
			if (angle < upRightAngle)
			{
				return VoxelData::Facing::PositiveZ;
			}
			else if (angle < upLeftAngle)
			{
				return VoxelData::Facing::PositiveX;
			}
			else
			{
				return VoxelData::Facing::NegativeZ;
			}
		}
	}				
	else if (nearFacing == VoxelData::Facing::PositiveZ)
	{
		// Starts on (x, 1.0).
		const bool onTop = camera.eyeVoxel.x > voxelX;
		const bool onBottom = camera.eyeVoxel.x < voxelX;

		if (onTop)
		{
			// Ignore top-right corner.
			if (angle < downLeftAngle)
			{
				return VoxelData::Facing::NegativeZ;
			}
			else
			{
				return VoxelData::Facing::NegativeX;
			}
		}
		else if (onBottom)
		{
			// Ignore bottom-right corner.
			if (angle < upLeftAngle)
			{
				return VoxelData::Facing::PositiveX;
			}
			else
			{
				return VoxelData::Facing::NegativeZ;
			}
		}
		else
		{
			if (angle < upLeftAngle)
			{
				return VoxelData::Facing::PositiveX;
			}
			else if (angle < downLeftAngle)
			{
				return VoxelData::Facing::NegativeZ;
			}
			else
			{
				return VoxelData::Facing::NegativeX;
			}
		}
	}
	else
	{
		// Starts on (x, 0.0). This one splits the origin, so it needs some 
		// special cases.
		const bool onTop = camera.eyeVoxel.x > voxelX;
		const bool onBottom = camera.eyeVoxel.x < voxelX;

		if (onTop)
		{
			// Ignore top-left corner.
			if ((angle > downLeftAngle) && (angle < downRightAngle))
			{
				return VoxelData::Facing::NegativeX;
			}
			else
			{
				return VoxelData::Facing::PositiveZ;
			}
		}
		else if (onBottom)
		{
			// Ignore bottom-left corner.
			if ((angle > upRightAngle) && (angle < upLeftAngle))
			{
				return VoxelData::Facing::PositiveX;
			}
			else
			{
				return VoxelData::Facing::PositiveZ;
			}
		}
		else
		{
			if ((angle < upRightAngle) || (angle > downRightAngle))
			{
				return VoxelData::Facing::PositiveZ;
			}
			else if (angle > downLeftAngle)
			{
				return VoxelData::Facing::NegativeX;
			}
			else
			{
				return VoxelData::Facing::PositiveX;
			}
		}
	}
}

double SoftwareRenderer::getDoorPercentOpen(int voxelX, int voxelZ,
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

double SoftwareRenderer::getProjectedY(const Double3 &point, 
	const Matrix4d &transform, double yShear)
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

int SoftwareRenderer::getLowerBoundedPixel(double projected, int frameDim)
{
	return MathUtils::clamp(static_cast<int>(std::ceil(projected - 0.50)), 0, frameDim);
}

int SoftwareRenderer::getUpperBoundedPixel(double projected, int frameDim)
{
	return MathUtils::clamp(static_cast<int>(std::floor(projected + 0.50)), 0, frameDim);
}

SoftwareRenderer::DrawRange SoftwareRenderer::makeDrawRange(const Double3 &startPoint,
	const Double3 &endPoint, const Camera &camera, const FrameView &frame)
{
	const double yProjStart = SoftwareRenderer::getProjectedY(
		startPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double yProjEnd = SoftwareRenderer::getProjectedY(
		endPoint, camera.transform, camera.yShear) * frame.heightReal;
	const int yStart = SoftwareRenderer::getLowerBoundedPixel(yProjStart, frame.height);
	const int yEnd = SoftwareRenderer::getUpperBoundedPixel(yProjEnd, frame.height);

	return DrawRange(yProjStart, yProjEnd, yStart, yEnd);
}

std::array<SoftwareRenderer::DrawRange, 2> SoftwareRenderer::makeDrawRangeTwoPart(
	const Double3 &startPoint, const Double3 &midPoint, const Double3 &endPoint,
	const Camera &camera, const FrameView &frame)
{
	const double startYProjStart = SoftwareRenderer::getProjectedY(
		startPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double startYProjEnd = SoftwareRenderer::getProjectedY(
		midPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double endYProjEnd = SoftwareRenderer::getProjectedY(
		endPoint, camera.transform, camera.yShear) * frame.heightReal;

	const int startYStart = SoftwareRenderer::getLowerBoundedPixel(startYProjStart, frame.height);
	const int startYEnd = SoftwareRenderer::getUpperBoundedPixel(startYProjEnd, frame.height);
	const int endYStart = startYEnd;
	const int endYEnd = SoftwareRenderer::getUpperBoundedPixel(endYProjEnd, frame.height);

	return std::array<DrawRange, 2>
	{
		DrawRange(startYProjStart, startYProjEnd, startYStart, startYEnd),
		DrawRange(startYProjEnd, endYProjEnd, endYStart, endYEnd)
	};
}

std::array<SoftwareRenderer::DrawRange, 3> SoftwareRenderer::makeDrawRangeThreePart(
	const Double3 &startPoint, const Double3 &midPoint1, const Double3 &midPoint2,
	const Double3 &endPoint, const Camera &camera, const FrameView &frame)
{
	const double startYProjStart = SoftwareRenderer::getProjectedY(
		startPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double startYProjEnd = SoftwareRenderer::getProjectedY(
		midPoint1, camera.transform, camera.yShear) * frame.heightReal;
	const double mid1YProjEnd = SoftwareRenderer::getProjectedY(
		midPoint2, camera.transform, camera.yShear) * frame.heightReal;
	const double mid2YProjEnd = SoftwareRenderer::getProjectedY(
		endPoint, camera.transform, camera.yShear) * frame.heightReal;

	const int startYStart = SoftwareRenderer::getLowerBoundedPixel(startYProjStart, frame.height);
	const int startYEnd = SoftwareRenderer::getUpperBoundedPixel(startYProjEnd, frame.height);
	const int mid1YStart = startYEnd;
	const int mid1YEnd = SoftwareRenderer::getUpperBoundedPixel(mid1YProjEnd, frame.height);
	const int mid2YStart = mid1YEnd;
	const int mid2YEnd = SoftwareRenderer::getUpperBoundedPixel(mid2YProjEnd, frame.height);

	return std::array<DrawRange, 3>
	{
		DrawRange(startYProjStart, startYProjEnd, startYStart, startYEnd),
		DrawRange(startYProjEnd, mid1YProjEnd, mid1YStart, mid1YEnd),
		DrawRange(mid1YProjEnd, mid2YProjEnd, mid2YStart, mid2YEnd)
	};
}

bool SoftwareRenderer::findDiag1Intersection(int voxelX, int voxelZ, const Double2 &nearPoint,
	const Double2 &farPoint, RayHit &hit)
{
	// Start, middle, and end points of the diagonal line segment relative to the grid.
	const Double2 diagStart(
		static_cast<double>(voxelX),
		static_cast<double>(voxelZ));
	const Double2 diagMiddle(
		static_cast<double>(voxelX) + 0.50,
		static_cast<double>(voxelZ) + 0.50);
	const Double2 diagEnd(
		static_cast<double>(voxelX) + Constants::JustBelowOne,
		static_cast<double>(voxelZ) + Constants::JustBelowOne);

	// Normals for the left and right faces of the wall, facing up-left and down-right
	// respectively (magic number is sqrt(2) / 2).
	const Double3 leftNormal(0.7071068, 0.0, -0.7071068);
	const Double3 rightNormal(-0.7071068, 0.0, 0.7071068);
	
	// An intersection occurs if the near point and far point are on different sides 
	// of the diagonal line, or if the near point lies on the diagonal line. No need
	// to normalize the (localPoint - diagMiddle) vector because it's just checking
	// if it's greater than zero.
	const Double2 leftNormal2D(leftNormal.x, leftNormal.z);
	const bool nearOnLeft = leftNormal2D.dot(nearPoint - diagMiddle) >= 0.0;
	const bool farOnLeft = leftNormal2D.dot(farPoint - diagMiddle) >= 0.0;
	const bool intersectionOccurred = (nearOnLeft && !farOnLeft) || (!nearOnLeft && farOnLeft);

	// Only set the output data if an intersection occurred.
	if (intersectionOccurred)
	{
		// Change in X and change in Z of the incoming ray across the voxel.
		const double dx = farPoint.x - nearPoint.x;
		const double dz = farPoint.y - nearPoint.y;

		// The hit coordinate is a 0->1 value representing where the diagonal was hit.
		const double hitCoordinate = [&nearPoint, &diagStart, dx, dz]()
		{
			// Special cases: when the slope is horizontal or vertical. This method treats
			// the X axis as the vertical axis and the Z axis as the horizontal axis.
			const double isHorizontal = std::abs(dx) < Constants::Epsilon;
			const double isVertical = std::abs(dz) < Constants::Epsilon;

			if (isHorizontal)
			{
				// The X axis intercept is the intersection coordinate.
				return nearPoint.x - diagStart.x;
			}
			else if (isVertical)
			{
				// The Z axis intercept is the intersection coordinate.
				return nearPoint.y - diagStart.y;
			}
			else
			{
				// Slope of the diagonal line (trivial, x = z).
				const double diagSlope = 1.0;

				// Vertical axis intercept of the diagonal line.
				const double diagXIntercept = diagStart.x - diagStart.y;

				// Slope of the incoming ray.
				const double raySlope = dx / dz;

				// Get the vertical axis intercept of the incoming ray.
				const double rayXIntercept = nearPoint.x - (raySlope * nearPoint.y);

				// General line intersection calculation.
				return ((rayXIntercept - diagXIntercept) / 
					(diagSlope - raySlope)) - diagStart.y;
			}
		}();

		// Set the hit data.
		hit.u = MathUtils::clamp(hitCoordinate, 0.0, Constants::JustBelowOne);
		hit.point = Double2(
			static_cast<double>(voxelX) + hit.u,
			static_cast<double>(voxelZ) + hit.u);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.normal = nearOnLeft ? leftNormal : rightNormal;

		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool SoftwareRenderer::findDiag2Intersection(int voxelX, int voxelZ, const Double2 &nearPoint,
	const Double2 &farPoint, RayHit &hit)
{
	// Mostly a copy of findDiag1Intersection(), though with a couple different values
	// for the diagonal (end points, slope, etc.).

	// Start, middle, and end points of the diagonal line segment relative to the grid.
	const Double2 diagStart(
		static_cast<double>(voxelX) + Constants::JustBelowOne,
		static_cast<double>(voxelZ));
	const Double2 diagMiddle(
		static_cast<double>(voxelX) + 0.50,
		static_cast<double>(voxelZ) + 0.50);
	const Double2 diagEnd(
		static_cast<double>(voxelX),
		static_cast<double>(voxelZ) + Constants::JustBelowOne);

	// Normals for the left and right faces of the wall, facing up-right and down-left
	// respectively (magic number is sqrt(2) / 2).
	const Double3 leftNormal(0.7071068, 0.0, 0.7071068);
	const Double3 rightNormal(-0.7071068, 0.0, -0.7071068);

	// An intersection occurs if the near point and far point are on different sides 
	// of the diagonal line, or if the near point lies on the diagonal line. No need
	// to normalize the (localPoint - diagMiddle) vector because it's just checking
	// if it's greater than zero.
	const Double2 leftNormal2D(leftNormal.x, leftNormal.z);
	const bool nearOnLeft = leftNormal2D.dot(nearPoint - diagMiddle) >= 0.0;
	const bool farOnLeft = leftNormal2D.dot(farPoint - diagMiddle) >= 0.0;
	const bool intersectionOccurred = (nearOnLeft && !farOnLeft) || (!nearOnLeft && farOnLeft);

	// Only set the output data if an intersection occurred.
	if (intersectionOccurred)
	{
		// Change in X and change in Z of the incoming ray across the voxel.
		const double dx = farPoint.x - nearPoint.x;
		const double dz = farPoint.y - nearPoint.y;

		// The hit coordinate is a 0->1 value representing where the diagonal was hit.
		const double hitCoordinate = [&nearPoint, &diagStart, dx, dz]()
		{
			// Special cases: when the slope is horizontal or vertical. This method treats
			// the X axis as the vertical axis and the Z axis as the horizontal axis.
			const double isHorizontal = std::abs(dx) < Constants::Epsilon;
			const double isVertical = std::abs(dz) < Constants::Epsilon;

			if (isHorizontal)
			{
				// The X axis intercept is the compliment of the intersection coordinate.
				return Constants::JustBelowOne - (nearPoint.x - diagStart.x);
			}
			else if (isVertical)
			{
				// The Z axis intercept is the compliment of the intersection coordinate.
				return Constants::JustBelowOne - (nearPoint.y - diagStart.y);
			}
			else
			{
				// Slope of the diagonal line (trivial, x = -z).
				const double diagSlope = -1.0;

				// Vertical axis intercept of the diagonal line.
				const double diagXIntercept = diagStart.x + diagStart.y;

				// Slope of the incoming ray.
				const double raySlope = dx / dz;

				// Get the vertical axis intercept of the incoming ray.
				const double rayXIntercept = nearPoint.x - (raySlope * nearPoint.y);

				// General line intersection calculation.
				return ((rayXIntercept - diagXIntercept) /
					(diagSlope - raySlope)) - diagStart.y;
			}
		}();

		// Set the hit data.
		hit.u = MathUtils::clamp(hitCoordinate, 0.0, Constants::JustBelowOne);
		hit.point = Double2(
			static_cast<double>(voxelX) + (Constants::JustBelowOne - hit.u),
			static_cast<double>(voxelZ) + hit.u);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.normal = nearOnLeft ? leftNormal : rightNormal;

		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool SoftwareRenderer::findInitialEdgeIntersection(int voxelX, int voxelZ, 
	VoxelData::Facing edgeFacing, bool flipped, const Double2 &nearPoint, const Double2 &farPoint,
	const Camera &camera, const Ray &ray, RayHit &hit)
{
	// Reuse the chasm facing code to find which face is intersected.
	const VoxelData::Facing farFacing = SoftwareRenderer::getInitialChasmFarFacing(
		voxelX, voxelZ, Double2(camera.eye.x, camera.eye.z), ray);

	// If the edge facing and far facing match, there's an intersection.
	if (edgeFacing == farFacing)
	{
		hit.innerZ = (farPoint - nearPoint).length();
		hit.u = [flipped, &farPoint, farFacing]()
		{
			const double uVal = [&farPoint, farFacing]()
			{
				if (farFacing == VoxelData::Facing::PositiveX)
				{
					return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
				}
				else if (farFacing == VoxelData::Facing::NegativeX)
				{
					return farPoint.y - std::floor(farPoint.y);
				}
				else if (farFacing == VoxelData::Facing::PositiveZ)
				{
					return farPoint.x - std::floor(farPoint.x);
				}
				else
				{
					return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
				}
			}();

			// Account for the possibility of the texture being flipped horizontally.
			return MathUtils::clamp(!flipped ? uVal : (Constants::JustBelowOne - uVal),
				0.0, Constants::JustBelowOne);
		}();

		hit.point = farPoint;
		hit.normal = -VoxelData::getNormal(farFacing);
		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool SoftwareRenderer::findEdgeIntersection(int voxelX, int voxelZ, VoxelData::Facing edgeFacing,
	bool flipped, VoxelData::Facing nearFacing, const Double2 &nearPoint, const Double2 &farPoint,
	double nearU, const Camera &camera, const Ray &ray, RayHit &hit)
{
	// If the edge facing and near facing match, the intersection is trivial.
	if (edgeFacing == nearFacing)
	{
		hit.innerZ = 0.0;
		hit.u = !flipped ? nearU : MathUtils::clamp(
			Constants::JustBelowOne - nearU, 0.0, Constants::JustBelowOne);
		hit.point = nearPoint;
		hit.normal = VoxelData::getNormal(nearFacing);
		return true;
	}
	else
	{
		// A search is needed to see whether an intersection occurred. Reuse the chasm
		// facing code to find what the far facing is.
		const VoxelData::Facing farFacing = SoftwareRenderer::getChasmFarFacing(
			voxelX, voxelZ, nearFacing, camera, ray);

		// If the edge facing and far facing match, there's an intersection.
		if (edgeFacing == farFacing)
		{
			hit.innerZ = (farPoint - nearPoint).length();
			hit.u = [flipped, &farPoint, farFacing]()
			{
				const double uVal = [&farPoint, farFacing]()
				{
					if (farFacing == VoxelData::Facing::PositiveX)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else if (farFacing == VoxelData::Facing::NegativeX)
					{
						return farPoint.y - std::floor(farPoint.y);
					}
					else if (farFacing == VoxelData::Facing::PositiveZ)
					{
						return farPoint.x - std::floor(farPoint.x);
					}
					else
					{
						return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
					}
				}();

				// Account for the possibility of the texture being flipped horizontally.
				return MathUtils::clamp(!flipped ? uVal : (Constants::JustBelowOne - uVal),
					0.0, Constants::JustBelowOne);
			}();

			hit.point = farPoint;
			hit.normal = -VoxelData::getNormal(farFacing);
			return true;
		}
		else
		{
			// No intersection.
			return false;
		}
	}
}

bool SoftwareRenderer::findInitialSwingingDoorIntersection(int voxelX, int voxelZ,
	double percentOpen, const Double2 &nearPoint, const Double2 &farPoint, bool xAxis,
	const Camera &camera, const Ray &ray, RayHit &hit)
{
	// Decide which corner the door's hinge will be in, and create the line segment
	// that will be rotated based on percent open.
	Double2 interpStart;
	const Double2 pivot = [voxelX, voxelZ, xAxis, &interpStart]()
	{
		const Int2 corner = [voxelX, voxelZ, xAxis, &interpStart]()
		{
			if (xAxis)
			{
				interpStart = -Double2::UnitX;
				return Int2(voxelX + 1, voxelZ + 1);
			}
			else
			{
				interpStart = -Double2::UnitY;
				return Int2(voxelX, voxelZ + 1);
			}
		}();

		const Double2 cornerReal(
			static_cast<double>(corner.x),
			static_cast<double>(corner.y));

		// Bias the pivot towards the voxel center slightly to avoid Z-fighting with
		// adjacent walls.
		const Double2 voxelCenter(
			static_cast<double>(voxelX) + 0.50,
			static_cast<double>(voxelZ) + 0.50);
		const Double2 bias = (voxelCenter - cornerReal) * Constants::Epsilon;
		return cornerReal + bias;
	}();

	// Use the left perpendicular vector of the door's closed position as the 
	// fully open position.
	const Double2 interpEnd = interpStart.leftPerp();

	// Actual position of the door in its rotation, represented as a vector.
	const Double2 doorVec = interpStart.lerp(interpEnd, 1.0 - percentOpen).normalized();

	// Use back-face culling with swinging doors so it's not obstructing the player's
	// view as much when it's opening.
	const Double2 eye2D(camera.eye.x, camera.eye.z);
	const bool isFrontFace = (eye2D - pivot).normalized().dot(doorVec.leftPerp()) > 0.0;

	if (isFrontFace)
	{
		// Vector cross product in 2D, returns a scalar.
		auto cross = [](const Double2 &a, const Double2 &b)
		{
			return (a.x * b.y) - (b.x * a.y);
		};

		// Solve line segment intersection between the incoming ray and the door.
		const Double2 p1 = pivot;
		const Double2 v1 = doorVec;
		const Double2 p2 = nearPoint;
		const Double2 v2 = farPoint - nearPoint;

		// Percent from p1 to (p1 + v1).
		const double t = cross(p2 - p1, v2) / cross(v1, v2);

		// See if the two line segments intersect.
		if ((t >= 0.0) && (t < 1.0))
		{
			// Hit.
			hit.point = p1 + (v1 * t);
			hit.innerZ = (hit.point - nearPoint).length();
			hit.u = t;
			hit.normal = [&v1]()
			{
				const Double2 norm2D = v1.rightPerp();
				return Double3(norm2D.x, 0.0, norm2D.y);
			}();

			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else
	{
		// Cull back face.
		return false;
	}
}

bool SoftwareRenderer::findInitialDoorIntersection(int voxelX, int voxelZ,
	VoxelData::DoorData::Type doorType, double percentOpen, const Double2 &nearPoint,
	const Double2 &farPoint, const Camera &camera, const Ray &ray,
	const VoxelGrid &voxelGrid, RayHit &hit)
{
	// Determine which axis the door should open/close for (either X or Z).
	const bool xAxis = [voxelX, voxelZ, &voxelGrid]()
	{
		// Check adjacent voxels on the X axis for air.
		auto voxelIsAir = [&voxelGrid](int x, int z)
		{
			const bool insideGrid = (x >= 0) && (x < voxelGrid.getWidth()) &&
				(z >= 0) && (z < voxelGrid.getDepth());

			if (insideGrid)
			{
				const uint16_t voxelID = voxelGrid.getVoxel(x, 1, z);
				const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
				return voxelData.dataType == VoxelDataType::None;
			}
			else
			{
				// Anything past the map edge is considered air.
				return true;
			}
		};

		// If voxels (x - 1, z) and (x + 1, z) are empty, return true.
		return voxelIsAir(voxelX - 1, voxelZ) && voxelIsAir(voxelX + 1, voxelZ);
	}();

	// If the current intersection surface is along one of the voxel's edges, treat the door
	// like a wall by basing intersection calculations on the far facing.
	const bool useFarFacing = [doorType, percentOpen]()
	{
		const bool isClosed = percentOpen == 0.0;
		return isClosed ||
			(doorType == VoxelData::DoorData::Type::Sliding) ||
			(doorType == VoxelData::DoorData::Type::Raising) ||
			(doorType == VoxelData::DoorData::Type::Splitting);
	}();

	if (useFarFacing)
	{
		// Treat the door like a wall. Reuse the chasm facing code to find which face is
		// intersected.
		const VoxelData::Facing farFacing = SoftwareRenderer::getInitialChasmFarFacing(
			voxelX, voxelZ, Double2(camera.eye.x, camera.eye.z), ray);
		const VoxelData::Facing doorFacing = xAxis ?
			VoxelData::Facing::PositiveX : VoxelData::Facing::PositiveZ;

		if (doorFacing == farFacing)
		{
			// The ray intersected the target facing. See if the door itself was intersected
			// and write out hit data based on the door type.
			const double farU = [&farPoint, xAxis]()
			{
				const double uVal = [&farPoint, xAxis]()
				{
					if (xAxis)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else
					{
						return farPoint.x - std::floor(farPoint.x);
					}
				}();

				return MathUtils::clamp(uVal, 0.0, Constants::JustBelowOne);
			}();

			if (doorType == VoxelData::DoorData::Type::Swinging)
			{
				// Treat like a wall.
				hit.innerZ = (farPoint - nearPoint).length();
				hit.u = farU;
				hit.point = farPoint;
				hit.normal = -VoxelData::getNormal(farFacing);
				return true;
			}
			else if (doorType == VoxelData::DoorData::Type::Sliding)
			{
				// If far U coordinate is within percent closed, it's a hit. At 100% open,
				// a sliding door is still partially visible.
				const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
				const double visibleAmount = 1.0 - ((1.0 - minVisible) * percentOpen);
				if (visibleAmount > farU)
				{
					hit.innerZ = (farPoint - nearPoint).length();
					hit.u = MathUtils::clamp(
						farU + (1.0 - visibleAmount), 0.0, Constants::JustBelowOne);
					hit.point = farPoint;
					hit.normal = -VoxelData::getNormal(farFacing);
					return true;
				}
				else
				{
					// No hit.
					return false;
				}
			}
			else if (doorType == VoxelData::DoorData::Type::Raising)
			{
				// Raising doors are always hit.
				hit.innerZ = (farPoint - nearPoint).length();
				hit.u = farU;
				hit.point = farPoint;
				hit.normal = -VoxelData::getNormal(farFacing);
				return true;
			}
			else if (doorType == VoxelData::DoorData::Type::Splitting)
			{
				// If far U coordinate is within percent closed on left or right half, it's a hit.
				// At 100% open, a splitting door is still partially visible.
				const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
				const bool leftHalf = farU < 0.50;
				const bool rightHalf = farU > 0.50;
				double leftVisAmount, rightVisAmount;
				const bool success = [percentOpen, farU, minVisible, leftHalf, rightHalf,
					&leftVisAmount, &rightVisAmount]()
				{
					if (leftHalf)
					{
						// Left half.
						leftVisAmount = 0.50 - ((0.50 - minVisible) * percentOpen);
						return farU <= leftVisAmount;
					}
					else if (rightHalf)
					{
						// Right half.
						rightVisAmount = 0.50 + ((0.50 - minVisible) * percentOpen);
						return farU >= rightVisAmount;
					}
					else
					{
						// Midpoint (only when door is completely closed).
						return percentOpen == 0.0;
					}
				}();

				if (success)
				{
					// Hit.
					hit.innerZ = (farPoint - nearPoint).length();
					hit.u = [farU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
					{
						const double u = [farU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
						{
							if (leftHalf)
							{
								return (farU + 0.50) - leftVisAmount;
							}
							else if (rightHalf)
							{
								return (farU + 0.50) - rightVisAmount;
							}
							else
							{
								// Midpoint.
								return 0.50;
							}
						}();

						return MathUtils::clamp(u, 0.0, Constants::JustBelowOne);
					}();

					hit.point = farPoint;
					hit.normal = -VoxelData::getNormal(farFacing);

					return true;
				}
				else
				{
					// No hit.
					return false;
				}
			}
			else
			{
				// Invalid door type.
				return false;
			}
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (doorType == VoxelData::DoorData::Type::Swinging)
	{
		return SoftwareRenderer::findInitialSwingingDoorIntersection(voxelX, voxelZ, percentOpen,
			nearPoint, farPoint, xAxis, camera, ray, hit);
	}
	else
	{
		// Invalid door type.
		return false;
	}
}

bool SoftwareRenderer::findSwingingDoorIntersection(int voxelX, int voxelZ,
	double percentOpen, VoxelData::Facing nearFacing, const Double2 &nearPoint,
	const Double2 &farPoint, double nearU, RayHit &hit)
{
	// Decide which corner the door's hinge will be in, and create the line segment
	// that will be rotated based on percent open.
	Double2 interpStart;
	const Double2 pivot = [voxelX, voxelZ, nearFacing, &interpStart]()
	{
		const Int2 corner = [voxelX, voxelZ, nearFacing, &interpStart]()
		{
			if (nearFacing == VoxelData::Facing::PositiveX)
			{
				interpStart = -Double2::UnitX;
				return Int2(voxelX + 1, voxelZ + 1);
			}
			else if (nearFacing == VoxelData::Facing::NegativeX)
			{
				interpStart = Double2::UnitX;
				return Int2(voxelX, voxelZ);
			}
			else if (nearFacing == VoxelData::Facing::PositiveZ)
			{
				interpStart = -Double2::UnitY;
				return Int2(voxelX, voxelZ + 1);
			}
			else if (nearFacing == VoxelData::Facing::NegativeZ)
			{
				interpStart = Double2::UnitY;
				return Int2(voxelX + 1, voxelZ);
			}
			else
			{
				throw DebugException("Invalid near facing \"" +
					std::to_string(static_cast<int>(nearFacing)) + "\".");
			}
		}();

		const Double2 cornerReal(
			static_cast<double>(corner.x),
			static_cast<double>(corner.y));

		// Bias the pivot towards the voxel center slightly to avoid Z-fighting with
		// adjacent walls.
		const Double2 voxelCenter(
			static_cast<double>(voxelX) + 0.50,
			static_cast<double>(voxelZ) + 0.50);
		const Double2 bias = (voxelCenter - cornerReal) * Constants::Epsilon;
		return cornerReal + bias;
	}();

	// Use the left perpendicular vector of the door's closed position as the 
	// fully open position.
	const Double2 interpEnd = interpStart.leftPerp();

	// Actual position of the door in its rotation, represented as a vector.
	const Double2 doorVec = interpStart.lerp(interpEnd, 1.0 - percentOpen).normalized();

	// Vector cross product in 2D, returns a scalar.
	auto cross = [](const Double2 &a, const Double2 &b)
	{
		return (a.x * b.y) - (b.x * a.y);
	};

	// Solve line segment intersection between the incoming ray and the door.
	const Double2 p1 = pivot;
	const Double2 v1 = doorVec;
	const Double2 p2 = nearPoint;
	const Double2 v2 = farPoint - nearPoint;

	// Percent from p1 to (p1 + v1).
	const double t = cross(p2 - p1, v2) / cross(v1, v2);

	// See if the two line segments intersect.
	if ((t >= 0.0) && (t < 1.0))
	{
		// Hit.
		hit.point = p1 + (v1 * t);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.u = t;
		hit.normal = [&v1]()
		{
			const Double2 norm2D = v1.rightPerp();
			return Double3(norm2D.x, 0.0, norm2D.y);
		}();

		return true;
	}
	else
	{
		// No hit.
		return false;
	}
}

bool SoftwareRenderer::findDoorIntersection(int voxelX, int voxelZ, 
	VoxelData::DoorData::Type doorType, double percentOpen, VoxelData::Facing nearFacing,
	const Double2 &nearPoint, const Double2 &farPoint, double nearU, RayHit &hit)
{
	// Check trivial case first: whether the door is closed.
	const bool isClosed = percentOpen == 0.0;

	if (isClosed)
	{
		// Treat like a wall.
		hit.innerZ = 0.0;
		hit.u = nearU;
		hit.point = nearPoint;
		hit.normal = VoxelData::getNormal(nearFacing);
		return true;
	}
	else if (doorType == VoxelData::DoorData::Type::Swinging)
	{
		return SoftwareRenderer::findSwingingDoorIntersection(voxelX, voxelZ, percentOpen,
			nearFacing, nearPoint, farPoint, nearU, hit);
	}
	else if (doorType == VoxelData::DoorData::Type::Sliding)
	{
		// If near U coordinate is within percent closed, it's a hit. At 100% open,
		// a sliding door is still partially visible.
		const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
		const double visibleAmount = 1.0 - ((1.0 - minVisible) * percentOpen);
		if (visibleAmount > nearU)
		{
			hit.innerZ = 0.0;
			hit.u = MathUtils::clamp(
				nearU + (1.0 - visibleAmount), 0.0, Constants::JustBelowOne);
			hit.point = nearPoint;
			hit.normal = VoxelData::getNormal(nearFacing);
			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (doorType == VoxelData::DoorData::Type::Raising)
	{
		// Raising doors are always hit.
		hit.innerZ = 0.0;
		hit.u = nearU;
		hit.point = nearPoint;
		hit.normal = VoxelData::getNormal(nearFacing);
		return true;
	}
	else if (doorType == VoxelData::DoorData::Type::Splitting)
	{
		// If near U coordinate is within percent closed on left or right half, it's a hit.
		// At 100% open, a splitting door is still partially visible.
		const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
		const bool leftHalf = nearU < 0.50;
		const bool rightHalf = nearU > 0.50;
		double leftVisAmount, rightVisAmount;
		const bool success = [percentOpen, nearU, minVisible, leftHalf, rightHalf,
			&leftVisAmount, &rightVisAmount]()
		{
			if (leftHalf)
			{
				// Left half.
				leftVisAmount = 0.50 - ((0.50 - minVisible) * percentOpen);
				return nearU <= leftVisAmount;
			}
			else if (rightHalf)
			{
				// Right half.
				rightVisAmount = 0.50 + ((0.50 - minVisible) * percentOpen);
				return nearU >= rightVisAmount;
			}
			else
			{
				// Midpoint (only when door is completely closed).
				return percentOpen == 0.0;
			}
		}();
		
		if (success)
		{
			// Hit.
			hit.innerZ = 0.0;
			hit.u = [nearU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
			{
				const double u = [nearU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
				{
					if (leftHalf)
					{
						return (nearU + 0.50) - leftVisAmount;
					}
					else if (rightHalf)
					{
						return (nearU + 0.50) - rightVisAmount;
					}
					else
					{
						// Midpoint.
						return 0.50;
					}
				}();
				
				return MathUtils::clamp(u, 0.0, Constants::JustBelowOne);
			}();

			hit.point = nearPoint;
			hit.normal = VoxelData::getNormal(nearFacing);

			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else
	{
		// Invalid door type.
		return false;
	}
}

void SoftwareRenderer::drawPixels(int x, const DrawRange &drawRange, double depth, double u,
	double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
	const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(VoxelTexture::WIDTH));

	// Linearly interpolated fog.
	const Double3 &fogColor = shadingInfo.getFogColor();
	const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Shading on the texture.
	// - @todo: contribution from lights.
	const Double3 shading(
		shadingInfo.ambient + sunComponent.x,
		shadingInfo.ambient + sunComponent.y,
		shadingInfo.ambient + sunComponent.z);

	// Clip the Y start and end coordinates as needed, and refresh the occlusion buffer.
	occlusion.clipRange(&yStart, &yEnd);
	occlusion.update(yStart, yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Check depth of the pixel before rendering.
		// - @todo: implement occlusion culling and back-to-front transparent rendering so
		//   this depth check isn't needed.
		if (depth <= (frame.depthBuffer[index] - Constants::Epsilon))
		{
			// Percent stepped from beginning to end on the column.
			const double yPercent = 
				((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

			// Vertical texture coordinate.
			const double v = vStart + ((vEnd - vStart) * yPercent);

			// Y position in texture.
			const int textureY = static_cast<int>(v * static_cast<double>(VoxelTexture::HEIGHT));

			// Alpha is ignored in this loop, so transparent texels will appear black.
			const int textureIndex = textureX + (textureY * VoxelTexture::WIDTH);
			const VoxelTexel &texel = texture.texels[textureIndex];

			// Texture color with shading.
			const double shadingMax = 1.0;
			double colorR = texel.r * std::min(shading.x + texel.emission, shadingMax);
			double colorG = texel.g * std::min(shading.y + texel.emission, shadingMax);
			double colorB = texel.b * std::min(shading.z + texel.emission, shadingMax);

			// Linearly interpolate with fog.
			colorR += (fogColor.x - colorR) * fogPercent;
			colorG += (fogColor.y - colorG) * fogPercent;
			colorB += (fogColor.z - colorB) * fogPercent;

			// Clamp maximum (don't worry about negative values).
			const double high = 1.0;
			colorR = (colorR > high) ? high : colorR;
			colorG = (colorG > high) ? high : colorG;
			colorB = (colorB > high) ? high : colorB;

			// Convert floats to integers.
			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;
			frame.depthBuffer[index] = depth;
		}
	}
}

void SoftwareRenderer::drawPerspectivePixels(int x, const DrawRange &drawRange,
	const Double2 &startPoint, const Double2 &endPoint, double depthStart, double depthEnd,
	const Double3 &normal, const VoxelTexture &texture, const ShadingInfo &shadingInfo,
	OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Fog color to interpolate with.
	const Double3 &fogColor = shadingInfo.getFogColor();

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Shading on the texture.
	// - @todo: contribution from lights.
	const Double3 shading(
		shadingInfo.ambient + sunComponent.x,
		shadingInfo.ambient + sunComponent.y,
		shadingInfo.ambient + sunComponent.z);

	// Values for perspective-correct interpolation.
	const double depthStartRecip = 1.0 / depthStart;
	const double depthEndRecip = 1.0 / depthEnd;
	const Double2 startPointDiv = startPoint * depthStartRecip;
	const Double2 endPointDiv = endPoint * depthEndRecip;
	const Double2 pointDivDiff = endPointDiv - startPointDiv;
	
	// Clip the Y start and end coordinates as needed, and refresh the occlusion buffer.
	occlusion.clipRange(&yStart, &yEnd);
	occlusion.update(yStart, yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Percent stepped from beginning to end on the column.
		const double yPercent = 
			((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

		// Interpolate between the near and far depth.
		const double depth = 1.0 / 
			(depthStartRecip + ((depthEndRecip - depthStartRecip) * yPercent));

		// Check depth of the pixel before rendering.
		// - @todo: implement occlusion culling and back-to-front transparent rendering so
		//   this depth check isn't needed.
		if (depth <= frame.depthBuffer[index])
		{
			// Linearly interpolated fog.
			const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

			// Interpolate between start and end points.
			const double currentPointX = (startPointDiv.x + (pointDivDiff.x * yPercent)) * depth;
			const double currentPointY = (startPointDiv.y + (pointDivDiff.y * yPercent)) * depth;

			// Texture coordinates.
			const double u = MathUtils::clamp(
				Constants::JustBelowOne - (currentPointX - std::floor(currentPointX)),
				0.0, Constants::JustBelowOne);
			const double v = MathUtils::clamp(
				Constants::JustBelowOne - (currentPointY - std::floor(currentPointY)),
				0.0, Constants::JustBelowOne);

			// Offsets in texture.
			const int textureX = static_cast<int>(u * static_cast<double>(VoxelTexture::WIDTH));
			const int textureY = static_cast<int>(v * static_cast<double>(VoxelTexture::HEIGHT));

			// Alpha is ignored in this loop, so transparent texels will appear black.
			const int textureIndex = textureX + (textureY * VoxelTexture::WIDTH);
			const VoxelTexel &texel = texture.texels[textureIndex];

			// Texture color with shading.
			const double shadingMax = 1.0;
			double colorR = texel.r * std::min(shading.x + texel.emission, shadingMax);
			double colorG = texel.g * std::min(shading.y + texel.emission, shadingMax);
			double colorB = texel.b * std::min(shading.z + texel.emission, shadingMax);

			// Linearly interpolate with fog.
			colorR += (fogColor.x - colorR) * fogPercent;
			colorG += (fogColor.y - colorG) * fogPercent;
			colorB += (fogColor.z - colorB) * fogPercent;

			// Clamp maximum (don't worry about negative values).
			const double high = 1.0;
			colorR = (colorR > high) ? high : colorR;
			colorG = (colorG > high) ? high : colorG;
			colorB = (colorB > high) ? high : colorB;

			// Convert floats to integers.
			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;
			frame.depthBuffer[index] = depth;
		}
	}
}

void SoftwareRenderer::drawTransparentPixels(int x, const DrawRange &drawRange, double depth,
	double u, double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
	const ShadingInfo &shadingInfo, const OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(VoxelTexture::WIDTH));

	// Linearly interpolated fog.
	const Double3 &fogColor = shadingInfo.getFogColor();
	const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Shading on the texture.
	// - @todo: contribution from lights.
	const Double3 shading(
		shadingInfo.ambient + sunComponent.x,
		shadingInfo.ambient + sunComponent.y,
		shadingInfo.ambient + sunComponent.z);

	// Clip the Y start and end coordinates as needed, but do not refresh the occlusion buffer,
	// because transparent ranges do not occlude as simply as opaque ranges.
	occlusion.clipRange(&yStart, &yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Check depth of the pixel before rendering.
		if (depth <= (frame.depthBuffer[index] - Constants::Epsilon))
		{
			// Percent stepped from beginning to end on the column.
			const double yPercent =
				((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

			// Vertical texture coordinate.
			const double v = vStart + ((vEnd - vStart) * yPercent);

			// Y position in texture.
			const int textureY = static_cast<int>(v * static_cast<double>(VoxelTexture::HEIGHT));

			// Alpha is checked in this loop, and transparent texels are not drawn.
			const int textureIndex = textureX + (textureY * VoxelTexture::WIDTH);
			const VoxelTexel &texel = texture.texels[textureIndex];
			
			if (!texel.transparent)
			{
				// Texture color with shading.
				const double shadingMax = 1.0;
				double colorR = texel.r * std::min(shading.x + texel.emission, shadingMax);
				double colorG = texel.g * std::min(shading.y + texel.emission, shadingMax);
				double colorB = texel.b * std::min(shading.z + texel.emission, shadingMax);

				// Linearly interpolate with fog.
				colorR += (fogColor.x - colorR) * fogPercent;
				colorG += (fogColor.y - colorG) * fogPercent;
				colorB += (fogColor.z - colorB) * fogPercent;
				
				// Clamp maximum (don't worry about negative values).
				const double high = 1.0;
				colorR = (colorR > high) ? high : colorR;
				colorG = (colorG > high) ? high : colorG;
				colorB = (colorB > high) ? high : colorB;

				// Convert floats to integers.
				const uint32_t colorRGB = static_cast<uint32_t>(
					((static_cast<uint8_t>(colorR * 255.0)) << 16) |
					((static_cast<uint8_t>(colorG * 255.0)) << 8) |
					((static_cast<uint8_t>(colorB * 255.0))));

				frame.colorBuffer[index] = colorRGB;
				frame.depthBuffer[index] = depth;
			}
		}
	}
}

void SoftwareRenderer::drawDistantPixels(int x, const DrawRange &drawRange, double u,
	double vStart, double vEnd, const SkyTexture &texture, bool emissive,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	const int yStart = drawRange.yStart;
	const int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(texture.width));
	
	// Shading on the texture. Some distant objects are completely bright.
	const double shading = emissive ? 1.0 : shadingInfo.distantAmbient;

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Percent stepped from beginning to end on the column.
		const double yPercent =
			((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

		// Vertical texture coordinate.
		const double v = vStart + ((vEnd - vStart) * yPercent);

		// Y position in texture.
		const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

		// Alpha is checked in this loop, and transparent texels are not drawn.
		const int textureIndex = textureX + (textureY * texture.width);
		const SkyTexel &texel = texture.texels[textureIndex];

		if (!texel.transparent)
		{
			// Texture color with shading.
			double colorR = texel.r * shading;
			double colorG = texel.g * shading;
			double colorB = texel.b * shading;

			// @todo: determine if distant objects should be affected by fog using some
			// arbitrary range in the new engine, just for aesthetic purposes.

			// Clamp maximum (don't worry about negative values).
			const double high = 1.0;
			colorR = (colorR > high) ? high : colorR;
			colorG = (colorG > high) ? high : colorG;
			colorB = (colorB > high) ? high : colorB;

			// Convert floats to integers.
			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;
		}
	}
}

void SoftwareRenderer::drawInitialVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
	const Ray &ray, VoxelData::Facing facing, const Double2 &nearPoint, const Double2 &farPoint,
	double nearZ, double farZ, const ShadingInfo &shadingInfo, double ceilingHeight,
	const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
	const std::vector<VoxelTexture> &textures, OcclusionData &occlusion, const FrameView &frame)
{
	// This method handles some special cases such as drawing the back-faces of wall sides.

	// When clamping Y values for drawing ranges, subtract 0.5 from starts and add 0.5 to 
	// ends before converting to integers because the drawing methods sample at the center 
	// of pixels. The clamping function depends on which side of the range is being clamped; 
	// either way, the drawing range should be contained within the projected range at the 
	// sub-pixel level. This ensures that the vertical texture coordinate is always within 0->1.

	const double wallU = [&farPoint, facing]()
	{
		const double uVal = [&farPoint, facing]()
		{
			if (facing == VoxelData::Facing::PositiveX)
			{
				return farPoint.y - std::floor(farPoint.y);
			}
			else if (facing == VoxelData::Facing::NegativeX)
			{
				return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
			}
			else if (facing == VoxelData::Facing::PositiveZ)
			{
				return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
			}
			else
			{
				return farPoint.x - std::floor(farPoint.x);
			}
		}();

		return MathUtils::clamp(uVal, 0.0, Constants::JustBelowOne);
	}();

	// Normal of the wall for the incoming ray, potentially shared between multiple voxels in
	// this voxel column.
	const Double3 wallNormal = -VoxelData::getNormal(facing);

	auto drawInitialVoxel = [x, voxelX, voxelZ, &camera, &ray, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &openDoors, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelHeight = ceilingHeight;
		const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

		if (voxelData.dataType == VoxelDataType::Wall)
		{
			// Draw inner ceiling, wall, and floor.
			const VoxelData::WallData &wallData = voxelData.wall;

			const Double3 farCeilingPoint(
				farPoint.x,
				voxelYReal + voxelHeight,
				farPoint.y);
			const Double3 nearCeilingPoint(
				nearPoint.x,
				farCeilingPoint.y,
				nearPoint.y);
			const Double3 farFloorPoint(
				farPoint.x,
				voxelYReal,
				farPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				farFloorPoint.y,
				nearPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
				nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint, camera, frame);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.at(wallData.ceilingID), shadingInfo,
				occlusion, frame);

			// Wall.
			SoftwareRenderer::drawPixels(x, drawRanges.at(1), farZ, wallU, 0.0,
				Constants::JustBelowOne, wallNormal, textures.at(wallData.sideID), shadingInfo,
				occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
				farZ, nearZ, Double3::UnitY, textures.at(wallData.floorID), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Floor)
		{
			// Do nothing. Floors can only be seen from above.
		}
		else if (voxelData.dataType == VoxelDataType::Ceiling)
		{
			// Draw bottom of ceiling voxel if the camera is below it.
			if (camera.eye.y < voxelYReal)
			{
				const VoxelData::CeilingData &ceilingData = voxelData.ceiling;

				const Double3 nearFloorPoint(
					nearPoint.x,
					voxelYReal,
					nearPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearFloorPoint, farFloorPoint, camera, frame);

				SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint,
					nearZ, farZ, -Double3::UnitY, textures.at(ceilingData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Raised)
		{
			const VoxelData::RaisedData &raisedData = voxelData.raised;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal + (raisedData.yOffset * voxelHeight),
				nearPoint.y);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					farCeilingPoint, nearCeilingPoint, camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearFloorPoint, farFloorPoint, camera, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
					nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint,
					camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
					nearZ, farZ, -Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), farZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
					farZ, nearZ, Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Diagonal)
		{
			const VoxelData::DiagonalData &diagData = voxelData.diagonal;

			// Find intersection.
			RayHit hit;
			const bool success = diagData.type1 ?
				SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
				SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				const Double3 diagTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const Double3 diagBottomPoint(
					diagTopPoint.x,
					voxelYReal,
					diagTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					diagTopPoint, diagBottomPoint, camera, frame);

				SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.at(diagData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::TransparentWall)
		{
			// Do nothing. Transparent walls have no back-faces.
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findInitialEdgeIntersection(
				voxelX, voxelZ, edgeData.facing, edgeData.flipped, nearPoint, farPoint,
				camera, ray, hit);

			if (success)
			{
				const Double3 edgeTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight + edgeData.yOffset,
					hit.point.y);
				const Double3 edgeBottomPoint(
					edgeTopPoint.x,
					voxelYReal + edgeData.yOffset,
					edgeTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					edgeTopPoint, edgeBottomPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
					0.0, Constants::JustBelowOne, hit.normal, textures.at(edgeData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Render back-face.
			const VoxelData::ChasmData &chasmData = voxelData.chasm;

			// Find which far face on the chasm was intersected.
			const VoxelData::Facing farFacing = SoftwareRenderer::getInitialChasmFarFacing(
				voxelX, voxelZ, Double2(camera.eye.x, camera.eye.z), ray);

			// Far.
			if (chasmData.faceIsVisible(farFacing))
			{
				const double farU = [&farPoint, farFacing]()
				{
					const double uVal = [&farPoint, farFacing]()
					{
						if (farFacing == VoxelData::Facing::PositiveX)
						{
							return farPoint.y - std::floor(farPoint.y);
						}
						else if (farFacing == VoxelData::Facing::NegativeX)
						{
							return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
						}
						else if (farFacing == VoxelData::Facing::PositiveZ)
						{
							return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
						}
						else
						{
							return farPoint.x - std::floor(farPoint.x);
						}
					}();

					return MathUtils::clamp(uVal, 0.0, Constants::JustBelowOne);
				}();

				const Double3 farNormal = -VoxelData::getNormal(farFacing);

				// Wet chasms and lava chasms are unaffected by ceiling height.
				const double chasmDepth = (chasmData.type == VoxelData::ChasmData::Type::Dry) ?
					voxelHeight : VoxelData::ChasmData::WET_LAVA_DEPTH;

				const Double3 farCeilingPoint(
					farPoint.x,
					voxelYReal + voxelHeight,
					farPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					farCeilingPoint.y - chasmDepth,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					farCeilingPoint, farFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, farZ, farU, 0.0,
					Constants::JustBelowOne, farNormal, textures.at(chasmData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			const VoxelData::DoorData &doorData = voxelData.door;
			const double percentOpen = SoftwareRenderer::getDoorPercentOpen(
				voxelX, voxelZ, openDoors);

			RayHit hit;
			const bool success = SoftwareRenderer::findInitialDoorIntersection(voxelX, voxelZ,
				doorData.type, percentOpen, nearPoint, farPoint, camera, ray, voxelGrid, hit);

			if (success)
			{
				if (doorData.type == VoxelData::DoorData::Type::Swinging)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Sliding)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Raising)
				{
					// Top point is fixed, bottom point depends on percent open.
					const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
					const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal + raisedAmount,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					// The start of the vertical texture coordinate depends on the percent open.
					const double vStart = raisedAmount / voxelHeight;

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, vStart, Constants::JustBelowOne, hit.normal,
						textures.at(doorData.id), shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Splitting)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
			}
		}
	};

	auto drawInitialVoxelBelow = [x, voxelX, voxelZ, &camera, &ray, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &openDoors, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelHeight = ceilingHeight;
		const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

		if (voxelData.dataType == VoxelDataType::Wall)
		{
			const VoxelData::WallData &wallData = voxelData.wall;

			const Double3 farCeilingPoint(
				farPoint.x,
				voxelYReal + voxelHeight,
				farPoint.y);
			const Double3 nearCeilingPoint(
				nearPoint.x,
				farCeilingPoint.y,
				nearPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				farCeilingPoint, nearCeilingPoint, camera, frame);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.at(wallData.ceilingID), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Floor)
		{
			// Draw top of floor voxel.
			const VoxelData::FloorData &floorData = voxelData.floor;

			const Double3 farCeilingPoint(
				farPoint.x,
				voxelYReal + voxelHeight,
				farPoint.y);
			const Double3 nearCeilingPoint(
				nearPoint.x,
				farCeilingPoint.y,
				nearPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				farCeilingPoint, nearCeilingPoint, camera, frame);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.at(floorData.id), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Ceiling)
		{
			// Do nothing. Ceilings can only be seen from below.
		}
		else if (voxelData.dataType == VoxelDataType::Raised)
		{
			const VoxelData::RaisedData &raisedData = voxelData.raised;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal + (raisedData.yOffset * voxelHeight),
				nearPoint.y);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					farCeilingPoint, nearCeilingPoint, camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearFloorPoint, farFloorPoint, camera, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
					nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint,
					camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
					nearZ, farZ, -Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), farZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
					farZ, nearZ, Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Diagonal)
		{
			const VoxelData::DiagonalData &diagData = voxelData.diagonal;

			// Find intersection.
			RayHit hit;
			const bool success = diagData.type1 ?
				SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
				SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				const Double3 diagTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const Double3 diagBottomPoint(
					diagTopPoint.x,
					voxelYReal,
					diagTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					diagTopPoint, diagBottomPoint, camera, frame);

				SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.at(diagData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::TransparentWall)
		{
			// Do nothing. Transparent walls have no back-faces.
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findInitialEdgeIntersection(
				voxelX, voxelZ, edgeData.facing, edgeData.flipped, nearPoint, farPoint,
				camera, ray, hit);

			if (success)
			{
				const Double3 edgeTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight + edgeData.yOffset,
					hit.point.y);
				const Double3 edgeBottomPoint(
					hit.point.x,
					voxelYReal + edgeData.yOffset,
					hit.point.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					edgeTopPoint, edgeBottomPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
					0.0, Constants::JustBelowOne, hit.normal, textures.at(edgeData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Render back-face.
			const VoxelData::ChasmData &chasmData = voxelData.chasm;

			// Find which far face on the chasm was intersected.
			const VoxelData::Facing farFacing = SoftwareRenderer::getInitialChasmFarFacing(
				voxelX, voxelZ, Double2(camera.eye.x, camera.eye.z), ray);

			// Far.
			if (chasmData.faceIsVisible(farFacing))
			{
				const double farU = [&farPoint, farFacing]()
				{
					const double uVal = [&farPoint, farFacing]()
					{
						if (farFacing == VoxelData::Facing::PositiveX)
						{
							return farPoint.y - std::floor(farPoint.y);
						}
						else if (farFacing == VoxelData::Facing::NegativeX)
						{
							return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
						}
						else if (farFacing == VoxelData::Facing::PositiveZ)
						{
							return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
						}
						else
						{
							return farPoint.x - std::floor(farPoint.x);
						}
					}();

					return MathUtils::clamp(uVal, 0.0, Constants::JustBelowOne);
				}();

				const Double3 farNormal = -VoxelData::getNormal(farFacing);

				// Wet chasms and lava chasms are unaffected by ceiling height.
				const double chasmDepth = (chasmData.type == VoxelData::ChasmData::Type::Dry) ?
					voxelHeight : VoxelData::ChasmData::WET_LAVA_DEPTH;

				const Double3 farCeilingPoint(
					farPoint.x,
					voxelYReal + voxelHeight,
					farPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					farCeilingPoint.y - chasmDepth,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					farCeilingPoint, farFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, farZ, farU, 0.0,
					Constants::JustBelowOne, farNormal, textures.at(chasmData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			const VoxelData::DoorData &doorData = voxelData.door;
			const double percentOpen = SoftwareRenderer::getDoorPercentOpen(
				voxelX, voxelZ, openDoors);

			RayHit hit;
			const bool success = SoftwareRenderer::findInitialDoorIntersection(voxelX, voxelZ,
				doorData.type, percentOpen, nearPoint, farPoint, camera, ray, voxelGrid, hit);

			if (success)
			{
				if (doorData.type == VoxelData::DoorData::Type::Swinging)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Sliding)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Raising)
				{
					// Top point is fixed, bottom point depends on percent open.
					const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
					const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal + raisedAmount,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					// The start of the vertical texture coordinate depends on the percent open.
					const double vStart = raisedAmount / voxelHeight;

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, vStart, Constants::JustBelowOne, hit.normal,
						textures.at(doorData.id), shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Splitting)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
			}
		}
	};

	auto drawInitialVoxelAbove = [x, voxelX, voxelZ, &camera, &ray, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &openDoors, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelHeight = ceilingHeight;
		const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

		if (voxelData.dataType == VoxelDataType::Wall)
		{
			const VoxelData::WallData &wallData = voxelData.wall;

			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);
			const Double3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.at(wallData.floorID), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Floor)
		{
			// Do nothing. Floors can only be seen from above.
		}
		else if (voxelData.dataType == VoxelDataType::Ceiling)
		{
			// Draw bottom of ceiling voxel.
			const VoxelData::CeilingData &ceilingData = voxelData.ceiling;

			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);
			const Double3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);

			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.at(ceilingData.id), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Raised)
		{
			const VoxelData::RaisedData &raisedData = voxelData.raised;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal + (raisedData.yOffset * voxelHeight),
				nearPoint.y);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					farCeilingPoint, nearCeilingPoint, camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearFloorPoint, farFloorPoint, camera, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
					nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint,
					camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
					nearZ, farZ, -Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), farZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
					farZ, nearZ, Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Diagonal)
		{
			const VoxelData::DiagonalData &diagData = voxelData.diagonal;

			// Find intersection.
			RayHit hit;
			const bool success = diagData.type1 ?
				SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
				SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				const Double3 diagTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const Double3 diagBottomPoint(
					diagTopPoint.x,
					voxelYReal,
					diagTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					diagTopPoint, diagBottomPoint, camera, frame);

				SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.at(diagData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::TransparentWall)
		{
			// Do nothing. Transparent walls have no back-faces.
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findInitialEdgeIntersection(
				voxelX, voxelZ, edgeData.facing, edgeData.flipped, nearPoint, farPoint,
				camera, ray, hit);

			if (success)
			{
				const Double3 edgeTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight + edgeData.yOffset,
					hit.point.y);
				const Double3 edgeBottomPoint(
					hit.point.x,
					voxelYReal + edgeData.yOffset,
					hit.point.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					edgeTopPoint, edgeBottomPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
					0.0, Constants::JustBelowOne, hit.normal, textures.at(edgeData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Ignore. Chasms should never be above the player's voxel.
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			const VoxelData::DoorData &doorData = voxelData.door;
			const double percentOpen = SoftwareRenderer::getDoorPercentOpen(
				voxelX, voxelZ, openDoors);

			RayHit hit;
			const bool success = SoftwareRenderer::findInitialDoorIntersection(voxelX, voxelZ,
				doorData.type, percentOpen, nearPoint, farPoint, camera, ray, voxelGrid, hit);

			if (success)
			{
				if (doorData.type == VoxelData::DoorData::Type::Swinging)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Sliding)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Raising)
				{
					// Top point is fixed, bottom point depends on percent open.
					const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
					const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal + raisedAmount,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					// The start of the vertical texture coordinate depends on the percent open.
					const double vStart = raisedAmount / voxelHeight;

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, vStart, Constants::JustBelowOne, hit.normal,
						textures.at(doorData.id), shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Splitting)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
			}
		}
	};

	// Relative Y voxel coordinate of the camera, compensating for the ceiling height.
	const int adjustedVoxelY = camera.getAdjustedEyeVoxelY(ceilingHeight);

	// Draw the player's current voxel first.
	drawInitialVoxel(adjustedVoxelY);

	// Draw voxels below the player's voxel.
	for (int voxelY = (adjustedVoxelY - 1); voxelY >= 0; voxelY--)
	{
		drawInitialVoxelBelow(voxelY);
	}

	// Draw voxels above the player's voxel.
	for (int voxelY = (adjustedVoxelY + 1); voxelY < voxelGrid.getHeight(); voxelY++)
	{
		drawInitialVoxelAbove(voxelY);
	}
}

void SoftwareRenderer::drawVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
	const Ray &ray, VoxelData::Facing facing, const Double2 &nearPoint, const Double2 &farPoint,
	double nearZ, double farZ, const ShadingInfo &shadingInfo, double ceilingHeight,
	const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
	const std::vector<VoxelTexture> &textures, OcclusionData &occlusion, const FrameView &frame)
{
	// Much of the code here is duplicated from the initial voxel column drawing method, but
	// there are a couple differences, like the horizontal texture coordinate being flipped,
	// and the drawing orders being slightly modified. The reason for having so much code is
	// so we cover all the different ray casting cases efficiently. It would slow down this 
	// method if it had to worry about an "initialColumn" boolean that's always false in the
	// general case.

	// When clamping Y values for drawing ranges, subtract 0.5 from starts and add 0.5 to 
	// ends before converting to integers because the drawing methods sample at the center 
	// of pixels. The clamping function depends on which side of the range is being clamped; 
	// either way, the drawing range should be contained within the projected range at the 
	// sub-pixel level. This ensures that the vertical texture coordinate is always within 0->1.

	// Horizontal texture coordinate for the wall, potentially shared between multiple voxels
	// in this voxel column.
	const double wallU = [&nearPoint, facing]()
	{
		const double uVal = [&nearPoint, facing]()
		{
			if (facing == VoxelData::Facing::PositiveX)
			{
				return Constants::JustBelowOne - (nearPoint.y - std::floor(nearPoint.y));
			}
			else if (facing == VoxelData::Facing::NegativeX)
			{
				return nearPoint.y - std::floor(nearPoint.y);
			}
			else if (facing == VoxelData::Facing::PositiveZ)
			{
				return nearPoint.x - std::floor(nearPoint.x);
			}
			else
			{
				return Constants::JustBelowOne - (nearPoint.x - std::floor(nearPoint.x));
			}
		}();

		return MathUtils::clamp(uVal, 0.0, Constants::JustBelowOne);
	}();

	// Normal of the wall for the incoming ray, potentially shared between multiple voxels in
	// this voxel column.
	const Double3 wallNormal = VoxelData::getNormal(facing);

	auto drawVoxel = [x, voxelX, voxelZ, &camera, &ray, facing, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &openDoors, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelHeight = ceilingHeight;
		const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

		if (voxelData.dataType == VoxelDataType::Wall)
		{
			// Draw side.
			const VoxelData::WallData &wallData = voxelData.wall;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);

			SoftwareRenderer::drawPixels(x, drawRange, nearZ, wallU, 0.0, Constants::JustBelowOne,
				wallNormal, textures.at(wallData.sideID), shadingInfo, occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Floor)
		{
			// Do nothing. Floors can only be seen from above.
		}
		else if (voxelData.dataType == VoxelDataType::Ceiling)
		{
			// Draw bottom of ceiling voxel if the camera is below it.
			if (camera.eye.y < voxelYReal)
			{
				const VoxelData::CeilingData &ceilingData = voxelData.ceiling;

				const Double3 nearFloorPoint(
					nearPoint.x,
					voxelYReal,
					nearPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearFloorPoint, farFloorPoint, camera, frame);

				SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(ceilingData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Raised)
		{
			const VoxelData::RaisedData &raisedData = voxelData.raised;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal + (raisedData.yOffset * voxelHeight),
				nearPoint.y);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
					farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint,
					farZ, nearZ, Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
					nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(0), nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
					nearZ, farZ, -Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearCeilingPoint, nearFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Diagonal)
		{
			const VoxelData::DiagonalData &diagData = voxelData.diagonal;

			// Find intersection.
			RayHit hit;
			const bool success = diagData.type1 ?
				SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
				SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				const Double3 diagTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const Double3 diagBottomPoint(
					diagTopPoint.x,
					voxelYReal,
					diagTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					diagTopPoint, diagBottomPoint, camera, frame);

				SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.at(diagData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::TransparentWall)
		{
			// Draw transparent side.
			const VoxelData::TransparentWallData &transparentWallData = voxelData.transparentWall;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU, 0.0,
				Constants::JustBelowOne, wallNormal, textures.at(transparentWallData.id),
				shadingInfo, occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
				edgeData.facing, edgeData.flipped, facing, nearPoint, farPoint, wallU,
				camera, ray, hit);

			if (success)
			{
				const Double3 edgeTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight + edgeData.yOffset,
					hit.point.y);
				const Double3 edgeBottomPoint(
					hit.point.x,
					voxelYReal + edgeData.yOffset,
					hit.point.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					edgeTopPoint, edgeBottomPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
					0.0, Constants::JustBelowOne, hit.normal, textures.at(edgeData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Render front and back-faces.
			const VoxelData::ChasmData &chasmData = voxelData.chasm;

			// Find which faces on the chasm were intersected.
			const VoxelData::Facing nearFacing = facing;
			const VoxelData::Facing farFacing = SoftwareRenderer::getChasmFarFacing(
				voxelX, voxelZ, nearFacing, camera, ray);

			// Near.
			if (chasmData.faceIsVisible(nearFacing))
			{
				const double nearU = Constants::JustBelowOne - wallU;
				const Double3 nearNormal = wallNormal;
				
				// Wet chasms and lava chasms are unaffected by ceiling height.
				const double chasmDepth = (chasmData.type == VoxelData::ChasmData::Type::Dry) ?
					voxelHeight : VoxelData::ChasmData::WET_LAVA_DEPTH;

				const Double3 nearCeilingPoint(
					nearPoint.x,
					voxelYReal + voxelHeight,
					nearPoint.y);
				const Double3 nearFloorPoint(
					nearPoint.x,
					nearCeilingPoint.y - chasmDepth,
					nearPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearCeilingPoint, nearFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, nearU, 0.0,
					Constants::JustBelowOne, nearNormal, textures.at(chasmData.id),
					shadingInfo, occlusion, frame);
			}

			// Far.
			if (chasmData.faceIsVisible(farFacing))
			{
				const double farU = [&farPoint, farFacing]()
				{
					const double uVal = [&farPoint, farFacing]()
					{
						if (farFacing == VoxelData::Facing::PositiveX)
						{
							return farPoint.y - std::floor(farPoint.y);
						}
						else if (farFacing == VoxelData::Facing::NegativeX)
						{
							return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
						}
						else if (farFacing == VoxelData::Facing::PositiveZ)
						{
							return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
						}
						else
						{
							return farPoint.x - std::floor(farPoint.x);
						}
					}();

					return MathUtils::clamp(uVal, 0.0, Constants::JustBelowOne);
				}();

				const Double3 farNormal = -VoxelData::getNormal(farFacing);

				// Wet chasms and lava chasms are unaffected by ceiling height.
				const double chasmDepth = (chasmData.type == VoxelData::ChasmData::Type::Dry) ?
					voxelHeight : VoxelData::ChasmData::WET_LAVA_DEPTH;

				const Double3 farCeilingPoint(
					farPoint.x,
					voxelYReal + voxelHeight,
					farPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					farCeilingPoint.y - chasmDepth,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					farCeilingPoint, farFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, farZ, farU, 0.0,
					Constants::JustBelowOne, farNormal, textures.at(chasmData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			const VoxelData::DoorData &doorData = voxelData.door;
			const double percentOpen = SoftwareRenderer::getDoorPercentOpen(
				voxelX, voxelZ, openDoors);

			RayHit hit;
			const bool success = SoftwareRenderer::findDoorIntersection(voxelX, voxelZ,
				doorData.type, percentOpen, facing, nearPoint, farPoint, wallU, hit);

			if (success)
			{
				if (doorData.type == VoxelData::DoorData::Type::Swinging)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Sliding)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Raising)
				{
					// Top point is fixed, bottom point depends on percent open.
					const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
					const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal + raisedAmount,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					// The start of the vertical texture coordinate depends on the percent open.
					const double vStart = raisedAmount / voxelHeight;

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, vStart,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id), shadingInfo,
						occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Splitting)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
			}
		}
	};

	auto drawVoxelBelow = [x, voxelX, voxelZ, &camera, &ray, facing, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &openDoors, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelHeight = ceilingHeight;
		const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

		if (voxelData.dataType == VoxelDataType::Wall)
		{
			const VoxelData::WallData &wallData = voxelData.wall;

			const Double3 farCeilingPoint(
				farPoint.x,
				voxelYReal + voxelHeight,
				farPoint.y);
			const Double3 nearCeilingPoint(
				nearPoint.x,
				farCeilingPoint.y,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.at(wallData.ceilingID), shadingInfo,
				occlusion, frame);

			// Wall.
			SoftwareRenderer::drawPixels(x, drawRanges.at(1), nearZ, wallU, 0.0,
				Constants::JustBelowOne, wallNormal, textures.at(wallData.sideID), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Floor)
		{
			// Draw top of floor voxel.
			const VoxelData::FloorData &floorData = voxelData.floor;

			const Double3 farCeilingPoint(
				farPoint.x,
				voxelYReal + voxelHeight,
				farPoint.y);
			const Double3 nearCeilingPoint(
				nearPoint.x,
				farCeilingPoint.y,
				nearPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				farCeilingPoint, nearCeilingPoint, camera, frame);

			SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.at(floorData.id), shadingInfo, 
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Ceiling)
		{
			// Do nothing. Ceilings can only be seen from below.
		}
		else if (voxelData.dataType == VoxelDataType::Raised)
		{
			const VoxelData::RaisedData &raisedData = voxelData.raised;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal + (raisedData.yOffset * voxelHeight),
				nearPoint.y);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
					farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint,
					farZ, nearZ, Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
					nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(0), nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
					nearZ, farZ, -Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearCeilingPoint, nearFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Diagonal)
		{
			const VoxelData::DiagonalData &diagData = voxelData.diagonal;

			// Find intersection.
			RayHit hit;
			const bool success = diagData.type1 ?
				SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
				SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				const Double3 diagTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const Double3 diagBottomPoint(
					diagTopPoint.x,
					voxelYReal,
					diagTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					diagTopPoint, diagBottomPoint, camera, frame);

				SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.at(diagData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::TransparentWall)
		{
			// Draw transparent side.
			const VoxelData::TransparentWallData &transparentWallData = voxelData.transparentWall;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU, 0.0,
				Constants::JustBelowOne, wallNormal, textures.at(transparentWallData.id),
				shadingInfo, occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
				edgeData.facing, edgeData.flipped, facing, nearPoint, farPoint, wallU,
				camera, ray, hit);

			if (success)
			{
				const Double3 edgeTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight + edgeData.yOffset,
					hit.point.y);
				const Double3 edgeBottomPoint(
					hit.point.x,
					voxelYReal + edgeData.yOffset,
					hit.point.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					edgeTopPoint, edgeBottomPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
					0.0, Constants::JustBelowOne, hit.normal, textures.at(edgeData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Render front and back-faces.
			const VoxelData::ChasmData &chasmData = voxelData.chasm;

			// Find which faces on the chasm were intersected.
			const VoxelData::Facing nearFacing = facing;
			const VoxelData::Facing farFacing = SoftwareRenderer::getChasmFarFacing(
				voxelX, voxelZ, nearFacing, camera, ray);

			// Near.
			if (chasmData.faceIsVisible(nearFacing))
			{
				const double nearU = Constants::JustBelowOne - wallU;
				const Double3 nearNormal = wallNormal;

				// Wet chasms and lava chasms are unaffected by ceiling height.
				const double chasmDepth = (chasmData.type == VoxelData::ChasmData::Type::Dry) ?
					voxelHeight : VoxelData::ChasmData::WET_LAVA_DEPTH;

				const Double3 nearCeilingPoint(
					nearPoint.x,
					voxelYReal + voxelHeight,
					nearPoint.y);
				const Double3 nearFloorPoint(
					nearPoint.x,
					nearCeilingPoint.y - chasmDepth,
					nearPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearCeilingPoint, nearFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, nearU, 0.0,
					Constants::JustBelowOne, nearNormal, textures.at(chasmData.id),
					shadingInfo, occlusion, frame);
			}

			// Far.
			if (chasmData.faceIsVisible(farFacing))
			{
				const double farU = [&farPoint, farFacing]()
				{
					const double uVal = [&farPoint, farFacing]()
					{
						if (farFacing == VoxelData::Facing::PositiveX)
						{
							return farPoint.y - std::floor(farPoint.y);
						}
						else if (farFacing == VoxelData::Facing::NegativeX)
						{
							return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
						}
						else if (farFacing == VoxelData::Facing::PositiveZ)
						{
							return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
						}
						else
						{
							return farPoint.x - std::floor(farPoint.x);
						}
					}();

					return MathUtils::clamp(uVal, 0.0, Constants::JustBelowOne);
				}();

				const Double3 farNormal = -VoxelData::getNormal(farFacing);

				// Wet chasms and lava chasms are unaffected by ceiling height.
				const double chasmDepth = (chasmData.type == VoxelData::ChasmData::Type::Dry) ?
					voxelHeight : VoxelData::ChasmData::WET_LAVA_DEPTH;

				const Double3 farCeilingPoint(
					farPoint.x,
					voxelYReal + voxelHeight,
					farPoint.y);
				const Double3 farFloorPoint(
					farPoint.x,
					farCeilingPoint.y - chasmDepth,
					farPoint.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					farCeilingPoint, farFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, farZ, farU, 0.0,
					Constants::JustBelowOne, farNormal, textures.at(chasmData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			const VoxelData::DoorData &doorData = voxelData.door;
			const double percentOpen = SoftwareRenderer::getDoorPercentOpen(
				voxelX, voxelZ, openDoors);

			RayHit hit;
			const bool success = SoftwareRenderer::findDoorIntersection(voxelX, voxelZ,
				doorData.type, percentOpen, facing, nearPoint, farPoint, wallU, hit);

			if (success)
			{
				if (doorData.type == VoxelData::DoorData::Type::Swinging)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Sliding)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Raising)
				{
					// Top point is fixed, bottom point depends on percent open.
					const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
					const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal + raisedAmount,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					// The start of the vertical texture coordinate depends on the percent open.
					const double vStart = raisedAmount / voxelHeight;

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, vStart,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id), shadingInfo,
						occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Splitting)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
			}
		}
	};

	auto drawVoxelAbove = [x, voxelX, voxelZ, &camera, &ray, facing, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &openDoors, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelHeight = ceilingHeight;
		const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

		if (voxelData.dataType == VoxelDataType::Wall)
		{
			const VoxelData::WallData &wallData = voxelData.wall;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);
			const Double3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);
			
			// Wall.
			SoftwareRenderer::drawPixels(x, drawRanges.at(0), nearZ, wallU, 0.0,
				Constants::JustBelowOne, wallNormal, textures.at(wallData.sideID), shadingInfo,
				occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.at(wallData.floorID), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Floor)
		{
			// Do nothing. Floors can only be seen from above.
		}
		else if (voxelData.dataType == VoxelDataType::Ceiling)
		{
			// Draw bottom of ceiling voxel.
			const VoxelData::CeilingData &ceilingData = voxelData.ceiling;

			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);
			const Double3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);

			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.at(ceilingData.id), shadingInfo,
				occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Raised)
		{
			const VoxelData::RaisedData &raisedData = voxelData.raised;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal + (raisedData.yOffset * voxelHeight),
				nearPoint.y);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
					farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint,
					farZ, nearZ, Double3::UnitY, textures.at(raisedData.ceilingID), shadingInfo,
					occlusion, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
					nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);

				// Wall.
				SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(0), nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
					nearZ, farZ, -Double3::UnitY, textures.at(raisedData.floorID), shadingInfo,
					occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				const auto drawRange = SoftwareRenderer::makeDrawRange(
					nearCeilingPoint, nearFloorPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU,
					raisedData.vTop, raisedData.vBottom, wallNormal,
					textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Diagonal)
		{
			const VoxelData::DiagonalData &diagData = voxelData.diagonal;

			// Find intersection.
			RayHit hit;
			const bool success = diagData.type1 ?
				SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
				SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				const Double3 diagTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const Double3 diagBottomPoint(
					diagTopPoint.x,
					voxelYReal,
					diagTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					diagTopPoint, diagBottomPoint, camera, frame);

				SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.at(diagData.id), shadingInfo,
					occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::TransparentWall)
		{
			// Draw transparent side.
			const VoxelData::TransparentWallData &transparentWallData = voxelData.transparentWall;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU, 0.0,
				Constants::JustBelowOne, wallNormal, textures.at(transparentWallData.id),
				shadingInfo, occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
				edgeData.facing, edgeData.flipped, facing, nearPoint, farPoint, wallU,
				camera, ray, hit);

			if (success)
			{
				const Double3 edgeTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight + edgeData.yOffset,
					hit.point.y);
				const Double3 edgeBottomPoint(
					hit.point.x,
					voxelYReal + edgeData.yOffset,
					hit.point.y);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					edgeTopPoint, edgeBottomPoint, camera, frame);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
					0.0, Constants::JustBelowOne, hit.normal, textures.at(edgeData.id),
					shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Ignore. Chasms should never be above the player's voxel.
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			const VoxelData::DoorData &doorData = voxelData.door;
			const double percentOpen = SoftwareRenderer::getDoorPercentOpen(
				voxelX, voxelZ, openDoors);

			RayHit hit;
			const bool success = SoftwareRenderer::findDoorIntersection(voxelX, voxelZ,
				doorData.type, percentOpen, facing, nearPoint, farPoint, wallU, hit);

			if (success)
			{
				if (doorData.type == VoxelData::DoorData::Type::Swinging)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
						hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Sliding)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Raising)
				{
					// Top point is fixed, bottom point depends on percent open.
					const double minVisible = SoftwareRenderer::DOOR_MIN_VISIBLE;
					const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal + raisedAmount,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					// The start of the vertical texture coordinate depends on the percent open.
					const double vStart = raisedAmount / voxelHeight;

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, vStart,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id), shadingInfo,
						occlusion, frame);
				}
				else if (doorData.type == VoxelData::DoorData::Type::Splitting)
				{
					const Double3 doorTopPoint(
						hit.point.x,
						voxelYReal + voxelHeight,
						hit.point.y);
					const Double3 doorBottomPoint(
						doorTopPoint.x,
						voxelYReal,
						doorTopPoint.z);

					const auto drawRange = SoftwareRenderer::makeDrawRange(
						doorTopPoint, doorBottomPoint, camera, frame);

					SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
						Constants::JustBelowOne, hit.normal, textures.at(doorData.id),
						shadingInfo, occlusion, frame);
				}
			}
		}
	};

	// Relative Y voxel coordinate of the camera, compensating for the ceiling height.
	const int adjustedVoxelY = camera.getAdjustedEyeVoxelY(ceilingHeight);

	// Draw voxel straight ahead first.
	drawVoxel(adjustedVoxelY);

	// Draw voxels below the voxel.
	for (int voxelY = (adjustedVoxelY - 1); voxelY >= 0; voxelY--)
	{
		drawVoxelBelow(voxelY);
	}

	// Draw voxels above the voxel.
	for (int voxelY = (adjustedVoxelY + 1); voxelY < voxelGrid.getHeight(); voxelY++)
	{
		drawVoxelAbove(voxelY);
	}
}

void SoftwareRenderer::drawFlat(int startX, int endX, const Flat::Frame &flatFrame,
	const Double3 &normal, bool flipped, const Double2 &eye, const ShadingInfo &shadingInfo,
	const FlatTexture &texture, const FrameView &frame)
{
	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// X percents across the screen for the given start and end columns.
	const double startXPercent = (static_cast<double>(startX) + 0.50) / 
		static_cast<double>(frame.width);
	const double endXPercent = (static_cast<double>(endX) + 0.50) /
		static_cast<double>(frame.width);

	const bool startsInRange =
		(flatFrame.startX >= startXPercent) && (flatFrame.startX <= endXPercent);
	const bool endsInRange = 
		(flatFrame.endX >= startXPercent) && (flatFrame.endX <= endXPercent);
	const bool coversRange =
		(flatFrame.startX <= startXPercent) && (flatFrame.endX >= endXPercent);
	
	// Throw out the draw call if the flat is not in the X range.
	if (!startsInRange && !endsInRange && !coversRange)
	{
		return;
	}

	// Get the min and max X range of coordinates in screen-space. This range is completely 
	// contained within the flat.
	const double clampedStartXPercent = MathUtils::clamp(
		startXPercent, flatFrame.startX, flatFrame.endX);
	const double clampedEndXPercent = MathUtils::clamp(
		endXPercent, flatFrame.startX, flatFrame.endX);

	// The percentages from start to end within the flat.
	const double startFlatPercent = (clampedStartXPercent - flatFrame.startX) /
		(flatFrame.endX - flatFrame.startX);
	const double endFlatPercent = (clampedEndXPercent - flatFrame.startX) /
		(flatFrame.endX - flatFrame.startX);

	// Points interpolated between for per-column depth calculations in the XZ plane.
	const Double3 startTopPoint = flatFrame.topStart.lerp(flatFrame.topEnd, startFlatPercent);
	const Double3 endTopPoint = flatFrame.topStart.lerp(flatFrame.topEnd, endFlatPercent);

	// Horizontal texture coordinates in the flat. Although the flat percent can be
	// equal to 1.0, the texture coordinate needs to be less than 1.0.
	const double startU = MathUtils::clamp(startFlatPercent, 0.0, Constants::JustBelowOne);
	const double endU = MathUtils::clamp(endFlatPercent, 0.0, Constants::JustBelowOne);

	// Get the start and end coordinates of the projected points (Y values potentially
	// outside the screen).
	const double projectedXStart = clampedStartXPercent * frame.widthReal;
	const double projectedXEnd = clampedEndXPercent * frame.widthReal;
	const double projectedYStart = flatFrame.startY * frame.heightReal;
	const double projectedYEnd = flatFrame.endY * frame.heightReal;

	// Clamp the coordinates for where the flat starts and stops on the screen.
	const int xStart = SoftwareRenderer::getLowerBoundedPixel(projectedXStart, frame.width);
	const int xEnd = SoftwareRenderer::getUpperBoundedPixel(projectedXEnd, frame.width);
	const int yStart = SoftwareRenderer::getLowerBoundedPixel(projectedYStart, frame.height);
	const int yEnd = SoftwareRenderer::getUpperBoundedPixel(projectedYEnd, frame.height);

	// Shading on the texture.
	// - @todo: contribution from lights.
	const Double3 shading(
		shadingInfo.ambient + sunComponent.x,
		shadingInfo.ambient + sunComponent.y,
		shadingInfo.ambient + sunComponent.z);

	// Draw by-column, similar to wall rendering.
	for (int x = xStart; x < xEnd; x++)
	{
		const double xPercent = ((static_cast<double>(x) + 0.50) - projectedXStart) /
			(projectedXEnd - projectedXStart);

		// Horizontal texture coordinate.
		const double u = startU + ((endU - startU) * xPercent);

		// Horizontal texel position.
		const int textureX = static_cast<int>(
			(flipped ? (Constants::JustBelowOne - u) : u) *
			static_cast<double>(texture.width));

		const Double3 topPoint = startTopPoint.lerp(endTopPoint, xPercent);

		// Get the true XZ distance for the depth.
		const double depth = (Double2(topPoint.x, topPoint.z) - eye).length();

		// Linearly interpolated fog.
		const Double3 &fogColor = shadingInfo.getFogColor();
		const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

		for (int y = yStart; y < yEnd; y++)
		{
			const int index = x + (y * frame.width);

			if (depth <= frame.depthBuffer[index])
			{
				const double yPercent = ((static_cast<double>(y) + 0.50) - projectedYStart) /
					(projectedYEnd - projectedYStart);

				// Vertical texture coordinate.
				const double startV = 0.0;
				const double endV = Constants::JustBelowOne;
				const double v = startV + ((endV - startV) * yPercent);

				// Vertical texel position.
				const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

				// Alpha is checked in this loop, and transparent texels are not drawn.
				// Flats do not have emission, so ignore it.
				const int textureIndex = textureX + (textureY * texture.width);
				const FlatTexel &texel = texture.texels[textureIndex];

				if (texel.a > 0.0)
				{
					// Texture color with shading.
					const double shadingMax = 1.0;
					double colorR = texel.r * std::min(shading.x, shadingMax);
					double colorG = texel.g * std::min(shading.y, shadingMax);
					double colorB = texel.b * std::min(shading.z, shadingMax);

					// Linearly interpolate with fog.
					colorR += (fogColor.x - colorR) * fogPercent;
					colorG += (fogColor.y - colorG) * fogPercent;
					colorB += (fogColor.z - colorB) * fogPercent;

					// Clamp maximum (don't worry about negative values).
					const double high = 1.0;
					colorR = (colorR > high) ? high : colorR;
					colorG = (colorG > high) ? high : colorG;
					colorB = (colorB > high) ? high : colorB;

					// Convert floats to integers.
					const uint32_t colorRGB = static_cast<uint32_t>(
						((static_cast<uint8_t>(colorR * 255.0)) << 16) |
						((static_cast<uint8_t>(colorG * 255.0)) << 8) |
						((static_cast<uint8_t>(colorB * 255.0))));

					frame.colorBuffer[index] = colorRGB;
					frame.depthBuffer[index] = depth;
				}
			}
		}
	}
}

void SoftwareRenderer::rayCast2D(int x, const Camera &camera, const Ray &ray,
	const ShadingInfo &shadingInfo, double ceilingHeight,
	const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
	const std::vector<VoxelTexture> &textures, OcclusionData &occlusion, const FrameView &frame)
{
	// Initially based on Lode Vandevenne's algorithm, this method of 2.5D ray casting is more 
	// expensive as it does not stop at the first wall intersection, and it also renders voxels 
	// above and below the current floor.

	// Some floating point behavior assumptions:
	// -> (value / 0.0) == infinity
	// -> (value / infinity) == 0.0
	// -> (int)(-0.8) == 0
	// -> (int)floor(-0.8) == -1
	// -> (int)ceil(-0.8) == 0

	const double dirXSquared = ray.dirX * ray.dirX;
	const double dirZSquared = ray.dirZ * ray.dirZ;

	const double deltaDistX = std::sqrt(1.0 + (dirZSquared / dirXSquared));
	const double deltaDistZ = std::sqrt(1.0 + (dirXSquared / dirZSquared));

	const bool nonNegativeDirX = ray.dirX >= 0.0;
	const bool nonNegativeDirZ = ray.dirZ >= 0.0;

	int stepX, stepZ;
	double sideDistX, sideDistZ;
	if (nonNegativeDirX)
	{
		stepX = 1;
		sideDistX = (camera.eyeVoxelReal.x + 1.0 - camera.eye.x) * deltaDistX;
	}
	else
	{
		stepX = -1;
		sideDistX = (camera.eye.x - camera.eyeVoxelReal.x) * deltaDistX;
	}

	if (nonNegativeDirZ)
	{
		stepZ = 1;
		sideDistZ = (camera.eyeVoxelReal.z + 1.0 - camera.eye.z) * deltaDistZ;
	}
	else
	{
		stepZ = -1;
		sideDistZ = (camera.eye.z - camera.eyeVoxelReal.z) * deltaDistZ;
	}

	// The Z distance from the camera to the wall, and the X or Z normal of the intersected
	// voxel face. The first Z distance is a special case, so it's brought outside the 
	// DDA loop.
	double zDistance;
	VoxelData::Facing facing;

	// Verify that the initial voxel coordinate is within the world bounds.
	bool voxelIsValid = 
		(camera.eyeVoxel.x >= 0) && 
		(camera.eyeVoxel.y >= 0) && 
		(camera.eyeVoxel.z >= 0) &&
		(camera.eyeVoxel.x < voxelGrid.getWidth()) && 
		(camera.eyeVoxel.y < voxelGrid.getHeight()) &&
		(camera.eyeVoxel.z < voxelGrid.getDepth());

	if (voxelIsValid)
	{
		// Decide how far the wall is, and which voxel face was hit.
		if (sideDistX < sideDistZ)
		{
			zDistance = sideDistX;
			facing = nonNegativeDirX ? VoxelData::Facing::NegativeX : 
				VoxelData::Facing::PositiveX;
		}
		else
		{
			zDistance = sideDistZ;
			facing = nonNegativeDirZ ? VoxelData::Facing::NegativeZ : 
				VoxelData::Facing::PositiveZ;
		}

		// The initial near point is directly in front of the player in the near Z 
		// camera plane.
		const Double2 initialNearPoint(
			camera.eye.x + (ray.dirX * SoftwareRenderer::NEAR_PLANE),
			camera.eye.z + (ray.dirZ * SoftwareRenderer::NEAR_PLANE));

		// The initial far point is the wall hit. This is used with the player's position 
		// for drawing the initial floor and ceiling.
		const Double2 initialFarPoint(
			camera.eye.x + (ray.dirX * zDistance),
			camera.eye.z + (ray.dirZ * zDistance));

		// Draw all voxels in a column at the player's XZ coordinate.
		SoftwareRenderer::drawInitialVoxelColumn(x, camera.eyeVoxel.x, camera.eyeVoxel.z,
			camera, ray, facing, initialNearPoint, initialFarPoint, SoftwareRenderer::NEAR_PLANE, 
			zDistance, shadingInfo, ceilingHeight, openDoors, voxelGrid, textures, occlusion,
			frame);
	}

	// The current voxel coordinate in the DDA loop. For all intents and purposes,
	// the Y cell coordinate is constant.
	Int3 cell(camera.eyeVoxel.x, camera.eyeVoxel.y, camera.eyeVoxel.z);

	// Lambda for stepping to the next XZ coordinate in the grid and updating the Z
	// distance for the current edge point.
	auto doDDAStep = [&camera, &ray, &voxelGrid, &sideDistX, &sideDistZ, &cell,
		&facing, &voxelIsValid, &zDistance, deltaDistX, deltaDistZ, stepX, stepZ,
		nonNegativeDirX, nonNegativeDirZ]()
	{
		if (sideDistX < sideDistZ)
		{
			sideDistX += deltaDistX;
			cell.x += stepX;
			facing = nonNegativeDirX ? VoxelData::Facing::NegativeX : 
				VoxelData::Facing::PositiveX;
			voxelIsValid &= (cell.x >= 0) && (cell.x < voxelGrid.getWidth());
		}
		else
		{
			sideDistZ += deltaDistZ;
			cell.z += stepZ;
			facing = nonNegativeDirZ ? VoxelData::Facing::NegativeZ : 
				VoxelData::Facing::PositiveZ;
			voxelIsValid &= (cell.z >= 0) && (cell.z < voxelGrid.getDepth());
		}

		const bool onXAxis = (facing == VoxelData::Facing::PositiveX) || 
			(facing == VoxelData::Facing::NegativeX);

		// Update the Z distance depending on which axis was stepped with.
		if (onXAxis)
		{
			zDistance = (static_cast<double>(cell.x) - 
				camera.eye.x + static_cast<double>((1 - stepX) / 2)) / ray.dirX;
		}
		else
		{
			zDistance = (static_cast<double>(cell.z) -
				camera.eye.z + static_cast<double>((1 - stepZ) / 2)) / ray.dirZ;
		}
	};

	// Step forward in the grid once to leave the initial voxel and update the Z distance.
	doDDAStep();

	// Step through the voxel grid while the current coordinate is valid, the 
	// distance stepped is less than the distance at which fog is maximum, and
	// the column is not completely occluded.
	while (voxelIsValid && (zDistance < shadingInfo.fogDistance) && 
		(occlusion.yMin != occlusion.yMax))
	{
		// Store the cell coordinates, axis, and Z distance for wall rendering. The
		// loop needs to do another DDA step to calculate the far point.
		const int savedCellX = cell.x;
		const int savedCellZ = cell.z;
		const VoxelData::Facing savedFacing = facing;
		const double wallDistance = zDistance;

		// Decide which voxel in the XZ plane to step to next, and update the Z distance.
		doDDAStep();

		// Near and far points in the XZ plane. The near point is where the wall is, and 
		// the far point is used with the near point for drawing the floor and ceiling.
		const Double2 nearPoint(
			camera.eye.x + (ray.dirX * wallDistance),
			camera.eye.z + (ray.dirZ * wallDistance));
		const Double2 farPoint(
			camera.eye.x + (ray.dirX * zDistance),
			camera.eye.z + (ray.dirZ * zDistance));

		// Draw all voxels in a column at the given XZ coordinate.
		SoftwareRenderer::drawVoxelColumn(x, savedCellX, savedCellZ, camera, ray, savedFacing,
			nearPoint, farPoint, wallDistance, zDistance, shadingInfo, ceilingHeight, 
			openDoors, voxelGrid, textures, occlusion, frame);
	}
}

void SoftwareRenderer::drawSkyGradient(int startY, int endY, const Camera &camera,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// Lambda for drawing one row of colors and depth in the frame buffer.
	auto drawSkyRow = [&frame](int y, const Double3 &color)
	{
		uint32_t *colorPtr = frame.colorBuffer;
		double *depthPtr = frame.depthBuffer;
		const int startIndex = y * frame.width;
		const int endIndex = (y + 1) * frame.width;
		const uint32_t colorValue = color.toRGB();
		constexpr double depthValue = std::numeric_limits<double>::infinity();

		// Clear the color and depth of one row.
		for (int i = startIndex; i < endIndex; i++)
		{
			colorPtr[i] = colorValue;
			depthPtr[i] = depthValue;
		}
	};

	// Get two points some arbitrary distance away from the camera to use as the top
	// and bottom reference points of the sky gradient.
	const Double3 forward = Double3(camera.forwardX, 0.0, camera.forwardZ).normalized();

	// Determine the sky gradient's position on-screen by getting the projected Y percentages for
	// the start and end. If these values are less than 0 or greater than 1, they are off-screen.
	const double gradientTopYPercent = [&camera, &forward]()
	{
		const Double3 gradientTopPoint = [&camera, &forward]()
		{
			// Top of the sky gradient is some angle above the horizon.
			const double gradientAngleRadians =
				SoftwareRenderer::SKY_GRADIENT_ANGLE * Constants::DegToRad;

			// Height of the gradient's triangle with width of 1 and angle of 30 degrees.
			const double upPercent = std::tan(gradientAngleRadians);
			const Double3 up = Double3::UnitY;

			// Direction from camera eye to the top of the sky gradient.
			const Double3 gradientTopDir = (forward + (up * upPercent)).normalized();

			return camera.eye + gradientTopDir;
		}();

		return SoftwareRenderer::getProjectedY(
			gradientTopPoint, camera.transform, camera.yShear);
	}();

	const double gradientBottomYPercent = [&camera, &forward]()
	{
		const Double3 gradientBottomPoint = camera.eye + forward;
		return SoftwareRenderer::getProjectedY(
			gradientBottomPoint, camera.transform, camera.yShear);
	}();

	for (int y = startY; y < endY; y++)
	{
		// Y percent across the screen.
		const double yPercent = (static_cast<double>(y) + 0.50) / frame.heightReal;

		// Y percent relative to the sky gradient.
		const double gradientPercent = Constants::JustBelowOne - MathUtils::clamp(
			(yPercent - gradientTopYPercent) / (gradientBottomYPercent - gradientTopYPercent),
			0.0, Constants::JustBelowOne);

		// Determine which sky color index the percent falls into, and how much of that
		// color to interpolate with the next one.
		const auto &skyColors = shadingInfo.skyColors;
		const int skyColorCount = static_cast<int>(skyColors.size());
		const double realIndex = gradientPercent * static_cast<double>(skyColorCount);
		const double percent = realIndex - std::floor(realIndex);
		const int index = static_cast<int>(realIndex);
		const int nextIndex = MathUtils::clamp(index + 1, 0, skyColorCount - 1);
		const Double3 color = skyColors.at(index).lerp(skyColors.at(nextIndex), percent);

		drawSkyRow(y, color);
	}
}

void SoftwareRenderer::drawDistantSky(int startX, int endX, bool parallaxSky,
	const std::vector<VisDistantObject> &visDistantObjs, const Camera &camera,
	const std::vector<SkyTexture> &skyTextures, const ShadingInfo &shadingInfo,
	const FrameView &frame)
{
	// For each visible distant object, if it is at least partially within the start and end
	// X, then draw. Reverse iterate so objects are drawn far to near.
	for (auto it = visDistantObjs.rbegin(); it != visDistantObjs.rend(); ++it)
	{
		const VisDistantObject &obj = *it;
		const SkyTexture &texture = *obj.texture;
		const DrawRange &drawRange = obj.drawRange;
		const double xProjStart = obj.xProjStart;
		const double xProjEnd = obj.xProjEnd;
		const int xDrawStart = std::max(obj.xStart, startX);
		const int xDrawEnd = std::min(obj.xEnd, endX);
		const bool emissive = obj.emissive;

		if (parallaxSky)
		{
			// Parallax rendering. Render the object based on its left and right edges.

			// @todo: see if these are necessary.
			//const double xAngleStart = obj.xAngleStart;
			//const double xAngleEnd = obj.xAngleEnd;

			for (int x = xDrawStart; x < xDrawEnd; x++)
			{
				// Percent X across the screen.
				const double xPercent = (static_cast<double>(x) + 0.50) / frame.widthReal;

				// Percentage across the horizontal span of the object in screen space.
				const double widthPercent = MathUtils::clamp(
					(xPercent - xProjStart) / (xProjEnd - xProjStart),
					0.0, Constants::JustBelowOne);

				// Horizontal texture coordinate, accounting for parallax.
				// @todo: incorporate angle/field of view/delta angle from center of view into this.
				const double u = widthPercent;

				SoftwareRenderer::drawDistantPixels(x, drawRange, u, 0.0,
					Constants::JustBelowOne, texture, emissive, shadingInfo, frame);
			}
		}
		else
		{
			// Classic rendering. Render the object based on its midpoint.
			for (int x = xDrawStart; x < xDrawEnd; x++)
			{
				// Percent X across the screen.
				const double xPercent = (static_cast<double>(x) + 0.50) / frame.widthReal;

				// Percentage across the horizontal span of the object in screen space.
				const double widthPercent = MathUtils::clamp(
					(xPercent - xProjStart) / (xProjEnd - xProjStart),
					0.0, Constants::JustBelowOne);

				// Horizontal texture coordinate, not accounting for parallax.
				const double u = widthPercent;

				SoftwareRenderer::drawDistantPixels(x, drawRange, u, 0.0,
					Constants::JustBelowOne, texture, emissive, shadingInfo, frame);
			}
		}
	}
}

void SoftwareRenderer::drawVoxels(int startX, int stride, const Camera &camera,
	double ceilingHeight, const std::vector<LevelData::DoorState> &openDoors,
	const VoxelGrid &voxelGrid, const std::vector<VoxelTexture> &voxelTextures,
	std::vector<OcclusionData> &occlusion, const ShadingInfo &shadingInfo, const FrameView &frame)
{
	const Double2 forwardZoomed(camera.forwardZoomedX, camera.forwardZoomedZ);
	const Double2 rightAspected(camera.rightAspectedX, camera.rightAspectedZ);

	// Draw pixel columns with spacing determined by the number of render threads.
	for (int x = startX; x < frame.width; x += stride)
	{
		// X percent across the screen.
		const double xPercent = (static_cast<double>(x) + 0.50) / frame.widthReal;

		// "Right" component of the ray direction, based on current screen X.
		const Double2 rightComp = rightAspected * ((2.0 * xPercent) - 1.0);

		// Calculate the ray direction through the pixel.
		// - If un-normalized, it uses the Z distance, but the insides of voxels
		//   don't look right then.
		const Double2 direction = (forwardZoomed + rightComp).normalized();
		const Ray ray(direction.x, direction.y);

		// Cast the 2D ray and fill in the column's pixels with color.
		SoftwareRenderer::rayCast2D(x, camera, ray, shadingInfo, ceilingHeight, openDoors,
			voxelGrid, voxelTextures, occlusion.at(x), frame);
	}
}

void SoftwareRenderer::drawFlats(int startX, int endX, const Camera &camera,
	const Double3 &flatNormal, const std::vector<VisibleFlat> &visibleFlats,
	const std::vector<FlatTexture> &flatTextures, const ShadingInfo &shadingInfo,
	const FrameView &frame)
{
	// Iterate through all flats, rendering those visible within the given X range of 
	// the screen.
	for (const auto &visibleFlat : visibleFlats)
	{
		const Flat &flat = visibleFlat.getFlat();
		const Flat::Frame &flatFrame = visibleFlat.getFrame();

		// Texture of the flat. It might be flipped horizontally as well, given by
		// the "flat.flipped" value.
		const FlatTexture &texture = flatTextures.at(flat.textureID);

		const Double2 eye2D(camera.eye.x, camera.eye.z);

		SoftwareRenderer::drawFlat(startX, endX, flatFrame, flatNormal, flat.flipped,
			eye2D, shadingInfo, texture, frame);
	}
}

void SoftwareRenderer::renderThreadLoop(RenderThreadData &threadData, int threadIndex, int startX,
	int endX, int startY, int endY)
{
	while (true)
	{
		// Initial wait condition. The lock must be unlocked after wait() so other threads can
		// lock it.
		std::unique_lock<std::mutex> lk(threadData.mutex);
		threadData.condVar.wait(lk, [&threadData]() { return threadData.go; });
		lk.unlock();

		// Received a go signal. Check if the renderer is being destroyed before doing anything.
		if (threadData.isDestructing)
		{
			break;
		}

		// Draw this thread's portion of the sky gradient.
		RenderThreadData::SkyGradient &skyGradient = threadData.skyGradient;
		SoftwareRenderer::drawSkyGradient(startY, endY, *threadData.camera,
			*threadData.shadingInfo, *threadData.frame);

		// This thread is done with the sky gradient.
		lk.lock();
		skyGradient.threadsDone++;

		// If this was the last thread on the sky gradient, notify all to continue.
		if (skyGradient.threadsDone == threadData.totalThreads)
		{
			lk.unlock();
			threadData.condVar.notify_all();
		}
		else
		{
			// Wait for other threads to finish.
			threadData.condVar.wait(lk, [&threadData, &skyGradient]()
			{
				return skyGradient.threadsDone == threadData.totalThreads;
			});

			lk.unlock();
		}

		// Wait for the visible distant object testing to finish.
		RenderThreadData::DistantSky &distantSky = threadData.distantSky;
		lk.lock();
		threadData.condVar.wait(lk, [&distantSky]() { return distantSky.doneVisTesting; });
		lk.unlock();

		// Draw this thread's portion of distant sky objects.
		SoftwareRenderer::drawDistantSky(startX, endX, distantSky.parallaxSky,
			*distantSky.visDistantObjs, *threadData.camera, *distantSky.skyTextures,
			*threadData.shadingInfo, *threadData.frame);

		// This thread is done with distant sky objects.
		lk.lock();
		distantSky.threadsDone++;

		// If this was the last thread on distant sky, notify all to continue.
		if (distantSky.threadsDone == threadData.totalThreads)
		{
			lk.unlock();
			threadData.condVar.notify_all();
		}
		else
		{
			// Wait for other threads to finish.
			threadData.condVar.wait(lk, [&threadData, &distantSky]()
			{
				return distantSky.threadsDone == threadData.totalThreads;
			});

			lk.unlock();
		}

		// Number of columns to skip per ray cast (for interleaved ray casting as a means of
		// load-balancing).
		const int strideX = threadData.totalThreads;

		// Draw this thread's portion of voxels.
		RenderThreadData::Voxels &voxels = threadData.voxels;
		SoftwareRenderer::drawVoxels(threadIndex, strideX, *threadData.camera,
			voxels.ceilingHeight, *voxels.openDoors, *voxels.voxelGrid, *voxels.voxelTextures,
			*voxels.occlusion, *threadData.shadingInfo, *threadData.frame);

		// This thread is done with voxels.
		lk.lock();
		voxels.threadsDone++;

		// If this was the last thread on voxels, notify all to continue.
		if (voxels.threadsDone == threadData.totalThreads)
		{
			lk.unlock();
			threadData.condVar.notify_all();
		}
		else
		{
			// Wait for other threads to finish.
			threadData.condVar.wait(lk, [&threadData, &voxels]()
			{
				return voxels.threadsDone == threadData.totalThreads;
			});

			lk.unlock();
		}

		// Wait for the visible flat sorting to finish.
		RenderThreadData::Flats &flats = threadData.flats;
		lk.lock();
		threadData.condVar.wait(lk, [&flats]() { return flats.doneSorting; });
		lk.unlock();

		// Draw this thread's portion of flats.
		SoftwareRenderer::drawFlats(startX, endX, *threadData.camera, *flats.flatNormal,
			*flats.visibleFlats, *flats.flatTextures, *threadData.shadingInfo, *threadData.frame);

		// This thread is done with flats.
		lk.lock();
		flats.threadsDone++;

		// If this was the last thread on flats, notify all to continue.
		if (flats.threadsDone == threadData.totalThreads)
		{
			lk.unlock();
			threadData.condVar.notify_all();
		}
		else
		{
			// Wait for other threads to finish.
			threadData.condVar.wait(lk, [&threadData, &flats]()
			{
				return flats.threadsDone == threadData.totalThreads;
			});

			lk.unlock();
		}
	}
}

void SoftwareRenderer::render(const Double3 &eye, const Double3 &direction, double fovY,
	double ambient, double daytimePercent, bool parallaxSky, double ceilingHeight,
	const std::vector<LevelData::DoorState> &openDoors, const VoxelGrid &voxelGrid,
	uint32_t *colorBuffer)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	// To account for tall pixels.
	const double projectionModifier = SoftwareRenderer::TALL_PIXEL_RATIO;

	// 2.5D camera definition.
	const Camera camera(eye, direction, fovY, aspect, projectionModifier);

	// Normal of all flats (always facing the camera).
	const Double3 flatNormal = Double3(-camera.forwardX, 0.0, -camera.forwardZ).normalized();

	// Calculate shading information for this frame. Create some helper structs to keep similar
	// values together.
	const ShadingInfo shadingInfo(this->skyPalette, daytimePercent, ambient, this->fogDistance);
	const FrameView frame(colorBuffer, this->depthBuffer.data(), this->width, this->height);

	// Set all the render-thread-specific shared data for this frame.
	this->threadData.init(static_cast<int>(this->renderThreads.size()),
		camera, shadingInfo, frame);
	this->threadData.skyGradient.init();
	this->threadData.distantSky.init(parallaxSky, this->visDistantObjs, this->skyTextures);
	this->threadData.voxels.init(ceilingHeight, openDoors, voxelGrid,
		this->voxelTextures, this->occlusion);
	this->threadData.flats.init(flatNormal, this->visibleFlats, this->flatTextures);

	// Give the render threads the go signal. They can work on the sky and voxels while this thread
	// does things like resetting occlusion and doing visible flat determination.
	// - Note about locks: they must always be locked before wait(), and stay locked after wait().
	std::unique_lock<std::mutex> lk(this->threadData.mutex);
	this->threadData.go = true;
	lk.unlock();
	this->threadData.condVar.notify_all();

	// Reset occlusion.
	std::fill(this->occlusion.begin(), this->occlusion.end(), OcclusionData(0, this->height));

	// Refresh the visible distant objects.
	this->updateVisibleDistantObjects(parallaxSky, shadingInfo.sunDirection, camera, frame);

	lk.lock();
	this->threadData.condVar.wait(lk, [this]()
	{
		return this->threadData.skyGradient.threadsDone == this->threadData.totalThreads;
	});

	// Keep the render threads from getting the go signal again before the next frame.
	this->threadData.go = false;

	// Let the render threads know that they can start drawing distant objects.
	this->threadData.distantSky.doneVisTesting = true;

	lk.unlock();
	this->threadData.condVar.notify_all();

	// Refresh the visible flats. This should erase the old list, calculate a new list, and sort
	// it by depth.
	this->updateVisibleFlats(camera);

	lk.lock();
	this->threadData.condVar.wait(lk, [this]()
	{
		return this->threadData.voxels.threadsDone == this->threadData.totalThreads;
	});

	// Let the render threads know that they can start drawing flats.
	this->threadData.flats.doneSorting = true;

	lk.unlock();
	this->threadData.condVar.notify_all();

	// Wait until render threads are done drawing flats.
	lk.lock();
	this->threadData.condVar.wait(lk, [this]()
	{
		return this->threadData.flats.threadsDone == this->threadData.totalThreads;
	});
}
