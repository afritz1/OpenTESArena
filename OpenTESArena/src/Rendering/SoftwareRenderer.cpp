#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <thread>

#include "SoftwareRenderer.h"

#include "../Math/Constants.h"
#include "../Utilities/Debug.h"
#include "../World/VoxelData.h"
#include "../World/VoxelGrid.h"

SoftwareRenderer::ShadingInfo::ShadingInfo(const Double3 &horizonFogColor, const Double3 &zenithFogColor,
	const Double3 &sunColor, const Double3 &sunDirection, double ambient)
	: horizonFogColor(horizonFogColor), zenithFogColor(zenithFogColor),
	sunColor(sunColor), sunDirection(sunDirection)
{
	this->ambient = ambient;
}

const double SoftwareRenderer::NEAR_PLANE = 0.0001;
const double SoftwareRenderer::FAR_PLANE = 1000.0;

SoftwareRenderer::SoftwareRenderer(int width, int height)
{
	// Initialize 2D frame buffer.
	const int pixelCount = width * height;
	this->zBuffer = std::vector<double>(pixelCount);
	std::fill(this->zBuffer.begin(), this->zBuffer.end(), 0);

	this->width = width;
	this->height = height;

	// Obtain the number of threads to use. "hardware_concurrency()" might return 0,
	// so it needs to be clamped positive.
	this->renderThreadCount = static_cast<int>(std::thread::hardware_concurrency());
	if (this->renderThreadCount == 0)
	{
		DebugMention("hardware_concurrency() returned 0.");
		this->renderThreadCount = 1;
	}

	// Fog distance is zero by default.
	this->fogDistance = 0.0;
}

SoftwareRenderer::~SoftwareRenderer()
{

}

void SoftwareRenderer::addFlat(int id, const Double3 &position, const Double2 &direction,
	double width, double height, int textureID)
{
	// Verify that the ID is not already in use.
	DebugAssert(this->flats.find(id) == this->flats.end(), 
		"Flat ID \"" + std::to_string(id) + "\" already taken.");

	SoftwareRenderer::Flat flat;
	flat.position = position;
	flat.direction = direction;
	flat.width = width;
	flat.height = height;
	flat.textureID = textureID;
	flat.flipped = false; // The initial value doesn't matter, it's updated frequently.

	// Add the flat (sprite, door, store sign, etc.).
	this->flats.insert(std::make_pair(id, flat));
}

void SoftwareRenderer::addLight(int id, const Double3 &point, const Double3 &color, 
	double intensity)
{
	DebugNotImplemented();
}

int SoftwareRenderer::addTexture(const uint32_t *pixels, int width, int height)
{
	const int pixelCount = width * height;

	TextureData texture;
	texture.pixels = std::vector<Double4>(pixelCount);
	texture.width = width;
	texture.height = height;

	// Convert ARGB color from integer to double-precision format for speed.
	// This does waste an extreme amount of memory (32 bytes per pixel!), but
	// it's not a big deal for Arena's textures (mostly 64x64, so eight textures
	// would be a megabyte).
	Double4 *texturePixels = texture.pixels.data();
	for (int i = 0; i < pixelCount; ++i)
	{
		texturePixels[i] = Double4::fromARGB(pixels[i]);
	}

	this->textures.push_back(std::move(texture));

	return static_cast<int>(this->textures.size() - 1);
}

void SoftwareRenderer::updateFlat(int id, const Double3 *position, const Double2 *direction,
	const double *width, const double *height, const int *textureID, const bool *flipped)
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

	if (direction != nullptr)
	{
		flat.direction = *direction;
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

	for (size_t i = 0; i < this->skyPalette.size(); ++i)
	{
		this->skyPalette[i] = Double3::fromRGB(colors[i]);
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

void SoftwareRenderer::resize(int width, int height)
{
	const int pixelCount = width * height;
	this->zBuffer.resize(pixelCount);
	std::fill(this->zBuffer.begin(), this->zBuffer.end(), 0);

	this->width = width;
	this->height = height;
}

void SoftwareRenderer::updateVisibleFlats(const Double3 &eye, double yShear,
	const Matrix4d &transform)
{
	this->visibleFlats.clear();

	// Used with calculating distances in the XZ plane.
	const Double2 eye2D(eye.x, eye.z);

	// This is essentially a visible sprite determination algorithm mixed with a 
	// trimmed-down vertex shader. It goes through all the flats and sees if they 
	// would be at least partially visible in the view frustum each frame.
	for (const auto &pair : this->flats)
	{
		const Flat &flat = pair.second;

		// Get the flat's axes. UnitY is "global up".
		const Double3 flatForward = Double3(flat.direction.x, 0.0, flat.direction.y).normalized();
		const Double3 flatUp = Double3::UnitY;
		const Double3 flatRight = flatForward.cross(flatUp).normalized();

		const Double3 flatRightScaled = flatRight * (flat.width * 0.50);
		const Double3 flatUpScaled = flatUp * flat.height;

		// Calculate just the bottom two corners of the flat in world space.
		// Line clipping needs to be done first before calculating the other corners.
		Double3 bottomLeft = flat.position - flatRightScaled;
		Double3 bottomRight = flat.position + flatRightScaled;

		// Transform the two points to camera space (projection * view).
		Double4 blPoint = transform * Double4(bottomLeft.x, bottomLeft.y, bottomLeft.z, 1.0);
		Double4 brPoint = transform * Double4(bottomRight.x, bottomRight.y, bottomRight.z, 1.0);

		// Check if the line segment needs clipping.
		const bool leftCloseEnough = blPoint.z < SoftwareRenderer::FAR_PLANE;
		const bool leftFarEnough = blPoint.z > SoftwareRenderer::NEAR_PLANE;
		const bool rightCloseEnough = brPoint.z < SoftwareRenderer::FAR_PLANE;
		const bool rightFarEnough = brPoint.z > SoftwareRenderer::NEAR_PLANE;

		const bool leftBounded = leftCloseEnough && leftFarEnough;
		const bool rightBounded = rightCloseEnough && rightFarEnough;

		// If both points are outside the clipping planes, the flat is not visible.
		if (!leftBounded && !rightBounded)
		{
			continue;
		}

		// Horizontal texture coordinates (modified by line clipping if needed).
		double leftU = 1.0;
		double rightU = 0.0;

		// Clip line if needed by interpolating between the bad point and good point.
		if (!leftFarEnough)
		{
			const double percent = (SoftwareRenderer::NEAR_PLANE - blPoint.z) /
				(brPoint.z - blPoint.z);

			bottomLeft = bottomLeft.lerp(bottomRight, percent);
			blPoint = transform * Double4(bottomLeft.x, bottomLeft.y, bottomLeft.z, 1.0);
			leftU = 1.0 - percent;
		}
		else if (!rightFarEnough)
		{
			const double percent = (SoftwareRenderer::NEAR_PLANE - brPoint.z) /
				(blPoint.z - brPoint.z);

			bottomRight = bottomRight.lerp(bottomLeft, percent);
			brPoint = transform * Double4(bottomRight.x, bottomRight.y, bottomRight.z, 1.0);
			rightU = percent;
		}

		// Generate the other two corners.
		const Double3 topLeft = bottomLeft + flatUpScaled;
		const Double3 topRight = bottomRight + flatUpScaled;

		Double4 tlPoint = transform * Double4(topLeft.x, topLeft.y, topLeft.z, 1.0);
		Double4 trPoint = transform * Double4(topRight.x, topRight.y, topRight.z, 1.0);

		// Normalize coordinates.
		blPoint = blPoint / blPoint.w;
		brPoint = brPoint / brPoint.w;
		tlPoint = tlPoint / tlPoint.w;
		trPoint = trPoint / trPoint.w;

		// Create projection data for the flat.
		Flat::Projection flatProjection;

		// Translate coordinates on the screen relative to the middle (0.5, 0.5). Multiply 
		// by 0.5 to apply the correct aspect ratio. Calculate true distances from the camera 
		// to the left and right edges in the XZ plane.
		flatProjection.left.x = 0.50 + (blPoint.x * 0.50);
		flatProjection.left.topY = (0.50 + yShear) - (tlPoint.y * 0.50);
		flatProjection.left.bottomY = (0.50 + yShear) - (blPoint.y * 0.50);
		flatProjection.left.z = (Double2(bottomLeft.x, bottomLeft.z) - eye2D).length();
		flatProjection.left.u = leftU;

		flatProjection.right.x = 0.50 + (brPoint.x * 0.50);
		flatProjection.right.topY = (0.50 + yShear) - (trPoint.y * 0.50);
		flatProjection.right.bottomY = (0.50 + yShear) - (brPoint.y * 0.50);
		flatProjection.right.z = (Double2(bottomRight.x, bottomRight.z) - eye2D).length();
		flatProjection.right.u = rightU;

		this->visibleFlats.push_back(std::make_pair(&flat, flatProjection));
	}

	// Sort the visible flat data farthest to nearest (this may be relevant for
	// transparencies).
	std::sort(this->visibleFlats.begin(), this->visibleFlats.end(),
		[](const std::pair<const Flat*, Flat::Projection> &a,
			const std::pair<const Flat*, Flat::Projection> &b)
	{
		return std::min(a.second.left.z, a.second.right.z) >
			std::min(b.second.left.z, b.second.right.z);
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
	const double radians = daytimePercent * (2.0 * PI);
	return Double3(0.0, -std::cos(radians), std::sin(radians)).normalized();
}

void SoftwareRenderer::castColumnRay(int x, const Double3 &eye, const Double2 &direction, 
	const Matrix4d &transform, double yShear, double daytimePercent,
	const Double3 &fogColor, const Double3 &sunDirection, const VoxelGrid &voxelGrid, 
	uint32_t *colorBuffer)
{
	// Initially based on Lode Vandevenne's algorithm, this method of rendering is more 
	// expensive than cheap 2.5D ray casting, as it does not stop at the first wall 
	// intersection, and it also renders walls above and below, but it is more correct 
	// as a result. Assume "direction" is normalized.

	// Some floating point behavior assumptions:
	// -> (value / 0.0) == infinity
	// -> (value / infinity) == 0.0
	// -> (int)(-0.8) == 0
	// -> (int)floor(-0.8) == -1
	// -> (int)ceil(-0.8) == 0

	// Lambda for drawing a column of wall pixels.
	// - 'x' is the screen column.
	// - 'y1' and 'y2' are projected Y coordinates.
	// - 'z' is the column's Z depth.
	// - 'u' is the U texture coordinate between 0 and 1.
	// - 'topV' and 'bottomV' are the start and end V texture coordinates.
	auto drawWallColumn = [](int x, double y1, double y2, double z, double u,
		double topV, double bottomV, const TextureData &texture, double fogDistance, 
		const Double3 &fogColor, int frameWidth, int frameHeight, double *depthBuffer, 
		uint32_t *output)
	{
		// Get the start and end Y pixel coordinates of the projected points (potentially
		// outside the top or bottom of the screen).
		const double heightReal = static_cast<double>(frameHeight);
		const int projectedStart = static_cast<int>(std::round(y1 * heightReal));
		const int projectedEnd = static_cast<int>(std::round(y2 * heightReal));

		// Clamp the Y coordinates for where the wall starts and stops on the screen.
		const int drawStart = std::max(0, projectedStart);
		const int drawEnd = std::min(frameHeight, projectedEnd);

		// Horizontal offset in texture.
		const int textureX = static_cast<int>(u * 
			static_cast<double>(texture.width)) % texture.width;

		// Linearly interpolated fog.
		const double fogPercent = std::min(z / fogDistance, 1.0);

		// Draw the column to the output buffer.
		for (int y = drawStart; y < drawEnd; ++y)
		{
			const int index = x + (y * frameWidth);

			// Check depth of the pixel. A bias of epsilon is added to reduce artifacts from
			// adjacent walls sharing the same Z value. More strict drawing rules (voxel
			// stepping order?) might be a better fix for this.
			if (z <= (depthBuffer[index] - EPSILON))
			{
				// Percent stepped from beginning to end on the column.
				const double yPercent = (static_cast<double>(y - projectedStart) + 0.50) /
					static_cast<double>(projectedEnd - projectedStart);

				// Vertical texture coordinate.
				const double v = topV + ((bottomV - topV) * yPercent);

				// Y position in texture.
				const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

				const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

				// Draw only if the texel is not transparent.
				if (texel.w > 0.0)
				{
					const Double3 color(texel.x, texel.y, texel.z);

					output[index] = color.lerp(fogColor, fogPercent).clamped().toRGB();
					depthBuffer[index] = z;
				}
			}
		}
	};
	
	// Lambda for drawing a column of floor pixels (not in the first voxel).
	// Right now, it renders near to far. Floors and ceilings can't be rendered in the 
	// same loop because they have different projected properties (like size on screen) 
	// due to the perspective transformation not preserving parallel lines.
	// - 'x' is the screen column.
	// - 'y' is the height of the floor.
	// - 'nearPoint' and 'farPoint' are XZ world points that share 'y'.
	auto drawFloorColumn = [](int x, double y, const Double2 &nearPoint,
		const Double2 &farPoint, const Double2 &eye, const Matrix4d &transform,
		double yShear, const TextureData &texture, double fogDistance,
		const Double3 &fogColor, int frameWidth, int frameHeight, double *depthBuffer,
		uint32_t *output)
	{
		// Transform the floor points to camera space (projection * view).
		// - Note that the near and far points are switched for floor rendering (for now).
		Double4 p1 = transform * Double4(nearPoint.x, y, nearPoint.y, 1.0);
		Double4 p2 = transform * Double4(farPoint.x, y, farPoint.y, 1.0);

		// Convert to normalized coordinates.
		p1 = p1 / p1.w;
		p2 = p2 / p2.w;

		// Translate the Y coordinates relative to the center of Y projection (y == 0.5).
		// Add Y-shearing for "fake" looking up and down. Multiply by 0.5 to apply the 
		// correct aspect ratio.
		// - Since the ray cast guarantees the intersection to be in the correct column
		//   of the screen, only the Y coordinates need to be projected.
		const double projectedY1 = (0.50 + yShear) - (p1.y * 0.50);
		const double projectedY2 = (0.50 + yShear) - (p2.y * 0.50);

		// Get the start and end Y pixel coordinates of the projected points (potentially
		// outside the top or bottom of the screen).
		const double heightReal = static_cast<double>(frameHeight);
		const int projectedStart = static_cast<int>(std::round(projectedY1 * heightReal));
		const int projectedEnd = static_cast<int>(std::round(projectedY2 * heightReal));

		// Clamp the Y coordinates for where the floor starts and stops on the screen.
		const int drawStart = std::max(0, projectedStart);
		const int drawEnd = std::min(frameHeight, projectedEnd);

		// True distance to the near and far points.
		const double nearZ = (nearPoint - eye).length();
		const double farZ = (farPoint - eye).length();

		// Values for perspective-correct interpolation.
		const double nearZRecip = 1.0 / nearZ;
		const double farZRecip = 1.0 / farZ;
		const Double2 nearPointDiv = nearPoint * nearZRecip;
		const Double2 farPointDiv = farPoint * farZRecip;

		// Draw the column to the output buffer.
		for (int y = drawStart; y < drawEnd; ++y)
		{
			const int index = x + (y * frameWidth);

			// Percent stepped from beginning to end on the column.
			const double yPercent = (static_cast<double>(y - projectedStart) + 0.50) /
				static_cast<double>(projectedEnd - projectedStart);

			// Interpolate between the near and far point.
			const Double2 currentPoint =
				(nearPointDiv + ((farPointDiv - nearPointDiv) * yPercent)) /
				(nearZRecip + ((farZRecip - nearZRecip) * yPercent));
			const double z = nearZ + ((farZ - nearZ) * yPercent);

			// Linearly interpolated fog.
			const double fogPercent = std::min(z / fogDistance, 1.0);

			if (z <= depthBuffer[index])
			{
				// Horizontal texture coordinate.
				const double u = currentPoint.y - std::floor(currentPoint.y);

				// Horizontal offset in texture.
				const int textureX = static_cast<int>(u *
					static_cast<double>(texture.width)) % texture.width;

				// Vertical texture coordinate.
				const double v = 1.0 - (currentPoint.x - std::floor(currentPoint.x));

				// Y position in texture.
				const int textureY = static_cast<int>(v *
					static_cast<double>(texture.height)) % texture.height;

				const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

				// Draw only if the texel is not transparent.
				if (texel.w > 0.0)
				{
					const Double3 color(texel.x, texel.y, texel.z);

					output[index] = color.lerp(fogColor, fogPercent).clamped().toRGB();
					depthBuffer[index] = z;
				}
			}
		}
	};

	// Lambda for drawing a column of ceiling pixels (not in the first voxel).
	// Right now, it renders far to near.
	// - 'x' is the screen column.
	// - 'y' is the height of the floor.
	// - 'nearPoint' and 'farPoint' are XZ world points that share 'y'.
	auto drawCeilingColumn = [](int x, double y, const Double2 &nearPoint,
		const Double2 &farPoint, const Double2 &eye, const Matrix4d &transform, 
		double yShear, const TextureData &texture, double fogDistance,
		const Double3 &fogColor, int frameWidth, int frameHeight, double *depthBuffer, 
		uint32_t *output)
	{
		// Transform the floor points to camera space (projection * view).
		Double4 p1 = transform * Double4(farPoint.x, y, farPoint.y, 1.0);
		Double4 p2 = transform * Double4(nearPoint.x, y, nearPoint.y, 1.0);

		// Convert to normalized coordinates.
		p1 = p1 / p1.w;
		p2 = p2 / p2.w;

		// Translate the Y coordinates relative to the center of Y projection (y == 0.5).
		// Add Y-shearing for "fake" looking up and down. Multiply by 0.5 to apply the 
		// correct aspect ratio.
		// - Since the ray cast guarantees the intersection to be in the correct column
		//   of the screen, only the Y coordinates need to be projected.
		const double projectedY1 = (0.50 + yShear) - (p1.y * 0.50);
		const double projectedY2 = (0.50 + yShear) - (p2.y * 0.50);

		// Get the start and end Y pixel coordinates of the projected points (potentially
		// outside the top or bottom of the screen).
		const double heightReal = static_cast<double>(frameHeight);
		const int projectedStart = static_cast<int>(std::round(projectedY1 * heightReal));
		const int projectedEnd = static_cast<int>(std::round(projectedY2 * heightReal));

		// Clamp the Y coordinates for where the floor starts and stops on the screen.
		const int drawStart = std::max(0, projectedStart);
		const int drawEnd = std::min(frameHeight, projectedEnd);

		// True distance to the near and far points.
		const double nearZ = (nearPoint - eye).length();
		const double farZ = (farPoint - eye).length();

		// Values for perspective-correct interpolation.
		const double nearZRecip = 1.0 / nearZ;
		const double farZRecip = 1.0 / farZ;
		const Double2 nearPointDiv = nearPoint * nearZRecip;
		const Double2 farPointDiv = farPoint * farZRecip;
		
		// Draw the column to the output buffer.
		for (int y = drawStart; y < drawEnd; ++y)
		{
			const int index = x + (y * frameWidth);

			// Percent stepped from beginning to end on the column.
			const double yPercent = (static_cast<double>(y - projectedStart) + 0.50) /
				static_cast<double>(projectedEnd - projectedStart);

			// Interpolate between the near and far point.
			const Double2 currentPoint =
				(farPointDiv + ((nearPointDiv - farPointDiv) * yPercent)) /
				(farZRecip + ((nearZRecip - farZRecip) * yPercent));
			const double z = farZ + ((nearZ - farZ) * yPercent);

			// Linearly interpolated fog.
			const double fogPercent = std::min(z / fogDistance, 1.0);

			if (z <= depthBuffer[index])
			{
				// Horizontal texture coordinate.
				const double u = currentPoint.y - std::floor(currentPoint.y);

				// Horizontal offset in texture.
				const int textureX = static_cast<int>(u *
					static_cast<double>(texture.width)) % texture.width;

				// Vertical texture coordinate.
				const double v = 1.0 - (currentPoint.x - std::floor(currentPoint.x));

				// Y position in texture.
				const int textureY = static_cast<int>(v * 
					static_cast<double>(texture.height)) % texture.height;

				const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

				// Draw only if the texel is not transparent.
				if (texel.w > 0.0)
				{
					const Double3 color(texel.x, texel.y, texel.z);

					output[index] = color.lerp(fogColor, fogPercent).clamped().toRGB();
					depthBuffer[index] = z;
				}
			}
		}
	};

	// Because 2D vectors use "X" and "Y", the Z component is actually
	// aliased as "Y", which is a minor annoyance.
	const double dirX = direction.x;
	const double dirZ = direction.y;
	const double dirXSquared = direction.x * direction.x;
	const double dirZSquared = direction.y * direction.y;

	const double deltaDistX = std::sqrt(1.0 + (dirZSquared / dirXSquared));
	const double deltaDistZ = std::sqrt(1.0 + (dirXSquared / dirZSquared));

	const bool nonNegativeDirX = direction.x >= 0.0;
	const bool nonNegativeDirZ = direction.y >= 0.0;

	// Constant DDA-related values.
	// - Technically, these could be moved out of the ray casting method, but they
	//   are not a performance concern for now. It's just to minimize renderer state.
	const Double3 startCellReal(
		std::floor(eye.x),
		std::floor(eye.y),
		std::floor(eye.z));
	const Int3 startCell(
		static_cast<int>(startCellReal.x),
		static_cast<int>(startCellReal.y),
		static_cast<int>(startCellReal.z));

	int stepX, stepZ;
	double sideDistX, sideDistZ;
	if (nonNegativeDirX)
	{
		stepX = 1;
		sideDistX = (startCellReal.x + 1.0 - eye.x) * deltaDistX;
	}
	else
	{
		stepX = -1;
		sideDistX = (eye.x - startCellReal.x) * deltaDistX;
	}

	if (nonNegativeDirZ)
	{
		stepZ = 1;
		sideDistZ = (startCellReal.z + 1.0 - eye.z) * deltaDistZ;
	}
	else
	{
		stepZ = -1;
		sideDistZ = (eye.z - startCellReal.z) * deltaDistZ;
	}

	// Pointer to voxel ID grid data.
	const char *voxels = voxelGrid.getVoxels();

	// The Z distance from the camera to the wall, and the axis of the intersected
	// voxel face. The first Z distance is a special case, so it's brought outside 
	// the DDA loop.
	double zDistance;
	Axis axis;

	// Verify that the initial voxel coordinate is within the world bounds.
	bool voxelIsValid = (startCell.x >= 0) && (startCell.y >= 0) && (startCell.z >= 0) &&
		(startCell.x < voxelGrid.getWidth()) && (startCell.y < voxelGrid.getHeight()) && 
		(startCell.z < voxelGrid.getDepth());

	if (voxelIsValid)
	{
		// Get the initial voxel ID and see how it should be rendered.
		const char initialVoxelID = voxels[startCell.x + (startCell.y * voxelGrid.getWidth()) +
			(startCell.z * voxelGrid.getWidth() * voxelGrid.getHeight())];

		// Decide how far the wall is, and which axis (voxel face) was hit.
		if (sideDistX < sideDistZ)
		{
			zDistance = sideDistX;
			axis = Axis::X;
		}
		else
		{
			zDistance = sideDistZ;
			axis = Axis::Z;
		}

		// X and Z coordinates for the initial wall hit. This will be used with the
		// player's position for drawing the floor and ceiling.
		const double initialFarPointX = eye.x + (dirX * zDistance);
		const double initialFarPointZ = eye.z + (dirZ * zDistance);

		// Get the voxel data referenced by the voxel ID. An ID of zero references the
		// "empty voxel", which has air for each voxel face.
		const VoxelData &initialVoxelData = voxelGrid.getVoxelData(initialVoxelID);

		// Horizontal texture coordinate for the initial wall column. It is always
		// a back face, so the texture coordinates are reversed.
		double u;
		if (axis == Axis::X)
		{
			const double uVal = initialFarPointZ - std::floor(initialFarPointZ);
			u = nonNegativeDirX ? (1.0 - uVal) : uVal;
		}
		else
		{
			const double uVal = initialFarPointX - std::floor(initialFarPointX);
			u = nonNegativeDirZ ? uVal : (1.0 - uVal);
		}

		// Generate a point on the ceiling edge and floor edge of the wall relative
		// to the hit point, accounting for the Y thickness of the wall as well.
		const Double3 floorPoint(
			initialFarPointX,
			startCellReal.y + initialVoxelData.yOffset,
			initialFarPointZ);
		const Double3 ceilingPoint(
			initialFarPointX,
			startCellReal.y + initialVoxelData.yOffset + initialVoxelData.ySize,
			initialFarPointZ);

		// Transform the points to camera space (projection * view).
		Double4 p1 = transform * Double4(ceilingPoint.x, ceilingPoint.y, ceilingPoint.z, 1.0);
		Double4 p2 = transform * Double4(floorPoint.x, floorPoint.y, floorPoint.z, 1.0);

		// Convert to normalized coordinates.
		p1 = p1 / p1.w;
		p2 = p2 / p2.w;

		// Translate the Y coordinates relative to the center of Y projection (y == 0.5).
		// Add Y-shearing for "fake" looking up and down. Multiply by 0.5 to apply the 
		// correct aspect ratio.
		// - Since the ray cast guarantees the intersection to be in the correct column
		//   of the screen, only the Y coordinates need to be projected.
		const double projectedY1 = (0.50 + yShear) - (p1.y * 0.50);
		const double projectedY2 = (0.50 + yShear) - (p2.y * 0.50);

		// Draw wall if the ID is not air.
		if (initialVoxelData.sideID > 0)
		{
			const TextureData &texture = this->textures[initialVoxelData.sideID - 1];

			// Draw the back-face wall column.
			drawWallColumn(x, projectedY1, projectedY2, zDistance, u, initialVoxelData.topV,
				initialVoxelData.bottomV, texture, this->fogDistance, fogColor, this->width,
				this->height, this->zBuffer.data(), colorBuffer);
		}

		// Near point for the floor and ceiling.
		const Double2 nearPoint(
			eye.x + (dirX * SoftwareRenderer::NEAR_PLANE), 
			eye.z + (dirZ * SoftwareRenderer::NEAR_PLANE));

		// These special cases only apply for the voxel the camera is in, because
		// the renderer always draws columns from top to bottom (for now).

		// Draw floor if the ID is not air.
		if (initialVoxelData.floorID > 0)
		{
			// Texture for floor.
			const TextureData &floorTexture = this->textures[initialVoxelData.floorID - 1];

			// Draw the floor as a floor if above, and as a ceiling if below.
			if (eye.y > (startCellReal.y + initialVoxelData.yOffset))
			{
				drawFloorColumn(x, floorPoint.y, Double2(floorPoint.x, floorPoint.z), nearPoint,
					Double2(eye.x, eye.z), transform, yShear, floorTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}
			else
			{
				drawCeilingColumn(x, floorPoint.y, Double2(floorPoint.x, floorPoint.z), nearPoint,
					Double2(eye.x, eye.z), transform, yShear, floorTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}
		}

		// Draw ceiling if the ID is not air.
		if (initialVoxelData.ceilingID > 0)
		{
			// Texture for ceiling.
			const TextureData &ceilingTexture = this->textures[initialVoxelData.ceilingID - 1];

			// Draw the ceiling as a ceiling if below, and as a floor if above.
			if (eye.y > (startCellReal.y + initialVoxelData.yOffset + initialVoxelData.ySize))
			{
				drawCeilingColumn(x, ceilingPoint.y, nearPoint, Double2(floorPoint.x, floorPoint.z),
					Double2(eye.x, eye.z), transform, yShear, ceilingTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}
			else
			{
				drawFloorColumn(x, ceilingPoint.y, nearPoint, Double2(floorPoint.x, floorPoint.z),
					Double2(eye.x, eye.z), transform, yShear, ceilingTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}
		}

		// Render the voxels below the camera.
		for (int voxelY = 0; voxelY < startCell.y; ++voxelY)
		{
			// Get the initial voxel ID and see how it should be rendered.
			const char initialVoxelIDBelow = voxels[startCell.x + (voxelY * voxelGrid.getWidth()) +
				(startCell.z * voxelGrid.getWidth() * voxelGrid.getHeight())];

			const VoxelData &initialVoxelData = voxelGrid.getVoxelData(initialVoxelIDBelow);

			// Horizontal texture coordinate for the initial wall column. It is always
			// a back face, so the texture coordinates are reversed.
			double u;
			if (axis == Axis::X)
			{
				const double uVal = initialFarPointZ - std::floor(initialFarPointZ);
				u = nonNegativeDirX ? (1.0 - uVal) : uVal;
			}
			else
			{
				const double uVal = initialFarPointX - std::floor(initialFarPointX);
				u = nonNegativeDirZ ? uVal : (1.0 - uVal);
			}

			// Generate a point on the ceiling edge and floor edge of the wall relative
			// to the hit point, accounting for the Y thickness of the wall as well.
			const Double3 floorPoint(
				initialFarPointX,
				static_cast<double>(voxelY) + initialVoxelData.yOffset,
				initialFarPointZ);
			const Double3 ceilingPoint(
				initialFarPointX,
				static_cast<double>(voxelY) + initialVoxelData.yOffset + initialVoxelData.ySize,
				initialFarPointZ);

			// Transform the points to camera space (projection * view).
			Double4 p1 = transform * Double4(ceilingPoint.x, ceilingPoint.y, ceilingPoint.z, 1.0);
			Double4 p2 = transform * Double4(floorPoint.x, floorPoint.y, floorPoint.z, 1.0);

			// Convert to normalized coordinates.
			p1 = p1 / p1.w;
			p2 = p2 / p2.w;

			// Translate the Y coordinates relative to the center of Y projection (y == 0.5).
			// Add Y-shearing for "fake" looking up and down. Multiply by 0.5 to apply the 
			// correct aspect ratio.
			// - Since the ray cast guarantees the intersection to be in the correct column
			//   of the screen, only the Y coordinates need to be projected.
			const double projectedY1 = (0.50 + yShear) - (p1.y * 0.50);
			const double projectedY2 = (0.50 + yShear) - (p2.y * 0.50);

			// Draw wall if the ID is not air.
			if (initialVoxelData.sideID > 0)
			{
				const TextureData &texture = this->textures[initialVoxelData.sideID - 1];

				// Draw the back-face wall column.
				drawWallColumn(x, projectedY1, projectedY2, zDistance, u, initialVoxelData.topV,
					initialVoxelData.bottomV, texture, this->fogDistance, fogColor, this->width,
					this->height, this->zBuffer.data(), colorBuffer);
			}

			// Near point for the floor and ceiling.
			const Double2 nearPoint(
				eye.x + (dirX * SoftwareRenderer::NEAR_PLANE),
				eye.z + (dirZ * SoftwareRenderer::NEAR_PLANE));

			// Draw floor if the ID is not air.
			if (initialVoxelData.floorID > 0)
			{
				// Texture for floor.
				const TextureData &floorTexture = this->textures[initialVoxelData.floorID - 1];

				// Draw the floor.
				drawFloorColumn(x, floorPoint.y, nearPoint, Double2(floorPoint.x, floorPoint.z),
					Double2(eye.x, eye.z), transform, yShear, floorTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}

			// Draw ceiling if the ID is not air.
			if (initialVoxelData.ceilingID > 0)
			{
				// Texture for ceiling.
				const TextureData &ceilingTexture = this->textures[initialVoxelData.ceilingID - 1];

				// Draw the ceiling.
				drawCeilingColumn(x, ceilingPoint.y, nearPoint, Double2(floorPoint.x, floorPoint.z),
					Double2(eye.x, eye.z), transform, yShear, ceilingTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}
		}

		// Render the voxels above the camera.
		for (int voxelY = (startCell.y + 1); voxelY < voxelGrid.getHeight(); ++voxelY)
		{
			// Get the initial voxel ID and see how it should be rendered.
			const char initialVoxelIDAbove = voxels[startCell.x + (voxelY * voxelGrid.getWidth()) +
				(startCell.z * voxelGrid.getWidth() * voxelGrid.getHeight())];

			const VoxelData &initialVoxelData = voxelGrid.getVoxelData(initialVoxelIDAbove);

			// Horizontal texture coordinate for the initial wall column. It is always
			// a back face, so the texture coordinates are reversed.
			double u;
			if (axis == Axis::X)
			{
				const double uVal = initialFarPointZ - std::floor(initialFarPointZ);
				u = nonNegativeDirX ? (1.0 - uVal) : uVal;
			}
			else
			{
				const double uVal = initialFarPointX - std::floor(initialFarPointX);
				u = nonNegativeDirZ ? uVal : (1.0 - uVal);
			}

			// Generate a point on the ceiling edge and floor edge of the wall relative
			// to the hit point, accounting for the Y thickness of the wall as well.
			const Double3 floorPoint(
				initialFarPointX,
				static_cast<double>(voxelY) + initialVoxelData.yOffset,
				initialFarPointZ);
			const Double3 ceilingPoint(
				initialFarPointX,
				static_cast<double>(voxelY) + initialVoxelData.yOffset + initialVoxelData.ySize,
				initialFarPointZ);

			// Transform the points to camera space (projection * view).
			Double4 p1 = transform * Double4(ceilingPoint.x, ceilingPoint.y, ceilingPoint.z, 1.0);
			Double4 p2 = transform * Double4(floorPoint.x, floorPoint.y, floorPoint.z, 1.0);

			// Convert to normalized coordinates.
			p1 = p1 / p1.w;
			p2 = p2 / p2.w;

			// Translate the Y coordinates relative to the center of Y projection (y == 0.5).
			// Add Y-shearing for "fake" looking up and down. Multiply by 0.5 to apply the 
			// correct aspect ratio.
			// - Since the ray cast guarantees the intersection to be in the correct column
			//   of the screen, only the Y coordinates need to be projected.
			const double projectedY1 = (0.50 + yShear) - (p1.y * 0.50);
			const double projectedY2 = (0.50 + yShear) - (p2.y * 0.50);

			// Draw wall if the ID is not air.
			if (initialVoxelData.sideID > 0)
			{
				const TextureData &texture = this->textures[initialVoxelData.sideID - 1];

				// Draw the back-face wall column.
				drawWallColumn(x, projectedY1, projectedY2, zDistance, u, initialVoxelData.topV,
					initialVoxelData.bottomV, texture, this->fogDistance, fogColor, this->width,
					this->height, this->zBuffer.data(), colorBuffer);
			}

			// Near point for the floor and ceiling.
			const Double2 nearPoint(
				eye.x + (dirX * SoftwareRenderer::NEAR_PLANE),
				eye.z + (dirZ * SoftwareRenderer::NEAR_PLANE));

			// Draw floor if the ID is not air.
			if (initialVoxelData.floorID > 0)
			{
				// Texture for floor.
				const TextureData &floorTexture = this->textures[initialVoxelData.floorID - 1];

				// Draw the floor.
				drawFloorColumn(x, floorPoint.y, nearPoint, Double2(floorPoint.x, floorPoint.z),
					Double2(eye.x, eye.z), transform, yShear, floorTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}

			// Draw ceiling if the ID is not air.
			if (initialVoxelData.ceilingID > 0)
			{
				// Texture for ceiling.
				const TextureData &ceilingTexture = this->textures[initialVoxelData.ceilingID - 1];

				// Draw the ceiling.
				drawCeilingColumn(x, ceilingPoint.y, nearPoint, Double2(floorPoint.x, floorPoint.z),
					Double2(eye.x, eye.z), transform, yShear, ceilingTexture, this->fogDistance, 
					fogColor, this->width, this->height, this->zBuffer.data(), colorBuffer);
			}
		}
	}

	// The current voxel coordinate in the DDA loop. For all intents and purposes,
	// the Y cell coordinate is constant.
	Int3 cell(startCell.x, startCell.y, startCell.z);

	// Step forward in the grid once to leave the initial voxel.
	if (sideDistX < sideDistZ)
	{
		sideDistX += deltaDistX;
		cell.x += stepX;
		axis = Axis::X;
		voxelIsValid &= (cell.x >= 0) && (cell.x < voxelGrid.getWidth());
	}
	else
	{
		sideDistZ += deltaDistZ;
		cell.z += stepZ;
		axis = Axis::Z;
		voxelIsValid &= (cell.z >= 0) && (cell.z < voxelGrid.getDepth());
	}

	zDistance = (axis == Axis::X) ?
		(static_cast<double>(cell.x) - eye.x + static_cast<double>((1 - stepX) / 2)) / dirX :
		(static_cast<double>(cell.z) - eye.z + static_cast<double>((1 - stepZ) / 2)) / dirZ;

	// Step through the voxel grid while the current coordinate is valid and
	// the distance stepped is less than the distance at which fog is maximum.
	while (voxelIsValid && (zDistance < this->fogDistance))
	{
		// X and Z coordinates on the wall from casting a ray along the XZ plane. This is 
		// used with the next DDA point ("far point") to draw the floor and ceiling.
		const double nearPointX = eye.x + (dirX * zDistance);
		const double nearPointZ = eye.z + (dirZ * zDistance);

		// Store the cell coordinates, axis, and Z distance for wall rendering, not 
		// floor and ceiling rendering.
		const int savedCellX = cell.x;
		const int savedCellZ = cell.z;
		const Axis savedAxis = axis;
		const double wallDistance = zDistance;

		// Decide which voxel in the XZ plane to step to next.
		if (sideDistX < sideDistZ)
		{
			sideDistX += deltaDistX;
			cell.x += stepX;
			axis = Axis::X;
			voxelIsValid &= (cell.x >= 0) && (cell.x < voxelGrid.getWidth());
		}
		else
		{
			sideDistZ += deltaDistZ;
			cell.z += stepZ;
			axis = Axis::Z;
			voxelIsValid &= (cell.z >= 0) && (cell.z < voxelGrid.getDepth());
		}

		// Update Z distance of the hit point. The X and Z cell coordinates have been 
		// updated by the DDA stepping.
		zDistance = (axis == Axis::X) ?
			(static_cast<double>(cell.x) - eye.x + static_cast<double>((1 - stepX) / 2)) / dirX :
			(static_cast<double>(cell.z) - eye.z + static_cast<double>((1 - stepZ) / 2)) / dirZ;

		// Far point (where the next wall hit would be). Used with the near point.
		const double farPointX = eye.x + (dirX * zDistance);
		const double farPointZ = eye.z + (dirZ * zDistance);

		// Render each voxel in the XZ column.
		for (int voxelY = 0; voxelY < voxelGrid.getHeight(); ++voxelY)
		{
			// Voxel ID of the current voxel in the column. Zero points to the "air" voxel.
			const char voxelID = voxels[savedCellX + (voxelY * voxelGrid.getWidth()) +
				(savedCellZ * voxelGrid.getWidth() * voxelGrid.getHeight())];

			// Get the voxel data associated with the voxel ID.
			const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);

			// Horizontal texture coordinate (constant for all wall pixels in a column).
			// - Remember to watch out for the edge cases where u == 1.0, resulting in an
			//   out-of-bounds texel access. Maybe use std::nextafter() for ~0.9999999?
			double u;
			if (savedAxis == Axis::X)
			{
				const double uVal = nearPointZ - std::floor(nearPointZ);
				u = nonNegativeDirX ? uVal : (1.0 - uVal);
			}
			else
			{
				const double uVal = nearPointX - std::floor(nearPointX);
				u = nonNegativeDirZ ? (1.0 - uVal) : uVal;
			}

			// Generate a point on the ceiling edge and floor edge of the wall relative
			// to the hit point, accounting for the Y thickness of the wall as well.
			const Double3 wallFloor(
				nearPointX,
				static_cast<double>(voxelY) + voxelData.yOffset,
				nearPointZ);
			const Double3 wallCeiling(
				nearPointX,
				static_cast<double>(voxelY) + voxelData.yOffset + voxelData.ySize,
				nearPointZ);

			// Transform the wall points to camera space (projection * view).
			Double4 p1 = transform * Double4(wallCeiling.x, wallCeiling.y, wallCeiling.z, 1.0);
			Double4 p2 = transform * Double4(wallFloor.x, wallFloor.y, wallFloor.z, 1.0);

			// Convert to normalized coordinates.
			p1 = p1 / p1.w;
			p2 = p2 / p2.w;

			// Translate the Y coordinates relative to the center of Y projection (y == 0.5).
			// Add Y-shearing for "fake" looking up and down. Multiply by 0.5 to apply the 
			// correct aspect ratio.
			// - Since the ray cast guarantees the intersection to be in the correct column
			//   of the screen, only the Y coordinates need to be projected.
			const double projectedY1 = (0.50 + yShear) - (p1.y * 0.50);
			const double projectedY2 = (0.50 + yShear) - (p2.y * 0.50);

			// Draw wall if the ID is not air.
			if (voxelData.sideID > 0)
			{
				const TextureData &texture = this->textures[voxelData.sideID - 1];

				drawWallColumn(x, projectedY1, projectedY2, wallDistance, u, voxelData.topV,
					voxelData.bottomV, texture, this->fogDistance, fogColor, this->width,
					this->height, this->zBuffer.data(), colorBuffer);
			}

			// Draw the floor if the ID is not air.
			if (voxelData.floorID > 0)
			{
				// Texture for floor.
				const TextureData &floorTexture = this->textures[voxelData.floorID - 1];

				drawFloorColumn(x, wallFloor.y, Double2(nearPointX, nearPointZ),
					Double2(farPointX, farPointZ), Double2(eye.x, eye.z), transform,
					yShear, floorTexture, fogDistance, fogColor, this->width, this->height, 
					this->zBuffer.data(), colorBuffer);
			}

			// Draw the ceiling if the ID is not air.
			if (voxelData.ceilingID > 0)
			{
				// Texture for ceiling.
				const TextureData &ceilingTexture = this->textures[voxelData.ceilingID - 1];

				drawCeilingColumn(x, wallCeiling.y, Double2(nearPointX, nearPointZ),
					Double2(farPointX, farPointZ), Double2(eye.x, eye.z), transform,
					yShear, ceilingTexture, fogDistance, fogColor, this->width, this->height, 
					this->zBuffer.data(), colorBuffer);
			}
		}
	}

	// Sprites.
	// - Maybe put the sprite drawing into a lambda, so the required parameters are
	//   easier to understand?
	// - Sprites *could* be done outside the ray casting loop, though they would need to be
	//   parallelized a bit differently then. Here, it is easy to parallelize by column,
	//   so it might be faster in practice, even if a little redundant work is done.

	for (const auto &pair : this->visibleFlats)
	{
		const Flat &flat = *pair.first;
		const Flat::Projection &flatProjection = pair.second;

		// X percent across the screen.
		const double xPercent = static_cast<double>(x) /
			static_cast<double>(this->width);

		// Find where the column is within the X range of the flat.
		const double xRangePercent = (xPercent - flatProjection.right.x) /
			(flatProjection.left.x - flatProjection.right.x);

		// Don't render the flat if the X range percent is invalid.
		if ((xRangePercent < 0.0) || (xRangePercent >= 1.0))
		{
			continue;
		}

		// Horizontal texture coordinate in the flat. This actually doesn't need
		// perspective-correctness after all.
		const double u = flatProjection.right.u + 
			((flatProjection.left.u - flatProjection.right.u) * xRangePercent);

		// Interpolate the projected Y coordinates based on the X range.
		const double projectedY1 = flatProjection.right.topY +
			((flatProjection.left.topY - flatProjection.right.topY) * xRangePercent);
		const double projectedY2 = flatProjection.right.bottomY +
			((flatProjection.left.bottomY - flatProjection.right.bottomY) * xRangePercent);

		// Get the start and end Y pixel coordinates of the projected points (potentially
		// outside the top or bottom of the screen).
		const double heightReal = static_cast<double>(this->height);
		const int projectedStart = static_cast<int>(std::round(projectedY1 * heightReal));
		const int projectedEnd = static_cast<int>(std::round(projectedY2 * heightReal));

		// Clamp the Y coordinates for where the wall starts and stops on the screen.
		const int drawStart = std::max(0, projectedStart);
		const int drawEnd = std::min(this->height, projectedEnd);

		// The texture associated with the voxel ID.
		const TextureData &texture = this->textures[flat.textureID];

		// X position in texture (temporarily using modulo to protect against edge cases 
		// where u == 1.0; it should be fixed in the u calculation instead).
		const int textureX = static_cast<int>((flat.flipped ? (1.0 - u) : u) *
			static_cast<double>(texture.width)) % texture.width;

		// I think this needs to be perspective correct. Currently, it's like affine
		// texture mapping, but for depth.
		const double zDistance = flatProjection.right.z +
			((flatProjection.left.z - flatProjection.right.z) * xRangePercent);

		// Linearly interpolated fog.
		const double fogPercent = std::min(zDistance / this->fogDistance, 1.0);

		double *depth = this->zBuffer.data();
		for (int y = drawStart; y < drawEnd; ++y)
		{
			const int index = x + (y * this->width);

			if (zDistance <= depth[index])
			{
				// Vertical texture coordinate.
				const double v = static_cast<double>(y - projectedStart) /
					static_cast<double>(projectedEnd - projectedStart);

				// Y position in texture.
				const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

				const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

				// Draw only if the texel is not transparent.
				if (texel.w > 0.0)
				{
					const Double3 color(texel.x, texel.y, texel.z);

					colorBuffer[index] = color.lerp(fogColor, fogPercent).clamped().toRGB();
					depth[index] = zDistance;
				}
			}
		}
	}
}

void SoftwareRenderer::render(const Double3 &eye, const Double3 &forward, double fovY,
	double daytimePercent, const VoxelGrid &voxelGrid, uint32_t *colorBuffer)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	// Camera values for rendering. We trick the 2.5D ray caster into thinking the player is 
	// always looking straight forward, but we use the Y component of the player's direction 
	// to offset projected coordinates via Y-shearing. Assume "forward" is normalized.
	const Double3 forwardXZ = Double3(forward.x, 0.0, forward.z).normalized();
	const Double3 rightXZ = forwardXZ.cross(Double3::UnitY).normalized();
	const Double3 up = Double3::UnitY;

	// Zoom of the camera, based on vertical field of view.
	const double zoom = 1.0 / std::tan((fovY * 0.5) * DEG_TO_RAD);

	// Refresh transformation matrix (model matrix isn't required because it's just 
	// the identity matrix).
	const Matrix4d view = Matrix4d::view(eye, forwardXZ, rightXZ, up);
	const Matrix4d projection = Matrix4d::perspective(fovY, aspect,
		SoftwareRenderer::NEAR_PLANE, SoftwareRenderer::FAR_PLANE);
	const Matrix4d transform = projection * view;

	// Y-shearing is the distance that projected Y coordinates are translated by based on the 
	// player's 3D direction and field of view. First get the player's angle relative to the 
	// horizon, then get the tangent of that angle. The Y component of the player's direction
	// must be clamped less than 1 because 1 would imply they are looking straight up or down, 
	// which is impossible in 2.5D rendering (the vertical line segment of the view frustum 
	// would be infinitely high or low). The camera code should take care of the clamping for us.
	const double yShear = [&forward, zoom]()
	{
		// Get the length of the forward vector's projection onto the XZ plane.
		const double xzProjection = std::sqrt((forward.x * forward.x) + (forward.z * forward.z));

		// Get the angle of the player's direction above or below the XZ plane.
		double angleRadians = (forward.y >= 0.0) ? 
			std::acos(xzProjection) : -std::acos(xzProjection);

		// Check for infinities and NaNs here due to some edge cases where Y values 
		// close to zero become NaNs in normalized vectors (like the player's direction).
		if (!std::isfinite(angleRadians))
		{
			angleRadians = 0.0;
		}

		// Get the number of screen heights to translate all projected Y coordinates by, 
		// relative to the current zoom. As a reference, this should be some value roughly 
		// between -1.0 and 1.0 for "acceptable skewing" at a vertical FOV of 90.0. If the 
		// camera is not clamped, this could theoretically be between -infinity and infinity, 
		// but it would result in far too much skewing.
		return std::tan(angleRadians) * zoom;
	}();

	// Camera values for generating 2D rays with.
	const Double2 forwardComp = Double2(forwardXZ.x, forwardXZ.z).normalized() * zoom;
	const Double2 right2D = Double2(rightXZ.x, rightXZ.z).normalized() * aspect;

	// Calculate fog color and sun direction. The fog color is also used for the sky.
	const Double3 fogColor = this->getFogColor(daytimePercent);
	const Double3 sunDirection = this->getSunDirection(daytimePercent);

	// Lambda for rendering some columns of pixels using 2.5D ray casting. This is
	// the cheaper form of ray casting (although still not very efficient), and results
	// in a "fake" 3D scene.
	auto renderColumns = [this, &eye, &voxelGrid, daytimePercent, colorBuffer, 
		&fogColor, &sunDirection, widthReal, &transform, yShear,
		&forwardComp, &right2D](int startX, int endX)
	{
		for (int x = startX; x < endX; ++x)
		{
			// X percent across the screen.
			const double xPercent = (static_cast<double>(x) + 0.50) / widthReal;

			// "Right" component of the ray direction, based on current screen X.
			const Double2 rightComp = right2D * ((2.0 * xPercent) - 1.0);

			// Calculate the ray direction through the pixel.
			// - If un-normalized, it uses the Z distance, but the insides of voxels
			//   don't look right then.
			const Double2 direction = (forwardComp + rightComp).normalized();

			// Cast the 2D ray and fill in the column's pixels with color.
			this->castColumnRay(x, eye, direction, transform, yShear,
				daytimePercent, fogColor, sunDirection, voxelGrid, colorBuffer);
		}
	};

	// Lambda for clearing some rows on the frame buffer quickly.
	auto clearRows = [this, colorBuffer, &fogColor](int startY, int endY)
	{
		const int startIndex = startY * this->width;
		const int endIndex = endY * this->width;

		// Clear some color rows.
		uint32_t *colorBegin = colorBuffer + startIndex;
		uint32_t *colorEnd = colorBuffer + endIndex;
		std::fill(colorBegin, colorEnd, fogColor.toRGB());

		// Clear some depth rows.
		const auto depthBegin = this->zBuffer.begin() + startIndex;
		const auto depthEnd = this->zBuffer.begin() + endIndex;
		std::fill(depthBegin, depthEnd, std::numeric_limits<double>::infinity());
	};

	// Start a thread for refreshing the visible flats. This should erase the old list,
	// calculate a new list, and sort it by depth.
	std::thread sortThread([this, eye, yShear, &transform]
	{ 
		this->updateVisibleFlats(eye, yShear, transform);
	});

	// Prepare render threads. These are used for clearing the frame buffer and rendering.
	std::vector<std::thread> renderThreads(this->renderThreadCount);

	// Start clearing the frame buffer with the render threads.
	for (size_t i = 0; i < renderThreads.size(); ++i)
	{
		// "blockSize" is the approximate number of rows per thread. Rounding is involved so 
		// the start and stop coordinates are correct for all resolutions.
		const double blockSize = heightReal / static_cast<double>(this->renderThreadCount);
		const int startY = static_cast<int>(std::round(static_cast<double>(i) * blockSize));
		const int endY = static_cast<int>(std::round(static_cast<double>(i + 1) * blockSize));

		// Make sure the rounding is correct.
		assert(startY >= 0);
		assert(endY <= this->height);

		renderThreads[i] = std::thread(clearRows, startY, endY);
	}

	// Wait for the render threads to finish clearing.
	for (auto &thread : renderThreads)
	{
		thread.join();
	}

	// Wait for the sorting thread to finish.
	sortThread.join();

	// Start rendering the scene with the render threads.
	for (size_t i = 0; i < renderThreads.size(); ++i)
	{
		// "blockSize" is the approximate number of columns per thread. Rounding is involved so 
		// the start and stop coordinates are correct for all resolutions.
		const double blockSize = widthReal / static_cast<double>(this->renderThreadCount);
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
