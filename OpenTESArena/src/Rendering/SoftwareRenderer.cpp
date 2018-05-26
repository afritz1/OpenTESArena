#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <thread>

#include "SoftwareRenderer.h"
#include "../Math/Constants.h"
#include "../Media/Color.h"
#include "../Utilities/Debug.h"
#include "../Utilities/Platform.h"
#include "../World/VoxelData.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelGrid.h"

SoftwareRenderer::VoxelTexel::VoxelTexel()
{
	this->r = 0.0;
	this->g = 0.0;
	this->b = 0.0;
	this->a = 0.0;
	this->emission = 0.0;
}

SoftwareRenderer::FlatTexel::FlatTexel()
{
	this->r = 0.0;
	this->g = 0.0;
	this->b = 0.0;
	this->a = 0.0;
}

SoftwareRenderer::FlatTexture::FlatTexture()
{
	this->width = 0;
	this->height = 0;
}

SoftwareRenderer::Camera::Camera(const Double3 &eye, const Double3 &direction,
	double fovY, double aspect, double projectionModifier)
	: eye(eye)
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
	this->zoom = 1.0 / std::tan((fovY * 0.5) * Constants::DegToRad);

	this->aspect = aspect;

	// Y-shearing is the distance that projected Y coordinates are translated by based on the 
	// player's 3D direction and field of view. First get the player's angle relative to the 
	// horizon, then get the tangent of that angle. The Y component of the player's direction
	// must be clamped less than 1 because 1 would imply they are looking straight up or down, 
	// which is impossible in 2.5D rendering (the vertical line segment of the view frustum 
	// would be infinitely high or low). The camera code should take care of the clamping for us.
	this->yShear = [this, &direction]()
	{
		// Get the vertical angle of the player's direction.
		const double angleRadians = [&direction]()
		{
			// Get the length of the direction vector's projection onto the XZ plane.
			const double xzProjection = std::sqrt(
				(direction.x * direction.x) + (direction.z * direction.z));

			if (direction.y > 0.0)
			{
				// Above the horizon.
				return std::acos(xzProjection);
			}
			else if (direction.y < 0.0)
			{
				// Below the horizon.
				return -std::acos(xzProjection);
			}
			else
			{
				// At the horizon.
				return 0.0;
			}
		}();

		// Get the number of screen heights to translate all projected Y coordinates by, 
		// relative to the current zoom. As a reference, this should be some value roughly 
		// between -1.0 and 1.0 for "acceptable skewing" at a vertical FOV of 90.0. If the 
		// camera is not clamped, this could theoretically be between -infinity and infinity, 
		// but it would result in far too much skewing.
		return std::tan(angleRadians) * this->zoom;
	}();
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

SoftwareRenderer::ShadingInfo::ShadingInfo(const Double3 &horizonSkyColor, 
	const Double3 &zenithSkyColor, const Double3 &sunColor, 
	const Double3 &sunDirection, double ambient, double fogDistance)
	: horizonSkyColor(horizonSkyColor), zenithSkyColor(zenithSkyColor),
	sunColor(sunColor), sunDirection(sunDirection)
{
	this->ambient = ambient;
	this->fogDistance = fogDistance;
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

const double SoftwareRenderer::NEAR_PLANE = 0.0001;
const double SoftwareRenderer::FAR_PLANE = 1000.0;
const int SoftwareRenderer::DEFAULT_VOXEL_TEXTURE_COUNT = 64;
const int SoftwareRenderer::DEFAULT_FLAT_TEXTURE_COUNT = 256;

SoftwareRenderer::SoftwareRenderer()
{
	// Initialize values to empty.
	this->width = 0;
	this->height = 0;
	this->renderThreadsMode = 0;
	this->fogDistance = 0.0;
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
}

void SoftwareRenderer::setRenderThreadsMode(int mode)
{
	this->renderThreadsMode = mode;
}

void SoftwareRenderer::addFlat(int id, const Double3 &position, double width, 
	double height, int textureID)
{
	// Verify that the ID is not already in use.
	DebugAssert(this->flats.find(id) == this->flats.end(), 
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
			// To do: change this calculation for rotated textures. Make sure to have a 
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
			dstTexel.a = srcTexel.w;

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
	DebugAssert(flatIter != this->flats.end(), 
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
	// To do: activate lights (don't worry about textures).

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
			texel.a = texelColor.w;
			texel.emission = texelEmission;
		}
	}
}

void SoftwareRenderer::removeFlat(int id)
{
	// Make sure the flat exists before removing it.
	const auto flatIter = this->flats.find(id);
	DebugAssert(flatIter != this->flats.end(), 
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
				this->visibleFlats.push_back(std::make_pair(&flat, std::move(flatFrame)));
			}
		}
	}

	// Sort the visible flats farthest to nearest (relevant for transparencies).
	std::sort(this->visibleFlats.begin(), this->visibleFlats.end(),
		[](const std::pair<const Flat*, Flat::Frame> &a,
			const std::pair<const Flat*, Flat::Frame> &b)
	{
		return a.second.z > b.second.z;
	});
}

/*Double3 SoftwareRenderer::castRay(const Double3 &direction,
	const VoxelGrid &voxelGrid) const
{
	// This is an extension of Lode Vandevenne's DDA algorithm from 2D to 3D.
	// Technically, it could be considered a "3D-DDA" algorithm. It will eventually 
	// have some additional features so all of Arena's geometry can be represented.

	// To do:
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

Double3 SoftwareRenderer::getFogColor(double daytimePercent) const
{
	// Get the real index (not the integer index), so the color can be interpolated
	// between two samples.
	const double realIndex = static_cast<double>(this->skyPalette.size()) * daytimePercent;

	const size_t index = static_cast<size_t>(realIndex);
	const size_t nextIndex = (index + 1) % this->skyPalette.size();

	const Double3 &color = this->skyPalette[index];
	const Double3 &nextColor = this->skyPalette[nextIndex];

	const double percent = realIndex - std::floor(realIndex);
	return color.lerp(nextColor, percent);
}

Double3 SoftwareRenderer::getSunDirection(double daytimePercent) const
{
	// The sun rises in the east (+Z) and sets in the west (-Z).
	const double radians = daytimePercent * (2.0 * Constants::Pi);
	return Double3(0.0, -std::cos(radians), std::sin(radians)).normalized();
}

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

double SoftwareRenderer::fullAtan2(double y, double x)
{
	const double angle = std::atan2(y, x);
	return (angle >= 0.0) ? angle : ((2.0 * Constants::Pi) + angle);
}

Double3 SoftwareRenderer::getNormal(VoxelData::Facing facing)
{
	// Decide what the normal is, based on the facing. It can only be on the X or Z axis 
	// because floors and ceilings are drawn separately from walls, and their 
	// normals are trivial.
	if (facing == VoxelData::Facing::PositiveX)
	{
		return Double3::UnitX;
	}
	else if (facing == VoxelData::Facing::NegativeX)
	{
		return -Double3::UnitX;
	}
	else if (facing == VoxelData::Facing::PositiveZ)
	{
		return Double3::UnitZ;
	}
	else
	{
		return -Double3::UnitZ;
	}
}

VoxelData::Facing SoftwareRenderer::getInitialChasmFarFacing(int voxelX, int voxelZ,
	const Double2 &eye, const Ray &ray)
{
	// Angle of the ray from the camera eye.
	const double angle = SoftwareRenderer::fullAtan2(ray.dirX, ray.dirZ);

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
	const double upLeftAngle = SoftwareRenderer::fullAtan2(upLeft.x, upLeft.y);
	const double upRightAngle = SoftwareRenderer::fullAtan2(upRight.x, upRight.y);
	const double downLeftAngle = SoftwareRenderer::fullAtan2(downLeft.x, downLeft.y);
	const double downRightAngle = SoftwareRenderer::fullAtan2(downRight.x, downRight.y);

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
	const double angle = SoftwareRenderer::fullAtan2(ray.dirX, ray.dirZ);

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
	const double upLeftAngle = SoftwareRenderer::fullAtan2(upLeft.x, upLeft.y);
	const double upRightAngle = SoftwareRenderer::fullAtan2(upRight.x, upRight.y);
	const double downLeftAngle = SoftwareRenderer::fullAtan2(downLeft.x, downLeft.y);
	const double downRightAngle = SoftwareRenderer::fullAtan2(downRight.x, downRight.y);

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
	return std::min(std::max(0,
		static_cast<int>(std::ceil(projected - 0.50))), frameDim);
}

int SoftwareRenderer::getUpperBoundedPixel(double projected, int frameDim)
{
	return std::min(std::max(0,
		static_cast<int>(std::floor(projected + 0.50))), frameDim);
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
		hit.u = std::max(std::min(hitCoordinate, Constants::JustBelowOne), 0.0);
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
		hit.u = std::max(std::min(hitCoordinate, Constants::JustBelowOne), 0.0);
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
	VoxelData::Facing edgeFacing, const Double2 &nearPoint, const Double2 &farPoint,
	const Camera &camera, const Ray &ray, RayHit &hit)
{
	// Reuse the chasm facing code to find which face is intersected.
	const VoxelData::Facing farFacing = SoftwareRenderer::getInitialChasmFarFacing(
		voxelX, voxelZ, Double2(camera.eye.x, camera.eye.z), ray);

	// If the edge facing and far facing match, there's an intersection.
	if (edgeFacing == farFacing)
	{
		hit.innerZ = (farPoint - nearPoint).length();
		hit.u = [&farPoint, farFacing]()
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

			return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
		}();

		hit.point = farPoint;
		hit.normal = -SoftwareRenderer::getNormal(farFacing);
		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool SoftwareRenderer::findEdgeIntersection(int voxelX, int voxelZ, VoxelData::Facing edgeFacing,
	VoxelData::Facing nearFacing, const Double2 &nearPoint, const Double2 &farPoint, double nearU,
	const Camera &camera, const Ray &ray, RayHit &hit)
{
	// If the edge facing and near facing match, the intersection is trivial.
	if (edgeFacing == nearFacing)
	{
		hit.innerZ = 0.0;
		hit.u = nearU;
		hit.point = nearPoint;
		hit.normal = SoftwareRenderer::getNormal(nearFacing);
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
			hit.u = [&farPoint, farFacing]()
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

				return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
			}();

			hit.point = farPoint;
			hit.normal = -SoftwareRenderer::getNormal(farFacing);
			return true;
		}
		else
		{
			// No intersection.
			return false;
		}
	}
}

bool SoftwareRenderer::findDoorIntersection(int voxelX, int voxelZ, 
	VoxelData::DoorData::Type doorType, const Double2 &nearPoint, 
	const Double2 &farPoint, RayHit &hit)
{
	// To do.
	return false;
}

void SoftwareRenderer::diagProjection(double voxelYReal, double voxelHeight,
	const Double2 &point, const Matrix4d &transform, double yShear, int frameHeight, 
	double heightReal, double &diagTopScreenY, double &diagBottomScreenY, 
	int &diagStart, int &diagEnd)
{
	// Points in world space for the top and bottom of the diagonal wall slice.
	const Double3 diagTop(point.x, voxelYReal + voxelHeight, point.y);
	const Double3 diagBottom(point.x, voxelYReal, point.y);

	// Projected Y coordinates on-screen.
	diagTopScreenY = SoftwareRenderer::getProjectedY(
		diagTop, transform, yShear) * heightReal;
	diagBottomScreenY = SoftwareRenderer::getProjectedY(
		diagBottom, transform, yShear) * heightReal;

	// Y drawing range on-screen.
	diagStart = std::min(std::max(0,
		static_cast<int>(std::ceil(diagTopScreenY - 0.50))), frameHeight);
	diagEnd = std::min(std::max(0,
		static_cast<int>(std::floor(diagBottomScreenY + 0.50))), frameHeight);
}

void SoftwareRenderer::drawPixels(int x, int yStart, int yEnd, double projectedYStart,
	double projectedYEnd, double depth, double u, double vStart, double vEnd,
	const Double3 &normal, const VoxelTexture &texture, const ShadingInfo &shadingInfo,
	OcclusionData &occlusion, const FrameView &frame)
{
	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(VoxelTexture::WIDTH));

	// Linearly interpolated fog.
	const Double3 &fogColor = shadingInfo.horizonSkyColor;
	const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Shading on the texture.
	// - To do: contribution from lights.
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
		// - To do: implement occlusion culling and back-to-front transparent rendering so
		//   this depth check isn't needed.
		if (depth <= (frame.depthBuffer[index] - Constants::Epsilon))
		{
			// Percent stepped from beginning to end on the column.
			const double yPercent = 
				((static_cast<double>(y) + 0.50) - projectedYStart) /
				(projectedYEnd - projectedYStart);

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

void SoftwareRenderer::drawPerspectivePixels(int x, int yStart, int yEnd, double projectedYStart,
	double projectedYEnd, const Double2 &startPoint, const Double2 &endPoint, double depthStart,
	double depthEnd, const Double3 &normal, const VoxelTexture &texture,
	const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame)
{
	// Fog color to interpolate with.
	const Double3 &fogColor = shadingInfo.horizonSkyColor;

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Shading on the texture.
	// - To do: contribution from lights.
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
			((static_cast<double>(y) + 0.50) - projectedYStart) /
			(projectedYEnd - projectedYStart);

		// Interpolate between the near and far depth.
		const double depth = 1.0 / 
			(depthStartRecip + ((depthEndRecip - depthStartRecip) * yPercent));

		// Check depth of the pixel before rendering.
		// - To do: implement occlusion culling and back-to-front transparent rendering so
		//   this depth check isn't needed.
		if (depth <= frame.depthBuffer[index])
		{
			// Linearly interpolated fog.
			const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

			// Interpolate between start and end points.
			const double currentPointX = (startPointDiv.x + (pointDivDiff.x * yPercent)) * depth;
			const double currentPointY = (startPointDiv.y + (pointDivDiff.y * yPercent)) * depth;

			// Texture coordinates.
			const double u = std::max(std::min(Constants::JustBelowOne,
				Constants::JustBelowOne - (currentPointX - std::floor(currentPointX))), 0.0);
			const double v = std::max(std::min(Constants::JustBelowOne,
				Constants::JustBelowOne - (currentPointY - std::floor(currentPointY))), 0.0);

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

void SoftwareRenderer::drawTransparentPixels(int x, int yStart, int yEnd, double projectedYStart,
	double projectedYEnd, double depth, double u, double vStart, double vEnd,
	const Double3 &normal, const VoxelTexture &texture, const ShadingInfo &shadingInfo,
	const OcclusionData &occlusion, const FrameView &frame)
{
	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(VoxelTexture::WIDTH));

	// Linearly interpolated fog.
	const Double3 &fogColor = shadingInfo.horizonSkyColor;
	const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Shading on the texture.
	// - To do: contribution from lights.
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
				((static_cast<double>(y) + 0.50) - projectedYStart) /
				(projectedYEnd - projectedYStart);

			// Vertical texture coordinate.
			const double v = vStart + ((vEnd - vStart) * yPercent);

			// Y position in texture.
			const int textureY = static_cast<int>(v * static_cast<double>(VoxelTexture::HEIGHT));

			// Alpha is checked in this loop, and transparent texels are not drawn.
			const int textureIndex = textureX + (textureY * VoxelTexture::WIDTH);
			const VoxelTexel &texel = texture.texels[textureIndex];
			
			if (texel.a > 0.0)
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

void SoftwareRenderer::drawInitialVoxelColumn(int x, int voxelX, int voxelZ, const Camera &camera,
	const Ray &ray, VoxelData::Facing facing, const Double2 &nearPoint, const Double2 &farPoint,
	double nearZ, double farZ, const ShadingInfo &shadingInfo, double ceilingHeight,
	const VoxelGrid &voxelGrid, const std::vector<VoxelTexture> &textures,
	OcclusionData &occlusion, const FrameView &frame)
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

		return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
	}();

	// Normal of the wall for the incoming ray, potentially shared between multiple voxels in
	// this voxel column.
	const Double3 wallNormal = -SoftwareRenderer::getNormal(facing);

	auto drawInitialVoxel = [x, voxelX, voxelZ, &camera, &ray, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
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

			const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
				farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double farFloorScreenY = SoftwareRenderer::getProjectedY(
				farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
				farCeilingScreenY, frame.height);
			const int wallStart = ceilingEnd;
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				farFloorScreenY, frame.height);
			const int floorStart = wallEnd;
			const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
				nearCeilingScreenY, farCeilingScreenY, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.at(wallData.ceilingID), shadingInfo,
				occlusion, frame);

			// Side.
			SoftwareRenderer::drawPixels(x, wallStart, wallEnd, farCeilingScreenY,
				farFloorScreenY, farZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(wallData.sideID), shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
				farFloorScreenY, nearFloorScreenY, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.at(wallData.floorID), shadingInfo,
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

				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
					nearFloorScreenY, frame.height);
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearCeilingScreenY, frame.height);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
					nearFloorScreenY, frame.height);
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					nearCeilingScreenY, frame.height);
				const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
					farCeilingScreenY, frame.height);
				const int wallStart = ceilingEnd;
				const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);
				const int floorStart = wallEnd;
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearFloorScreenY, frame.height);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					nearCeilingScreenY, farCeilingScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, farCeilingScreenY,
					farFloorScreenY, farZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					farFloorScreenY, nearFloorScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
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
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				SoftwareRenderer::diagProjection(voxelYReal, voxelHeight, hit.point,
					camera.transform, camera.yShear, frame.height, frame.heightReal,
					diagTopScreenY, diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawPixels(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(diagData.id), shadingInfo, occlusion, frame);
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
				voxelX, voxelZ, edgeData.facing, nearPoint, farPoint, camera, ray, hit);

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

				const double edgeTopScreenY = SoftwareRenderer::getProjectedY(
					edgeTopPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double edgeBottomScreenY = SoftwareRenderer::getProjectedY(
					edgeBottomPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int edgeStart = SoftwareRenderer::getLowerBoundedPixel(
					edgeTopScreenY, frame.height);
				const int edgeEnd = SoftwareRenderer::getUpperBoundedPixel(
					edgeBottomScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, edgeStart, edgeEnd, edgeTopScreenY,
					edgeBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(edgeData.id), shadingInfo, occlusion, frame);
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

					return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
				}();

				const Double3 farNormal = -SoftwareRenderer::getNormal(farFacing);

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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int farStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int farEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, farStart, farEnd, farCeilingScreenY,
					farFloorScreenY, farZ, farU, 0.0, Constants::JustBelowOne, farNormal,
					textures.at(chasmData.id), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			// To do: find intersection via SoftwareRenderer::findDoorIntersection().
			// Render nothing for now.
		}
	};

	auto drawInitialVoxelBelow = [x, voxelX, voxelZ, &camera, &ray, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &voxelGrid,
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
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

			const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
				farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
				farCeilingScreenY, frame.height);
			const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearCeilingScreenY, frame.height);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
				farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.at(wallData.ceilingID),
				shadingInfo, occlusion, frame);
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

			const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
				farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
				farCeilingScreenY, frame.height);
			const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearCeilingScreenY, frame.height);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
				farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearCeilingScreenY, frame.height);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
					nearFloorScreenY, frame.height);
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					nearCeilingScreenY, frame.height);
				const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
					farCeilingScreenY, frame.height);
				const int wallStart = ceilingEnd;
				const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);
				const int floorStart = wallEnd;
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearFloorScreenY, frame.height);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					nearCeilingScreenY, farCeilingScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, farCeilingScreenY,
					farFloorScreenY, farZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					farFloorScreenY, nearFloorScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
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
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				SoftwareRenderer::diagProjection(voxelYReal, voxelHeight, hit.point,
					camera.transform, camera.yShear, frame.height, frame.heightReal,
					diagTopScreenY, diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawPixels(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(diagData.id), shadingInfo, occlusion, frame);
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
				voxelX, voxelZ, edgeData.facing, nearPoint, farPoint, camera, ray, hit);

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

				const double edgeTopScreenY = SoftwareRenderer::getProjectedY(
					edgeTopPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double edgeBottomScreenY = SoftwareRenderer::getProjectedY(
					edgeBottomPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int edgeStart = SoftwareRenderer::getLowerBoundedPixel(
					edgeTopScreenY, frame.height);
				const int edgeEnd = SoftwareRenderer::getUpperBoundedPixel(
					edgeBottomScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, edgeStart, edgeEnd, edgeTopScreenY,
					edgeBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(edgeData.id), shadingInfo, occlusion, frame);
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

					return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
				}();

				const Double3 farNormal = -SoftwareRenderer::getNormal(farFacing);

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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int farStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int farEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, farStart, farEnd, farCeilingScreenY,
					farFloorScreenY, farZ, farU, 0.0, Constants::JustBelowOne, farNormal,
					textures.at(chasmData.id), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			// To do: find intersection via SoftwareRenderer::findDoorIntersection().
			// Render nothing for now.
		}
	};

	auto drawInitialVoxelAbove = [x, voxelX, voxelZ, &camera, &ray, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &voxelGrid, 
		&textures, &occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
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

			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double farFloorScreenY = SoftwareRenderer::getProjectedY(
				farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
				nearFloorScreenY, frame.height);
			const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
				farFloorScreenY, frame.height);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
				nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.at(wallData.floorID),
				shadingInfo, occlusion, frame);
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

			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double farFloorScreenY = SoftwareRenderer::getProjectedY(
				farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
				nearFloorScreenY, frame.height);
			const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
				farFloorScreenY, frame.height);

			SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
				nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearCeilingScreenY, frame.height);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
					nearFloorScreenY, frame.height);
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					nearCeilingScreenY, frame.height);
				const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
					farCeilingScreenY, frame.height);
				const int wallStart = ceilingEnd;
				const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);
				const int floorStart = wallEnd;
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearFloorScreenY, frame.height);

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					nearCeilingScreenY, farCeilingScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, farCeilingScreenY,
					farFloorScreenY, farZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo,
					occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					farFloorScreenY, nearFloorScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
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
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				SoftwareRenderer::diagProjection(voxelYReal, voxelHeight, hit.point,
					camera.transform, camera.yShear, frame.height, frame.heightReal,
					diagTopScreenY, diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawPixels(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(diagData.id), shadingInfo, occlusion, frame);
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
				voxelX, voxelZ, edgeData.facing, nearPoint, farPoint, camera, ray, hit);

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

				const double edgeTopScreenY = SoftwareRenderer::getProjectedY(
					edgeTopPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double edgeBottomScreenY = SoftwareRenderer::getProjectedY(
					edgeBottomPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int edgeStart = SoftwareRenderer::getLowerBoundedPixel(
					edgeTopScreenY, frame.height);
				const int edgeEnd = SoftwareRenderer::getUpperBoundedPixel(
					edgeBottomScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, edgeStart, edgeEnd, edgeTopScreenY,
					edgeBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(edgeData.id), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Ignore. Chasms should never be above the player's voxel.
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			// To do: find intersection via SoftwareRenderer::findDoorIntersection().
			// Render nothing for now.
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
	const VoxelGrid &voxelGrid, const std::vector<VoxelTexture> &textures,
	OcclusionData &occlusion, const FrameView &frame)
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

		return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
	}();

	// Normal of the wall for the incoming ray, potentially shared between multiple voxels in
	// this voxel column.
	const Double3 wallNormal = SoftwareRenderer::getNormal(facing);

	auto drawVoxel = [x, voxelX, voxelZ, &camera, &ray, facing, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &voxelGrid, &textures,
		&occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
			
			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			SoftwareRenderer::drawPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(wallData.sideID), shadingInfo, occlusion, frame);
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

				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
					nearFloorScreenY, frame.height);
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int ceilingEnd = wallStart;

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo,
					occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = wallEnd;
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
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
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				SoftwareRenderer::diagProjection(voxelYReal, voxelHeight, hit.point,
					camera.transform, camera.yShear, frame.height, frame.heightReal,
					diagTopScreenY, diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawPixels(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne, 
					hit.normal, textures.at(diagData.id), shadingInfo, occlusion, frame);
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal, 
				textures.at(transparentWallData.id), shadingInfo, occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
				edgeData.facing, facing, nearPoint, farPoint, wallU, camera, ray, hit);

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

				const double edgeTopScreenY = SoftwareRenderer::getProjectedY(
					edgeTopPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double edgeBottomScreenY = SoftwareRenderer::getProjectedY(
					edgeBottomPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int edgeStart = SoftwareRenderer::getLowerBoundedPixel(
					edgeTopScreenY, frame.height);
				const int edgeEnd = SoftwareRenderer::getUpperBoundedPixel(
					edgeBottomScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, edgeStart, edgeEnd, edgeTopScreenY,
					edgeBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(edgeData.id), shadingInfo, occlusion, frame);
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

				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int nearStart = SoftwareRenderer::getLowerBoundedPixel(
					nearCeilingScreenY, frame.height);
				const int nearEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearFloorScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, nearStart, nearEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, nearU, 0.0, Constants::JustBelowOne, nearNormal,
					textures.at(chasmData.id), shadingInfo, occlusion, frame);
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

					return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
				}();

				const Double3 farNormal = -SoftwareRenderer::getNormal(farFacing);

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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int farStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int farEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, farStart, farEnd, farCeilingScreenY,
					farFloorScreenY, farZ, farU, 0.0, Constants::JustBelowOne, farNormal,
					textures.at(chasmData.id), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			// To do: find intersection via SoftwareRenderer::findDoorIntersection().

			// Just render as transparent wall for now.
			const VoxelData::DoorData &doorData = voxelData.door;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(doorData.id), shadingInfo, occlusion, frame);
		}
	};

	auto drawVoxelBelow = [x, voxelX, voxelZ, &camera, &ray, facing, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &voxelGrid, &textures,
		&occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
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

			const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
				farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
				farCeilingScreenY, frame.height);
			const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallStart = ceilingEnd;
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
				farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.at(wallData.ceilingID),
				shadingInfo, occlusion, frame);

			// Side.
			SoftwareRenderer::drawPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(wallData.sideID), shadingInfo, occlusion, frame);
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

			const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
				farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
				farCeilingScreenY, frame.height);
			const int ceilingEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearCeilingScreenY, frame.height);

			SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
				farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int ceilingEnd = wallStart;

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, 
					occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = wallEnd;
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, occlusion, frame);
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
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				SoftwareRenderer::diagProjection(voxelYReal, voxelHeight, hit.point,
					camera.transform, camera.yShear, frame.height, frame.heightReal,
					diagTopScreenY, diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawPixels(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(diagData.id), shadingInfo, occlusion, frame);
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(transparentWallData.id), shadingInfo, occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
				edgeData.facing, facing, nearPoint, farPoint, wallU, camera, ray, hit);

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

				const double edgeTopScreenY = SoftwareRenderer::getProjectedY(
					edgeTopPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double edgeBottomScreenY = SoftwareRenderer::getProjectedY(
					edgeBottomPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int edgeStart = SoftwareRenderer::getLowerBoundedPixel(
					edgeTopScreenY, frame.height);
				const int edgeEnd = SoftwareRenderer::getUpperBoundedPixel(
					edgeBottomScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, edgeStart, edgeEnd, edgeTopScreenY,
					edgeBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(edgeData.id), shadingInfo, occlusion, frame);
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

				const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
					nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
					nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int nearStart = SoftwareRenderer::getLowerBoundedPixel(
					nearCeilingScreenY, frame.height);
				const int nearEnd = SoftwareRenderer::getUpperBoundedPixel(
					nearFloorScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, nearStart, nearEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, nearU, 0.0, Constants::JustBelowOne, nearNormal,
					textures.at(chasmData.id), shadingInfo, occlusion, frame);
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

					return std::max(std::min(uVal, Constants::JustBelowOne), 0.0);
				}();

				const Double3 farNormal = -SoftwareRenderer::getNormal(farFacing);

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

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int farStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int farEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, farStart, farEnd, farCeilingScreenY,
					farFloorScreenY, farZ, farU, 0.0, Constants::JustBelowOne, farNormal, 
					textures.at(chasmData.id), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			// To do: find intersection via SoftwareRenderer::findDoorIntersection().

			// Just render as transparent wall for now.
			const VoxelData::DoorData &doorData = voxelData.door;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(doorData.id), shadingInfo, occlusion, frame);
		}
	};

	auto drawVoxelAbove = [x, voxelX, voxelZ, &camera, &ray, facing, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, wallU, &shadingInfo, ceilingHeight, &voxelGrid, &textures,
		&occlusion, &frame](int voxelY)
	{
		const uint16_t voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double farFloorScreenY = SoftwareRenderer::getProjectedY(
				farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);
			const int floorStart = wallEnd;
			const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
				farFloorScreenY, frame.height);
			
			// Side.
			SoftwareRenderer::drawPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(wallData.sideID), shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
				nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.at(wallData.floorID),
				shadingInfo, occlusion, frame);
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

			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double farFloorScreenY = SoftwareRenderer::getProjectedY(
				farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int floorStart = SoftwareRenderer::getLowerBoundedPixel(
				nearFloorScreenY, frame.height);
			const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
				farFloorScreenY, frame.height);

			SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
				nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			// Draw order depends on the player's Y position relative to the platform.
			if (camera.eye.y > nearCeilingPoint.y)
			{
				// Above platform.
				const Double3 farCeilingPoint(
					farPoint.x,
					nearCeilingPoint.y,
					farPoint.y);

				const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
					farCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int ceilingStart = SoftwareRenderer::getLowerBoundedPixel(
					farCeilingScreenY, frame.height);
				const int ceilingEnd = wallStart;

				// Ceiling.
				SoftwareRenderer::drawPerspectivePixels(x, ceilingStart, ceilingEnd,
					farCeilingScreenY, nearCeilingScreenY, farPoint, nearPoint, farZ,
					nearZ, Double3::UnitY, textures.at(raisedData.ceilingID),
					shadingInfo, occlusion, frame);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, 
					occlusion, frame);
			}
			else if (camera.eye.y < nearFloorPoint.y)
			{
				// Below platform.
				const Double3 farFloorPoint(
					farPoint.x,
					nearFloorPoint.y,
					farPoint.y);

				const double farFloorScreenY = SoftwareRenderer::getProjectedY(
					farFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int floorStart = wallEnd;
				const int floorEnd = SoftwareRenderer::getUpperBoundedPixel(
					farFloorScreenY, frame.height);

				// Side.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo, occlusion, frame);

				// Floor.
				SoftwareRenderer::drawPerspectivePixels(x, floorStart, floorEnd,
					nearFloorScreenY, farFloorScreenY, nearPoint, farPoint, nearZ,
					farZ, -Double3::UnitY, textures.at(raisedData.floorID),
					shadingInfo, occlusion, frame);
			}
			else
			{
				// Between top and bottom.
				SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, wallU, raisedData.vTop, raisedData.vBottom,
					wallNormal, textures.at(raisedData.sideID), shadingInfo,
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
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				SoftwareRenderer::diagProjection(voxelYReal, voxelHeight, hit.point,
					camera.transform, camera.yShear, frame.height, frame.heightReal,
					diagTopScreenY, diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawPixels(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(diagData.id), shadingInfo, occlusion, frame);
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

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(transparentWallData.id), shadingInfo, occlusion, frame);
		}
		else if (voxelData.dataType == VoxelDataType::Edge)
		{
			const VoxelData::EdgeData &edgeData = voxelData.edge;

			// Find intersection.
			RayHit hit;
			const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
				edgeData.facing, facing, nearPoint, farPoint, wallU, camera, ray, hit);

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

				const double edgeTopScreenY = SoftwareRenderer::getProjectedY(
					edgeTopPoint, camera.transform, camera.yShear) * frame.heightReal;
				const double edgeBottomScreenY = SoftwareRenderer::getProjectedY(
					edgeBottomPoint, camera.transform, camera.yShear) * frame.heightReal;

				const int edgeStart = SoftwareRenderer::getLowerBoundedPixel(
					edgeTopScreenY, frame.height);
				const int edgeEnd = SoftwareRenderer::getUpperBoundedPixel(
					edgeBottomScreenY, frame.height);

				SoftwareRenderer::drawTransparentPixels(x, edgeStart, edgeEnd, edgeTopScreenY,
					edgeBottomScreenY, nearZ + hit.innerZ, hit.u, 0.0, Constants::JustBelowOne,
					hit.normal, textures.at(edgeData.id), shadingInfo, occlusion, frame);
			}
		}
		else if (voxelData.dataType == VoxelDataType::Chasm)
		{
			// Ignore. Chasms should never be above the player's voxel.
		}
		else if (voxelData.dataType == VoxelDataType::Door)
		{
			// To do: find intersection via SoftwareRenderer::findDoorIntersection().

			// Just render as transparent wall for now.
			const VoxelData::DoorData &doorData = voxelData.door;

			const Double3 nearCeilingPoint(
				nearPoint.x,
				voxelYReal + voxelHeight,
				nearPoint.y);
			const Double3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);

			const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
				nearCeilingPoint, camera.transform, camera.yShear) * frame.heightReal;
			const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
				nearFloorPoint, camera.transform, camera.yShear) * frame.heightReal;

			const int wallStart = SoftwareRenderer::getLowerBoundedPixel(
				nearCeilingScreenY, frame.height);
			const int wallEnd = SoftwareRenderer::getUpperBoundedPixel(
				nearFloorScreenY, frame.height);

			SoftwareRenderer::drawTransparentPixels(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, wallU, 0.0, Constants::JustBelowOne, wallNormal,
				textures.at(doorData.id), shadingInfo, occlusion, frame);
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
	const double clampedStartXPercent = std::max(startXPercent,
		std::min(flatFrame.startX, flatFrame.endX));
	const double clampedEndXPercent = std::min(endXPercent,
		std::max(flatFrame.startX, flatFrame.endX));

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
	const double startU = std::max(std::min(startFlatPercent, Constants::JustBelowOne), 0.0);
	const double endU = std::max(std::min(endFlatPercent, Constants::JustBelowOne), 0.0);

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
	// - To do: contribution from lights.
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
		const Double3 &fogColor = shadingInfo.horizonSkyColor;
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
	const ShadingInfo &shadingInfo, double ceilingHeight, const VoxelGrid &voxelGrid,
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

	// Pointer to voxel ID grid data.
	const uint16_t *voxels = voxelGrid.getVoxels();

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
			zDistance, shadingInfo, ceilingHeight, voxelGrid, textures, occlusion, frame);
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
			voxelGrid, textures, occlusion, frame);
	}
}

void SoftwareRenderer::render(const Double3 &eye, const Double3 &direction, double fovY,
	double ambient, double daytimePercent, double ceilingHeight, const VoxelGrid &voxelGrid, 
	uint32_t *colorBuffer)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;
	const double projectionModifier = 1.20; // To account for tall pixels.

	// 2.5D camera definition.
	const Camera camera(eye, direction, fovY, aspect, projectionModifier);

	// Camera values for generating 2D rays with.
	const Double2 forwardComp = 
		Double2(camera.forwardX, camera.forwardZ).normalized() * camera.zoom;
	const Double2 right2D = 
		Double2(camera.rightX, camera.rightZ).normalized() * camera.aspect;

	// Calculate shading information.
	const Double3 horizonFogColor = this->getFogColor(daytimePercent);
	const Double3 zenithFogColor = horizonFogColor * 0.85; // Temp.
	const Double3 sunDirection = this->getSunDirection(daytimePercent);
	const Double3 sunColor = [&sunDirection]()
	{
		const Double3 baseColor(0.90, 0.875, 0.85);

		// Darken the sun color if it's below the horizon so wall faces aren't lit 
		// as much during the night. This is just an artistic value to compensate
		// for the lack of shadows.
		return (sunDirection.y >= 0.0) ? baseColor :
			(baseColor * (1.0 - (5.0 * std::abs(sunDirection.y)))).clamped();
	}();

	// Create some helper structs to keep similar values together.
	const ShadingInfo shadingInfo(horizonFogColor, zenithFogColor, sunColor, 
		sunDirection, ambient, this->fogDistance);
	const FrameView frame(colorBuffer, this->depthBuffer.data(), this->width, this->height);

	// Lambda for rendering some columns of pixels. The voxel rendering portion uses 2.5D 
	// ray casting, which is the cheaper form of ray casting (although still not very 
	// efficient overall), and results in a "fake" 3D scene.
	auto renderColumns = [this, &camera, ceilingHeight, &voxelGrid, &shadingInfo,
		widthReal, &forwardComp, &right2D, &frame](int startX, int endX)
	{
		for (int x = startX; x < endX; x++)
		{
			// X percent across the screen.
			const double xPercent = (static_cast<double>(x) + 0.50) / widthReal;

			// "Right" component of the ray direction, based on current screen X.
			const Double2 rightComp = right2D * ((2.0 * xPercent) - 1.0);

			// Calculate the ray direction through the pixel.
			// - If un-normalized, it uses the Z distance, but the insides of voxels
			//   don't look right then.
			const Double2 direction = (forwardComp + rightComp).normalized();
			const Ray ray(direction.x, direction.y);

			// Cast the 2D ray and fill in the column's pixels with color.
			this->rayCast2D(x, camera, ray, shadingInfo, ceilingHeight, 
				voxelGrid, this->voxelTextures, this->occlusion.at(x), frame);
		}

		// Iterate through all flats, rendering those visible within the given X range of 
		// the screen.
		for (const auto &pair : this->visibleFlats)
		{
			const Flat &flat = *pair.first;
			const Flat::Frame &flatFrame = pair.second;

			// Normal of the flat (always facing the camera).
			const Double3 flatNormal = Double3(-forwardComp.x, 0.0, -forwardComp.y).normalized();

			// Texture of the flat. It might be flipped horizontally as well, given by
			// the "flat.flipped" value.
			const FlatTexture &texture = this->flatTextures[flat.textureID];

			const Double2 eye2D(camera.eye.x, camera.eye.z);

			SoftwareRenderer::drawFlat(startX, endX, flatFrame, flatNormal, flat.flipped, 
				eye2D, shadingInfo, texture, frame);
		}
	};

	// Lambda for clearing some rows on the frame buffer quickly.
	auto clearRows = [this, colorBuffer, &horizonFogColor, &zenithFogColor](int startY, int endY)
	{
		const int startIndex = startY * this->width;
		const int endIndex = endY * this->width;

		uint32_t *colorPtr = colorBuffer;
		double *depthPtr = this->depthBuffer.data();

		const uint32_t colorValue = horizonFogColor.toRGB();
		const double depthValue = std::numeric_limits<double>::infinity();

		// Clear the color and depth of some rows.
		for (int i = startIndex; i < endIndex; i++)
		{
			colorPtr[i] = colorValue;
			depthPtr[i] = depthValue;
		}
	};

	// Start a thread for refreshing the visible flats. This should erase the old list,
	// calculate a new list, and sort it by depth.
	std::thread sortThread([this, &camera] { this->updateVisibleFlats(camera); });

	// Prepare render threads, obtaining the number of threads to use from the render threads
	// mode. These are used for clearing the frame buffer and rendering.
	std::vector<std::thread> renderThreads(
		SoftwareRenderer::getRenderThreadsFromMode(this->renderThreadsMode));

	// Start clearing the frame buffer with the render threads.
	for (size_t i = 0; i < renderThreads.size(); i++)
	{
		// "blockSize" is the approximate number of rows per thread. Rounding is involved so 
		// the start and stop coordinates are correct for all resolutions.
		const double blockSize = heightReal / static_cast<double>(renderThreads.size());
		const int startY = static_cast<int>(std::round(static_cast<double>(i) * blockSize));
		const int endY = static_cast<int>(std::round(static_cast<double>(i + 1) * blockSize));

		// Make sure the rounding is correct.
		assert(startY >= 0);
		assert(endY <= this->height);

		renderThreads[i] = std::thread(clearRows, startY, endY);
	}

	// Reset occlusion.
	std::fill(this->occlusion.begin(), this->occlusion.end(), 
		OcclusionData(0, this->height));

	// Wait for the render threads to finish clearing.
	for (auto &thread : renderThreads)
	{
		thread.join();
	}

	// Wait for the sorting thread to finish.
	sortThread.join();

	// Start rendering the scene with the render threads.
	for (size_t i = 0; i < renderThreads.size(); i++)
	{
		// "blockSize" is the approximate number of columns per thread. Rounding is involved so 
		// the start and stop coordinates are correct for all resolutions.
		const double blockSize = widthReal / static_cast<double>(renderThreads.size());
		const int startX = static_cast<int>(std::round(static_cast<double>(i) * blockSize));
		const int endX = static_cast<int>(std::round(static_cast<double>(i + 1) * blockSize));

		// Make sure the rounding is correct.
		assert(startX >= 0);
		assert(endX <= this->width);

		renderThreads[i] = std::thread(renderColumns, startX, endX);
	}

	// Wait for the render threads to finish rendering.
	for (auto &thread : renderThreads)
	{
		thread.join();
	}
}
