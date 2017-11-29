#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <thread>

#include "SoftwareRenderer.h"
#include "../Math/Constants.h"
#include "../Utilities/Debug.h"
#include "../Utilities/Platform.h"
#include "../World/VoxelData.h"
#include "../World/VoxelGrid.h"

SoftwareRenderer::ShadingInfo::ShadingInfo(const Double3 &horizonSkyColor, 
	const Double3 &zenithSkyColor, const Double3 &sunColor, 
	const Double3 &sunDirection, double ambient, double fogDistance)
	: horizonSkyColor(horizonSkyColor), zenithSkyColor(zenithSkyColor),
	sunColor(sunColor), sunDirection(sunDirection)
{
	this->ambient = ambient;
	this->fogDistance = fogDistance;
}

const double SoftwareRenderer::NEAR_PLANE = 0.0001;
const double SoftwareRenderer::FAR_PLANE = 1000.0;
const double SoftwareRenderer::JUST_BELOW_ONE = std::nextafter(1.0, 0.0);

SoftwareRenderer::SoftwareRenderer(int width, int height)
{
	// Initialize 2D frame buffer.
	const int pixelCount = width * height;
	this->depthBuffer = std::vector<double>(pixelCount);
	std::fill(this->depthBuffer.begin(), this->depthBuffer.end(), 
		std::numeric_limits<double>::infinity());

	this->width = width;
	this->height = height;

	// Obtain the number of threads to use.
	this->renderThreadCount = Platform::getThreadCount();

	// Fog distance is zero by default.
	this->fogDistance = 0.0;
}

SoftwareRenderer::~SoftwareRenderer()
{

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

	SoftwareTexture texture;
	texture.pixels = std::vector<Double4>(pixelCount);
	texture.width = width;
	texture.height = height;

	// Initialize the transparency boolean to false, and set it to true if any
	// non-opaque texels exist in the texture.
	texture.containsTransparency = false;

	// Convert ARGB color from integer to double-precision format for speed.
	// This does waste an extreme amount of memory (32 bytes per pixel!), but
	// it's not a big deal for Arena's textures (mostly 64x64, so eight textures
	// would be a megabyte).
	Double4 *texturePixels = texture.pixels.data();
	for (int i = 0; i < pixelCount; i++)
	{
		texturePixels[i] = Double4::fromARGB(pixels[i]);

		// Set the transparency boolean if the texture contains any non-opaque texels
		// (like with hedges). This only affects how occlusion culling is handled, and 
		// isn't used with partially transparent texels because that would require a
		// reordering of how voxels are rendered.
		if (!texture.containsTransparency && (texturePixels[i].w < 1.0))
		{
			texture.containsTransparency = true;
		}
	}

	this->textures.push_back(std::move(texture));

	return static_cast<int>(this->textures.size() - 1);
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

void SoftwareRenderer::removeAllTextures()
{
	this->textures.clear();
}

void SoftwareRenderer::resize(int width, int height)
{
	const int pixelCount = width * height;
	this->depthBuffer.resize(pixelCount);
	std::fill(this->depthBuffer.begin(), this->depthBuffer.end(), 
		std::numeric_limits<double>::infinity());

	this->width = width;
	this->height = height;
}

void SoftwareRenderer::updateVisibleFlats(const Double2 &eye, const Double2 &direction,
	const Matrix4d &transform, double yShear, double aspect, double zoom)
{
	assert(direction.isNormalized());

	this->visibleFlats.clear();

	// Each flat shares the same axes. The forward direction always faces opposite to 
	// the camera direction.
	const Double3 flatForward = Double3(-direction.x, 0.0, -direction.y).normalized();
	const Double3 flatUp = Double3::UnitY;
	const Double3 flatRight = flatForward.cross(flatUp).normalized();

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
		const Double2 flatEyeDiff = (flatPosition2D - eye).normalized();
		const bool inFrontOfCamera = direction.dot(flatEyeDiff) > 0.0;

		if (inFrontOfCamera)
		{
			// Now project two of the flat's opposing corner points into camera space.
			// The Z value is used with flat sorting (not rendering), and the X and Y values 
			// are used to find where the flat is on-screen.
			Double4 projStart = transform * Double4(flatFrame.topStart, 1.0);
			Double4 projEnd = transform * Double4(flatFrame.bottomEnd, 1.0);

			// Normalize coordinates.
			projStart = projStart / projStart.w;
			projEnd = projEnd / projEnd.w;

			// Assign each screen value to the flat frame data.
			flatFrame.startX = 0.50 + (projStart.x * 0.50);
			flatFrame.endX = 0.50 + (projEnd.x * 0.50);
			flatFrame.startY = (0.50 + yShear) - (projStart.y * 0.50);
			flatFrame.endY = (0.50 + yShear) - (projEnd.y * 0.50);
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

Double3 SoftwareRenderer::getWallNormal(WallFacing wallFacing)
{
	// Decide what the wall normal is, based on the wall facing. It can only be on the 
	// X or Z axis because floors and ceilings are drawn separately from walls, and their 
	// normals are trivial.
	if (wallFacing == WallFacing::PositiveX)
	{
		return Double3::UnitX;
	}
	else if (wallFacing == WallFacing::NegativeX)
	{
		return -Double3::UnitX;
	}
	else if (wallFacing == WallFacing::PositiveZ)
	{
		return Double3::UnitZ;
	}
	else
	{
		return -Double3::UnitZ;
	}
}

double SoftwareRenderer::getProjectedY(const Double3 &point, 
	const Matrix4d &transform, double yShear)
{
	// Project the 3D point to camera space.
	Double4 projected = transform * Double4(point, 1.0);

	// Convert the projected Y to normalized coordinates.
	projected.y /= projected.w;

	// Calculate the Y position relative to the center row of the screen, and offset it by 
	// the Y-shear. Multiply by 0.5 for the correct aspect ratio.
	return (0.50 + yShear) - (projected.y * 0.50);
}

bool SoftwareRenderer::findDiag1Intersection(int voxelX, int voxelZ, const Double2 &nearPoint,
	const Double2 &farPoint, DiagonalHit &hit)
{
	// Start, middle, and end points of the diagonal line segment relative to the grid.
	const Double2 diagStart(
		static_cast<double>(voxelX),
		static_cast<double>(voxelZ));
	const Double2 diagMiddle(
		static_cast<double>(voxelX) + 0.50,
		static_cast<double>(voxelZ) + 0.50);
	const Double2 diagEnd(
		static_cast<double>(voxelX) + SoftwareRenderer::JUST_BELOW_ONE,
		static_cast<double>(voxelZ) + SoftwareRenderer::JUST_BELOW_ONE);

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
		hit.u = std::max(std::min(hitCoordinate, SoftwareRenderer::JUST_BELOW_ONE), 0.0);
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
	const Double2 &farPoint, DiagonalHit &hit)
{
	// Mostly a copy of findDiag1Intersection(), though with a couple different values
	// for the diagonal (end points, slope, etc.).

	// Start, middle, and end points of the diagonal line segment relative to the grid.
	const Double2 diagStart(
		static_cast<double>(voxelX) + SoftwareRenderer::JUST_BELOW_ONE,
		static_cast<double>(voxelZ));
	const Double2 diagMiddle(
		static_cast<double>(voxelX) + 0.50,
		static_cast<double>(voxelZ) + 0.50);
	const Double2 diagEnd(
		static_cast<double>(voxelX),
		static_cast<double>(voxelZ) + SoftwareRenderer::JUST_BELOW_ONE);

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
				return SoftwareRenderer::JUST_BELOW_ONE - (nearPoint.x - diagStart.x);
			}
			else if (isVertical)
			{
				// The Z axis intercept is the compliment of the intersection coordinate.
				return SoftwareRenderer::JUST_BELOW_ONE - (nearPoint.y - diagStart.y);
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
		hit.u = std::max(std::min(hitCoordinate, SoftwareRenderer::JUST_BELOW_ONE), 0.0);
		hit.point = Double2(
			static_cast<double>(voxelX) + (SoftwareRenderer::JUST_BELOW_ONE - hit.u),
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

void SoftwareRenderer::diagonalProjection(double voxelYReal, const VoxelData &voxelData,
	const Double2 &point, const Matrix4d &transform, double yShear, int frameHeight,
	double heightReal, double &diagTopScreenY, double &diagBottomScreenY, int &diagStart, 
	int &diagEnd)
{
	// Points in world space for the top and bottom of the diagonal wall slice.
	const Double3 diagTop(
		point.x,
		voxelYReal + voxelData.yOffset + voxelData.ySize,
		point.y);
	const Double3 diagBottom(
		point.x,
		voxelYReal + voxelData.yOffset,
		point.y);

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

void SoftwareRenderer::drawWall(int x, int yStart, int yEnd, double projectedYStart,
	double projectedYEnd, double z, double u, double topV, double bottomV, 
	const Double3 &normal, const SoftwareTexture &texture, const ShadingInfo &shadingInfo,
	int frameWidth, int frameHeight, double *depthBuffer, uint32_t *colorBuffer)
{
	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u *
		static_cast<double>(texture.width)) % texture.width;

	// Linearly interpolated fog.
	const double fogPercent = std::min(z / shadingInfo.fogDistance, 1.0);
	const Double3 &fogColor = shadingInfo.horizonSkyColor;

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frameWidth);

		// Check depth of the pixel. A bias of epsilon is added to reduce artifacts from
		// adjacent walls sharing the same Z value. More strict drawing rules (voxel
		// stepping order? Occlusion culling?) might be a better fix for this.
		if (z <= (depthBuffer[index] - Constants::Epsilon))
		{
			// Percent stepped from beginning to end on the column.
			const double yPercent = ((static_cast<double>(y) + 0.50) - projectedYStart) /
				static_cast<double>(projectedYEnd - projectedYStart);

			// Vertical texture coordinate.
			const double v = topV + ((bottomV - topV) * yPercent);

			// Y position in texture.
			const int textureY = static_cast<int>(v * 
				static_cast<double>(texture.height)) % texture.height;

			const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

			// Draw only if the texel is not transparent.
			if (texel.w > 0.0)
			{
				const Double3 color(
					texel.x * (shadingInfo.ambient + sunComponent.x),
					texel.y * (shadingInfo.ambient + sunComponent.y),
					texel.z * (shadingInfo.ambient + sunComponent.z));

				colorBuffer[index] = color.lerp(fogColor, fogPercent).clamped().toRGB();
				depthBuffer[index] = z;
			}
		}
	}
}

void SoftwareRenderer::drawFloorOrCeiling(int x, int yStart, int yEnd, double projectedYStart,
	double projectedYEnd, const Double2 &startPoint, const Double2 &endPoint,
	double startZ, double endZ, const Double3 &normal, const SoftwareTexture &texture,
	const ShadingInfo &shadingInfo, int frameWidth, int frameHeight, double *depthBuffer,
	uint32_t *colorBuffer)
{
	// Values for perspective-correct interpolation.
	const double startZRecip = 1.0 / startZ;
	const double endZRecip = 1.0 / endZ;
	const Double2 startPointDiv = startPoint * startZRecip;
	const Double2 endPointDiv = endPoint * endZRecip;

	// Fog color to interpolate with.
	const Double3 &fogColor = shadingInfo.horizonSkyColor;

	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frameWidth);

		// Percent stepped from beginning to end on the column.
		const double yPercent = ((static_cast<double>(y) + 0.50) - projectedYStart) /
			static_cast<double>(projectedYEnd - projectedYStart);

		// Interpolate between the near and far point.
		const Double2 currentPoint =
			(startPointDiv + ((endPointDiv - startPointDiv) * yPercent)) /
			(startZRecip + ((endZRecip - startZRecip) * yPercent));
		const double z = 1.0 / (startZRecip + ((endZRecip - startZRecip) * yPercent));

		// Linearly interpolated fog.
		const double fogPercent = std::min(z / shadingInfo.fogDistance, 1.0);

		if (z <= depthBuffer[index])
		{
			// Horizontal texture coordinate.
			const double u = 1.0 - (currentPoint.x - std::floor(currentPoint.x));

			// Horizontal offset in texture.
			const int textureX = static_cast<int>(u *
				static_cast<double>(texture.width)) % texture.width;

			// Vertical texture coordinate.
			const double v = 1.0 - (currentPoint.y - std::floor(currentPoint.y));

			// Y position in texture.
			const int textureY = static_cast<int>(v *
				static_cast<double>(texture.height)) % texture.height;

			const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

			// Draw only if the texel is not transparent.
			if (texel.w > 0.0)
			{
				const Double3 color(
					texel.x * (shadingInfo.ambient + sunComponent.x),
					texel.y * (shadingInfo.ambient + sunComponent.y),
					texel.z * (shadingInfo.ambient + sunComponent.z));

				colorBuffer[index] = color.lerp(fogColor, fogPercent).clamped().toRGB();
				depthBuffer[index] = z;
			}
		}
	}
}

void SoftwareRenderer::drawInitialVoxelColumn(int x, int voxelX, int voxelZ, double playerY,
	WallFacing wallFacing, const Double2 &nearPoint, const Double2 &farPoint, double nearZ,
	double farZ, const Matrix4d &transform, double yShear, const ShadingInfo &shadingInfo,
	const VoxelGrid &voxelGrid, const std::vector<SoftwareTexture> &textures, int frameWidth,
	int frameHeight, double *depthBuffer, uint32_t *colorBuffer)
{
	// This method handles some special cases such as drawing the back-faces of wall sides.

	// When clamping Y values for drawing ranges, subtract 0.5 from starts and add 0.5 to 
	// ends before converting to integers because the drawing methods sample at the center 
	// of pixels. The clamping function depends on which side of the range is being clamped; 
	// either way, the drawing range should be contained within the projected range at the 
	// sub-pixel level. This ensures that the vertical texture coordinate is always within 0->1.

	const double heightReal = static_cast<double>(frameHeight);

	// Y position at the base of the player's voxel.
	const double playerYFloor = std::floor(playerY);

	// Voxel Y of the player.
	const int playerVoxelY = static_cast<int>(playerYFloor);

	// Horizontal texture coordinate for the inner wall. Shared between all voxels in this
	// voxel column.
	const double u = [&farPoint, wallFacing]()
	{
		if (wallFacing == WallFacing::PositiveX)
		{
			return farPoint.y - std::floor(farPoint.y);
		}
		else if (wallFacing == WallFacing::NegativeX)
		{
			return 1.0 - (farPoint.y - std::floor(farPoint.y));
		}
		else if (wallFacing == WallFacing::PositiveZ)
		{
			return 1.0 - (farPoint.x - std::floor(farPoint.x));
		}
		else
		{
			return farPoint.x - std::floor(farPoint.x);
		}
	}();

	// Wall normals are always reversed for the initial voxel column.
	const Double3 wallNormal = -SoftwareRenderer::getWallNormal(wallFacing);

	auto drawPlayersVoxel = [x, voxelX, voxelZ, playerY, playerYFloor, &wallNormal, &nearPoint, 
		&farPoint, nearZ, farZ, u, &transform, yShear, &shadingInfo, &voxelGrid, &textures, 
		frameWidth, frameHeight, heightReal, depthBuffer, colorBuffer]()
	{
		const int voxelY = static_cast<int>(playerYFloor);
		const char voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);

		// 3D points to be used for rendering columns once they are projected.
		const Double3 farCeilingPoint(
			farPoint.x, 
			playerYFloor + voxelData.yOffset + voxelData.ySize, 
			farPoint.y);
		const Double3 nearCeilingPoint(
			nearPoint.x,
			playerYFloor + voxelData.yOffset + voxelData.ySize,
			nearPoint.y);
		const Double3 farFloorPoint(
			farPoint.x,
			playerYFloor + voxelData.yOffset,
			farPoint.y);
		const Double3 nearFloorPoint(
			nearPoint.x,
			playerYFloor + voxelData.yOffset,
			nearPoint.y);

		// Y position of the player relative to the base of the voxel.
		const double playerYRelative = playerY - playerYFloor;

		// Y screen positions of the projections (unclamped; potentially outside the screen).
		// Once they are clamped and are converted to integers, then they are suitable for 
		// defining drawing ranges.
		const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
			farCeilingPoint, transform, yShear) * heightReal;
		const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
			nearCeilingPoint, transform, yShear) * heightReal;
		const double farFloorScreenY = SoftwareRenderer::getProjectedY(
			farFloorPoint, transform, yShear) * heightReal;
		const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
			nearFloorPoint, transform, yShear) * heightReal;
		
		// Decide which order to draw each pixel column in, based on the player's Y position.
		if (playerYRelative > (voxelData.yOffset + voxelData.ySize))
		{
			// Above wall (drawing order matters).
			const int ceilingStart = std::min(std::max(0, 
				static_cast<int>(std::ceil(farCeilingScreenY - 0.50))), frameHeight);
			const int ceilingEnd = std::min(std::max(0,
				static_cast<int>(std::floor(nearCeilingScreenY + 0.50))), frameHeight);
			const int wallStart = ceilingStart;
			const int wallEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farFloorScreenY + 0.50))), frameHeight);
			const int floorStart = wallEnd;
			const int floorEnd = std::min(std::max(0,
				static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);

			// 1) Ceiling.
			if (voxelData.ceilingID > 0)
			{
				const Double3 ceilingNormal = Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
					nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
					textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 2) Diagonal 1.
			if (voxelData.diag1ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag1Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 3) Diagonal 2.
			if (voxelData.diag2ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag2Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 4) Inner wall.
			if (voxelData.sideID > 0)
			{
				SoftwareRenderer::drawWall(x, wallStart, wallEnd, farCeilingScreenY,
					farFloorScreenY, farZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
					textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 5) Inner floor.
			if (voxelData.floorID > 0)
			{
				const Double3 floorNormal = Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
					nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
					textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}
		}
		else if (playerYRelative < voxelData.yOffset)
		{
			// Below wall (drawing order matters).
			const int ceilingStart = std::min(std::max(0,
				static_cast<int>(std::ceil(nearCeilingScreenY - 0.50))), frameHeight);
			const int ceilingEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farCeilingScreenY + 0.50))), frameHeight);
			const int wallStart = ceilingEnd;
			const int wallEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farFloorScreenY + 0.50))), frameHeight);
			const int floorStart = std::min(std::max(0,
				static_cast<int>(std::ceil(nearFloorScreenY - 0.50))), frameHeight);
			const int floorEnd = wallEnd;

			// 1) Floor.
			if (voxelData.floorID > 0)
			{
				const Double3 floorNormal = -Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
					nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
					textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 2) Diagonal 1.
			if (voxelData.diag1ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag1Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 3) Diagonal 2.
			if (voxelData.diag2ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag2Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 4) Inner wall.
			if (voxelData.sideID > 0)
			{
				SoftwareRenderer::drawWall(x, wallStart, wallEnd, farCeilingScreenY,
					farFloorScreenY, farZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
					textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 5) Inner ceiling.
			if (voxelData.ceilingID > 0)
			{
				const Double3 ceilingNormal = -Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
					nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
					textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}
		}
		else
		{
			// Inside wall (drawing order does not matter).
			const int ceilingStart = std::min(std::max(0,
				static_cast<int>(std::ceil(nearCeilingScreenY - 0.50))), frameHeight);
			const int ceilingEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farCeilingScreenY + 0.50))), frameHeight);
			const int wallStart = ceilingEnd;
			const int wallEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farFloorScreenY + 0.50))), frameHeight);
			const int floorStart = wallEnd;
			const int floorEnd = std::min(std::max(0,
				static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);

			// 1) Diagonal 1.
			if (voxelData.diag1ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag1Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 2) Diagonal 2.
			if (voxelData.diag2ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag2Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 3) Inner ceiling.
			if (voxelData.ceilingID > 0)
			{
				const Double3 ceilingNormal = -Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
					nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
					textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 4) Inner wall.
			if (voxelData.sideID > 0)
			{
				SoftwareRenderer::drawWall(x, wallStart, wallEnd, farCeilingScreenY,
					farFloorScreenY, farZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
					textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 5) Inner floor.
			if (voxelData.floorID > 0)
			{
				const Double3 floorNormal = Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
					nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
					textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}
		}
	};

	auto drawInitialVoxelBelow = [x, voxelX, voxelZ, &wallNormal, &nearPoint, &farPoint,
		nearZ, farZ, u, &transform, yShear, &shadingInfo, &voxelGrid, &textures, frameWidth,
		frameHeight, heightReal, depthBuffer, colorBuffer](int voxelY)
	{
		const char voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelYReal = static_cast<double>(voxelY);

		// 3D points to be used for rendering columns once they are projected.
		const Double3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			farPoint.y);
		const Double3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			nearPoint.y);
		const Double3 farFloorPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset,
			farPoint.y);
		const Double3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset,
			nearPoint.y);
		
		// Y screen positions of the projections (unclamped; potentially outside the screen).
		// Once they are clamped and are converted to integers, then they are suitable for 
		// defining drawing ranges.
		const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
			farCeilingPoint, transform, yShear) * heightReal;
		const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
			nearCeilingPoint, transform, yShear) * heightReal;
		const double farFloorScreenY = SoftwareRenderer::getProjectedY(
			farFloorPoint, transform, yShear) * heightReal;
		const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
			nearFloorPoint, transform, yShear) * heightReal;
		
		const int ceilingStart = std::min(std::max(0,
			static_cast<int>(std::ceil(farCeilingScreenY - 0.50))), frameHeight);
		const int ceilingEnd = std::min(std::max(0,
			static_cast<int>(std::floor(nearCeilingScreenY + 0.50))), frameHeight);
		const int wallStart = ceilingStart;
		const int wallEnd = std::min(std::max(0,
			static_cast<int>(std::floor(farFloorScreenY + 0.50))), frameHeight);
		const int floorStart = wallEnd;
		const int floorEnd = std::min(std::max(0,
			static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);
		
		// 1) Ceiling.
		if (voxelData.ceilingID > 0)
		{
			const Double3 ceilingNormal = Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
				nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
				textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 2) Diagonal 1.
		if (voxelData.diag1ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag1Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}

		// 3) Diagonal 2.
		if (voxelData.diag2ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag2Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}

		// 4) Inner wall.
		if (voxelData.sideID > 0)
		{
			SoftwareRenderer::drawWall(x, wallStart, wallEnd, farCeilingScreenY,
				farFloorScreenY, farZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
				textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 5) Inner floor.
		if (voxelData.floorID > 0)
		{
			const Double3 floorNormal = Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
				nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
				textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}
	};

	auto drawInitialVoxelAbove = [x, voxelX, voxelZ, playerY, &wallNormal, &nearPoint, &farPoint,
		nearZ, farZ, u, &transform, yShear, &shadingInfo, &voxelGrid, &textures, frameWidth,
		frameHeight, heightReal, depthBuffer, colorBuffer](int voxelY)
	{
		const char voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelYReal = static_cast<double>(voxelY);

		// 3D points to be used for rendering columns once they are projected.
		const Double3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			farPoint.y);
		const Double3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			nearPoint.y);
		const Double3 farFloorPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset,
			farPoint.y);
		const Double3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset,
			nearPoint.y);

		// Y screen positions of the projections (unclamped; potentially outside the screen).
		// Once they are clamped and are converted to integers, then they are suitable for 
		// defining drawing ranges.
		const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
			farCeilingPoint, transform, yShear) * heightReal;
		const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
			nearCeilingPoint, transform, yShear) * heightReal;
		const double farFloorScreenY = SoftwareRenderer::getProjectedY(
			farFloorPoint, transform, yShear) * heightReal;
		const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
			nearFloorPoint, transform, yShear) * heightReal;

		const int ceilingStart = std::min(std::max(0,
			static_cast<int>(std::ceil(nearCeilingScreenY - 0.50))), frameHeight);
		const int ceilingEnd = std::min(std::max(0,
			static_cast<int>(std::floor(farCeilingScreenY + 0.50))), frameHeight);
		const int wallStart = ceilingEnd;
		const int wallEnd = std::min(std::max(0,
			static_cast<int>(std::floor(farFloorScreenY + 0.50))), frameHeight);
		const int floorStart = std::min(std::max(0,
			static_cast<int>(std::ceil(nearFloorScreenY - 0.50))), frameHeight);
		const int floorEnd = wallEnd;

		// 1) Floor.
		if (voxelData.floorID > 0)
		{
			const Double3 floorNormal = -Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
				nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
				textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 2) Diagonal 1.
		if (voxelData.diag1ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag1Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}

		// 3) Diagonal 2.
		if (voxelData.diag2ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag2Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}

		// 4) Inner wall.
		if (voxelData.sideID > 0)
		{
			SoftwareRenderer::drawWall(x, wallStart, wallEnd, farCeilingScreenY,
				farFloorScreenY, farZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
				textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 5) Inner ceiling.
		if (voxelData.ceilingID > 0)
		{
			const Double3 ceilingNormal = -Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
				nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
				textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}
	};

	// Draw the voxel that the player is in first.
	drawPlayersVoxel();

	// Draw voxels below the player.
	for (int voxelY = (playerVoxelY - 1); voxelY >= 0; voxelY--)
	{
		drawInitialVoxelBelow(voxelY);
	}

	// Draw voxels above the player.
	for (int voxelY = (playerVoxelY + 1); voxelY < voxelGrid.getHeight(); voxelY++)
	{
		drawInitialVoxelAbove(voxelY);
	}
}

void SoftwareRenderer::drawVoxelColumn(int x, int voxelX, int voxelZ, double playerY,
	WallFacing wallFacing, const Double2 &nearPoint, const Double2 &farPoint, double nearZ,
	double farZ, const Matrix4d &transform, double yShear, const ShadingInfo &shadingInfo,
	const VoxelGrid &voxelGrid, const std::vector<SoftwareTexture> &textures, int frameWidth,
	int frameHeight, double *depthBuffer, uint32_t *colorBuffer)
{
	// Much of the code here is duplicated from the initial voxel column drawing method, but
	// there are a couple differences, like the horizontal texture coordinate being flipped,
	// and the drawing orders being slightly modified. The reason for having so much code is
	// so we cover all the different ray casting cases efficiently. The initial voxel column 
	// drawing is its own method because it's the only one that needs to handle back-faces of 
	// wall sides.

	// When clamping Y values for drawing ranges, subtract 0.5 from starts and add 0.5 to 
	// ends before converting to integers because the drawing methods sample at the center 
	// of pixels. The clamping function depends on which side of the range is being clamped; 
	// either way, the drawing range should be contained within the projected range at the 
	// sub-pixel level. This ensures that the vertical texture coordinate is always within 0->1.

	const double heightReal = static_cast<double>(frameHeight);

	// Y position at the base of the player's voxel.
	const double playerYFloor = std::floor(playerY);

	// Voxel Y of the player.
	const int playerVoxelY = static_cast<int>(playerYFloor);

	// Horizontal texture coordinate for the wall. Shared between all voxels in this
	// voxel column.
	const double u = [&nearPoint, wallFacing]()
	{
		if (wallFacing == WallFacing::PositiveX)
		{
			return 1.0 - (nearPoint.y - std::floor(nearPoint.y));
		}
		else if (wallFacing == WallFacing::NegativeX)
		{
			return nearPoint.y - std::floor(nearPoint.y);
		}
		else if (wallFacing == WallFacing::PositiveZ)
		{
			return nearPoint.x - std::floor(nearPoint.x);
		}
		else
		{
			return 1.0 - (nearPoint.x - std::floor(nearPoint.x));
		}
	}();

	// Wall normals behave as they usually would when rendering after the initial voxel 
	// column; no need to cover special cases like wall back faces.
	const Double3 wallNormal = SoftwareRenderer::getWallNormal(wallFacing);

	auto drawVoxel = [x, voxelX, voxelZ, playerY, playerYFloor, &wallNormal, &nearPoint,
		&farPoint, nearZ, farZ, u, &transform, yShear, &shadingInfo, &voxelGrid, &textures,
		frameWidth, frameHeight, heightReal, depthBuffer, colorBuffer]()
	{
		const int voxelY = static_cast<int>(playerYFloor);
		const char voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);

		// 3D points to be used for rendering columns once they are projected.
		const Double3 farCeilingPoint(
			farPoint.x,
			playerYFloor + voxelData.yOffset + voxelData.ySize,
			farPoint.y);
		const Double3 nearCeilingPoint(
			nearPoint.x,
			playerYFloor + voxelData.yOffset + voxelData.ySize,
			nearPoint.y);
		const Double3 farFloorPoint(
			farPoint.x,
			playerYFloor + voxelData.yOffset,
			farPoint.y);
		const Double3 nearFloorPoint(
			nearPoint.x,
			playerYFloor + voxelData.yOffset,
			nearPoint.y);

		// Y position of the player relative to the base of the voxel.
		const double playerYRelative = playerY - playerYFloor;

		// Y screen positions of the projections (unclamped; potentially outside the screen).
		// Once they are clamped and are converted to integers, then they are suitable for 
		// defining drawing ranges.
		const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
			farCeilingPoint, transform, yShear) * heightReal;
		const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
			nearCeilingPoint, transform, yShear) * heightReal;
		const double farFloorScreenY = SoftwareRenderer::getProjectedY(
			farFloorPoint, transform, yShear) * heightReal;
		const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
			nearFloorPoint, transform, yShear) * heightReal;

		// Decide which order to draw each pixel column in, based on the player's Y position.
		if (playerYRelative > (voxelData.yOffset + voxelData.ySize))
		{
			// Above voxel (draw inner floor last).
			const int ceilingStart = std::min(std::max(0,
				static_cast<int>(std::ceil(farCeilingScreenY - 0.50))), frameHeight);
			const int ceilingEnd = std::min(std::max(0,
				static_cast<int>(std::floor(nearCeilingScreenY + 0.50))), frameHeight);
			const int wallStart = ceilingEnd;
			const int wallEnd = std::min(std::max(0,
				static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);
			const int floorStart = std::min(std::max(0,
				static_cast<int>(std::ceil(farFloorScreenY - 0.50))), frameHeight);
			const int floorEnd = wallEnd;

			// 1) Ceiling.
			if (voxelData.ceilingID > 0)
			{
				const Double3 ceilingNormal = Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
					nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
					textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 2) Wall.
			if (voxelData.sideID > 0)
			{
				SoftwareRenderer::drawWall(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
					textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 3) Diagonal 1.
			if (voxelData.diag1ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag1Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 4) Diagonal 2.
			if (voxelData.diag2ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag2Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 5) Inner floor.
			if (voxelData.floorID > 0)
			{
				const Double3 floorNormal = Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
					nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
					textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}
		}
		else if (playerYRelative < voxelData.yOffset)
		{
			// Below voxel (draw inner ceiling last).
			const int ceilingStart = std::min(std::max(0,
				static_cast<int>(std::ceil(nearCeilingScreenY - 0.50))), frameHeight);
			const int ceilingEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farCeilingScreenY + 0.50))), frameHeight);
			const int wallStart = ceilingStart;
			const int wallEnd = std::min(std::max(0,
				static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);
			const int floorStart = wallEnd;
			const int floorEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farFloorScreenY + 0.50))), frameHeight);

			// 1) Wall.
			if (voxelData.sideID > 0)
			{
				SoftwareRenderer::drawWall(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
					textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 2) Floor.
			if (voxelData.floorID > 0)
			{
				const Double3 floorNormal = -Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
					nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
					textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 3) Diagonal 1.
			if (voxelData.diag1ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag1Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 4) Diagonal 2.
			if (voxelData.diag2ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag2Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 5) Inner ceiling.
			if (voxelData.ceilingID > 0)
			{
				const Double3 ceilingNormal = -Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
					nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
					textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}
		}
		else
		{
			// Between top and bottom (draw wall first).
			const int ceilingStart = std::min(std::max(0,
				static_cast<int>(std::ceil(nearCeilingScreenY - 0.50))), frameHeight);
			const int ceilingEnd = std::min(std::max(0,
				static_cast<int>(std::floor(farCeilingScreenY + 0.50))), frameHeight);
			const int wallStart = ceilingStart;
			const int wallEnd = std::min(std::max(0,
				static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);
			const int floorStart = std::min(std::max(0,
				static_cast<int>(std::ceil(farFloorScreenY - 0.50))), frameHeight);
			const int floorEnd = wallEnd;

			// 1) Wall.
			if (voxelData.sideID > 0)
			{
				SoftwareRenderer::drawWall(x, wallStart, wallEnd, nearCeilingScreenY,
					nearFloorScreenY, nearZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
					textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 2) Diagonal 1.
			if (voxelData.diag1ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag1Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY, 
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 3) Diagonal 2.
			if (voxelData.diag2ID > 0)
			{
				DiagonalHit hit;
				const bool success = SoftwareRenderer::findDiag2Intersection(
					voxelX, voxelZ, nearPoint, farPoint, hit);

				if (success)
				{
					double diagTopScreenY, diagBottomScreenY;
					int diagStart, diagEnd;

					// Assign the diagonal projection values to the declared variables.
					SoftwareRenderer::diagonalProjection(playerYFloor, voxelData, hit.point,
						transform, yShear, frameHeight, heightReal, diagTopScreenY,
						diagBottomScreenY, diagStart, diagEnd);

					SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
						diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
						voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
						shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
				}
			}

			// 4) Inner ceiling.
			if (voxelData.ceilingID > 0)
			{
				const Double3 ceilingNormal = -Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, nearCeilingScreenY,
					farCeilingScreenY, nearPoint, farPoint, nearZ, farZ, ceilingNormal,
					textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}

			// 5) Inner floor.
			if (voxelData.floorID > 0)
			{
				const Double3 floorNormal = Double3::UnitY;
				SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
					nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
					textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
					depthBuffer, colorBuffer);
			}
		}
	};

	auto drawVoxelBelow = [x, voxelX, voxelZ, &wallNormal, &nearPoint, &farPoint,
		nearZ, farZ, u, &transform, yShear, &shadingInfo, &voxelGrid, &textures, frameWidth,
		frameHeight, heightReal, depthBuffer, colorBuffer](int voxelY)
	{
		const char voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelYReal = static_cast<double>(voxelY);

		// 3D points to be used for rendering columns once they are projected.
		const Double3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			farPoint.y);
		const Double3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			nearPoint.y);
		const Double3 farFloorPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset,
			farPoint.y);
		const Double3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset,
			nearPoint.y);

		// Y screen positions of the projections (unclamped; potentially outside the screen).
		// Once they are clamped and are converted to integers, then they are suitable for 
		// defining drawing ranges.
		const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
			farCeilingPoint, transform, yShear) * heightReal;
		const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
			nearCeilingPoint, transform, yShear) * heightReal;
		const double farFloorScreenY = SoftwareRenderer::getProjectedY(
			farFloorPoint, transform, yShear) * heightReal;
		const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
			nearFloorPoint, transform, yShear) * heightReal;

		const int ceilingStart = std::min(std::max(0,
			static_cast<int>(std::ceil(farCeilingScreenY - 0.50))), frameHeight);
		const int ceilingEnd = std::min(std::max(0,
			static_cast<int>(std::floor(nearCeilingScreenY + 0.50))), frameHeight);
		const int wallStart = ceilingEnd;
		const int wallEnd = std::min(std::max(0,
			static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);
		const int floorStart = std::min(std::max(0,
			static_cast<int>(std::ceil(farFloorScreenY - 0.50))), frameHeight);
		const int floorEnd = wallEnd;

		// 1) Ceiling.
		if (voxelData.ceilingID > 0)
		{
			const Double3 ceilingNormal = Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, farCeilingScreenY,
				nearCeilingScreenY, farPoint, nearPoint, farZ, nearZ, ceilingNormal,
				textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 2) Wall.
		if (voxelData.sideID > 0)
		{
			SoftwareRenderer::drawWall(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
				textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 3) Diagonal 1.
		if (voxelData.diag1ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag1Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}

		// 4) Diagonal 2.
		if (voxelData.diag2ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag2Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}

		// 5) Inner floor.
		if (voxelData.floorID > 0)
		{
			const Double3 floorNormal = Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, farFloorScreenY,
				nearFloorScreenY, farPoint, nearPoint, farZ, nearZ, floorNormal,
				textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}
	};

	auto drawVoxelAbove = [x, voxelX, voxelZ, &wallNormal, &nearPoint, &farPoint,
		nearZ, farZ, u, &transform, yShear, &shadingInfo, &voxelGrid, &textures, frameWidth,
		frameHeight, heightReal, depthBuffer, colorBuffer](int voxelY)
	{
		const char voxelID = voxelGrid.getVoxels()[voxelX + (voxelY * voxelGrid.getWidth()) +
			(voxelZ * voxelGrid.getWidth() * voxelGrid.getHeight())];
		const VoxelData &voxelData = voxelGrid.getVoxelData(voxelID);
		const double voxelYReal = static_cast<double>(voxelY);

		// 3D points to be used for rendering columns once they are projected.
		const Double3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			farPoint.y);
		const Double3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset + voxelData.ySize,
			nearPoint.y);
		const Double3 farFloorPoint(
			farPoint.x,
			voxelYReal + voxelData.yOffset,
			farPoint.y);
		const Double3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + voxelData.yOffset,
			nearPoint.y);

		// Y screen positions of the projections (unclamped; potentially outside the screen).
		// Once they are clamped and are converted to integers, then they are suitable for 
		// defining drawing ranges.
		const double farCeilingScreenY = SoftwareRenderer::getProjectedY(
			farCeilingPoint, transform, yShear) * heightReal;
		const double nearCeilingScreenY = SoftwareRenderer::getProjectedY(
			nearCeilingPoint, transform, yShear) * heightReal;
		const double farFloorScreenY = SoftwareRenderer::getProjectedY(
			farFloorPoint, transform, yShear) * heightReal;
		const double nearFloorScreenY = SoftwareRenderer::getProjectedY(
			nearFloorPoint, transform, yShear) * heightReal;

		const int ceilingStart = std::min(std::max(0,
			static_cast<int>(std::ceil(nearCeilingScreenY - 0.50))), frameHeight);
		const int ceilingEnd = std::min(std::max(0,
			static_cast<int>(std::floor(farCeilingScreenY + 0.50))), frameHeight);
		const int wallStart = ceilingStart;
		const int wallEnd = std::min(std::max(0,
			static_cast<int>(std::floor(nearFloorScreenY + 0.50))), frameHeight);
		const int floorStart = wallEnd;
		const int floorEnd = std::min(std::max(0,
			static_cast<int>(std::floor(farFloorScreenY + 0.50))), frameHeight);

		// 1) Floor.
		if (voxelData.floorID > 0)
		{
			const Double3 floorNormal = -Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, floorStart, floorEnd, nearFloorScreenY,
				farFloorScreenY, nearPoint, farPoint, nearZ, farZ, floorNormal,
				textures.at(voxelData.floorID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 2) Wall.
		if (voxelData.sideID > 0)
		{
			SoftwareRenderer::drawWall(x, wallStart, wallEnd, nearCeilingScreenY,
				nearFloorScreenY, nearZ, u, voxelData.topV, voxelData.bottomV, wallNormal,
				textures.at(voxelData.sideID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}

		// 3) Diagonal 1.
		if (voxelData.diag1ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag1Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag1ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}

		// 4) Diagonal 2.
		if (voxelData.diag2ID > 0)
		{
			DiagonalHit hit;
			const bool success = SoftwareRenderer::findDiag2Intersection(
				voxelX, voxelZ, nearPoint, farPoint, hit);

			if (success)
			{
				double diagTopScreenY, diagBottomScreenY;
				int diagStart, diagEnd;

				// Assign the diagonal projection values to the declared variables.
				SoftwareRenderer::diagonalProjection(voxelYReal, voxelData, hit.point,
					transform, yShear, frameHeight, heightReal, diagTopScreenY,
					diagBottomScreenY, diagStart, diagEnd);

				SoftwareRenderer::drawWall(x, diagStart, diagEnd, diagTopScreenY,
					diagBottomScreenY, nearZ + hit.innerZ, hit.u, voxelData.topV, 
					voxelData.bottomV, hit.normal, textures.at(voxelData.diag2ID - 1), 
					shadingInfo, frameWidth, frameHeight, depthBuffer, colorBuffer);
			}
		}
		
		// 5) Inner ceiling.
		if (voxelData.ceilingID > 0)
		{
			const Double3 ceilingNormal = -Double3::UnitY;
			SoftwareRenderer::drawFloorOrCeiling(x, ceilingStart, ceilingEnd, nearCeilingScreenY,
				farCeilingScreenY, nearPoint, farPoint, nearZ, farZ, ceilingNormal,
				textures.at(voxelData.ceilingID - 1), shadingInfo, frameWidth, frameHeight,
				depthBuffer, colorBuffer);
		}
	};

	// Draw voxel straight ahead first.
	drawVoxel();

	// Draw voxels below the voxel.
	for (int voxelY = (playerVoxelY - 1); voxelY >= 0; voxelY--)
	{
		drawVoxelBelow(voxelY);
	}

	// Draw voxels above the voxel.
	for (int voxelY = (playerVoxelY + 1); voxelY < voxelGrid.getHeight(); voxelY++)
	{
		drawVoxelAbove(voxelY);
	}
}

void SoftwareRenderer::drawFlat(int startX, int endX, const Flat::Frame &flatFrame,
	const Double3 &normal, bool flipped, const Double2 &eye, const ShadingInfo &shadingInfo,
	const SoftwareTexture &texture, int frameWidth, int frameHeight,
	double *depthBuffer, uint32_t *colorBuffer)
{
	// Contribution from the sun.
	const double lightNormalDot = std::max(0.0, shadingInfo.sunDirection.dot(normal));
	const Double3 sunComponent = (shadingInfo.sunColor * lightNormalDot).clamped(
		0.0, 1.0 - shadingInfo.ambient);

	// X percents across the screen for the given start and end columns.
	const double startXPercent = (static_cast<double>(startX) + 0.50) / 
		static_cast<double>(frameWidth);
	const double endXPercent = (static_cast<double>(endX) + 0.50) /
		static_cast<double>(frameWidth);

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
	const double startU = std::max(std::min(startFlatPercent, 
		SoftwareRenderer::JUST_BELOW_ONE), 0.0);
	const double endU = std::max(std::min(endFlatPercent, 
		SoftwareRenderer::JUST_BELOW_ONE), 0.0);

	// Get the start and end coordinates of the projected points (Y values potentially
	// outside the screen).
	const double widthReal = static_cast<double>(frameWidth);
	const double heightReal = static_cast<double>(frameHeight);
	const double projectedXStart = clampedStartXPercent * widthReal;
	const double projectedXEnd = clampedEndXPercent * widthReal;
	const double projectedYStart = flatFrame.startY * heightReal;
	const double projectedYEnd = flatFrame.endY * heightReal;

	// Clamp the coordinates for where the flat starts and stops on the screen.
	const int xStart = std::min(std::max(0,
		static_cast<int>(std::ceil(projectedXStart - 0.50))), frameWidth);
	const int xEnd = std::min(std::max(0,
		static_cast<int>(std::floor(projectedXEnd + 0.50))), frameWidth);
	const int yStart = std::min(std::max(0,
		static_cast<int>(std::ceil(projectedYStart - 0.50))), frameHeight);
	const int yEnd = std::min(std::max(0,
		static_cast<int>(std::floor(projectedYEnd + 0.50))), frameHeight);

	// Draw by-column, similar to wall rendering.
	for (int x = xStart; x < xEnd; x++)
	{
		const double xPercent = ((static_cast<double>(x) + 0.50) - projectedXStart) /
			(projectedXEnd - projectedXStart);

		// Horizontal texture coordinate.
		const double u = startU + ((endU - startU) * xPercent);

		// Horizontal texel position.
		const int textureX = static_cast<int>(
			(flipped ? (SoftwareRenderer::JUST_BELOW_ONE - u) : u) *
			static_cast<double>(texture.width));

		const Double3 topPoint = startTopPoint.lerp(endTopPoint, xPercent);

		// Get the true XZ distance for the depth.
		const double z = (Double2(topPoint.x, topPoint.z) - eye).length();

		for (int y = yStart; y < yEnd; y++)
		{
			const int index = x + (y * frameWidth);

			// Vertical texture coordinate.
			const double v = ((static_cast<double>(y) + 0.50) - projectedYStart) /
				(projectedYEnd - projectedYStart);

			// Vertical texel position.
			const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

			// Linearly interpolated fog.
			const double fogPercent = std::min(z / shadingInfo.fogDistance, 1.0);
			const Double3 &fogColor = shadingInfo.horizonSkyColor;

			if (z <= depthBuffer[index])
			{
				const Double4 &texel = texture.pixels[textureX + (textureY * texture.width)];

				// Draw only if the texel is not transparent.
				if (texel.w > 0.0)
				{
					const Double3 color(
						texel.x * (shadingInfo.ambient + sunComponent.x),
						texel.y * (shadingInfo.ambient + sunComponent.y),
						texel.z * (shadingInfo.ambient + sunComponent.z));

					colorBuffer[index] = color.lerp(fogColor, fogPercent).clamped().toRGB();
					depthBuffer[index] = z;
				}
			}
		}
	}
}

void SoftwareRenderer::rayCast2D(int x, const Double3 &eye, const Double2 &direction,
	const Matrix4d &transform, double yShear, const ShadingInfo &shadingInfo,
	const VoxelGrid &voxelGrid, uint32_t *colorBuffer)
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

	// The Z distance from the camera to the wall, and the X or Z normal of the intersected
	// voxel face. The first Z distance is a special case, so it's brought outside the 
	// DDA loop.
	double zDistance;
	WallFacing wallFacing;

	// Verify that the initial voxel coordinate is within the world bounds.
	bool voxelIsValid = (startCell.x >= 0) && (startCell.y >= 0) && (startCell.z >= 0) &&
		(startCell.x < voxelGrid.getWidth()) && (startCell.y < voxelGrid.getHeight()) && 
		(startCell.z < voxelGrid.getDepth());

	if (voxelIsValid)
	{
		// Get the initial voxel ID and see how it should be rendered.
		const char initialVoxelID = voxels[startCell.x + (startCell.y * voxelGrid.getWidth()) +
			(startCell.z * voxelGrid.getWidth() * voxelGrid.getHeight())];

		// Decide how far the wall is, and which voxel face was hit.
		if (sideDistX < sideDistZ)
		{
			zDistance = sideDistX;
			wallFacing = nonNegativeDirX ? WallFacing::NegativeX : WallFacing::PositiveX;
		}
		else
		{
			zDistance = sideDistZ;
			wallFacing = nonNegativeDirZ ? WallFacing::NegativeZ : WallFacing::PositiveZ;
		}

		// The initial near point is directly in front of the player in the near Z 
		// camera plane.
		const Double2 initialNearPoint(
			eye.x + (dirX * SoftwareRenderer::NEAR_PLANE),
			eye.z + (dirZ * SoftwareRenderer::NEAR_PLANE));

		// The initial far point is the wall hit. This is used with the player's position 
		// for drawing the initial floor and ceiling.
		const Double2 initialFarPoint(
			eye.x + (dirX * zDistance),
			eye.z + (dirZ * zDistance));

		// Draw all voxels in a column at the player's XZ coordinate.
		SoftwareRenderer::drawInitialVoxelColumn(x, startCell.x, startCell.z, eye.y,
			wallFacing, initialNearPoint, initialFarPoint, SoftwareRenderer::NEAR_PLANE, 
			zDistance, transform, yShear, shadingInfo, voxelGrid, this->textures, this->width, 
			this->height, this->depthBuffer.data(), colorBuffer);
	}

	// The current voxel coordinate in the DDA loop. For all intents and purposes,
	// the Y cell coordinate is constant.
	Int3 cell(startCell.x, startCell.y, startCell.z);

	// Lambda for stepping to the next XZ coordinate in the grid and updating the Z
	// distance for the current edge point.
	auto doDDAStep = [&sideDistX, &sideDistZ, &cell, &wallFacing, &voxelIsValid, &zDistance,
		deltaDistX, deltaDistZ, stepX, stepZ, dirX, dirZ, nonNegativeDirX, nonNegativeDirZ,
		&eye, &voxelGrid]()
	{
		if (sideDistX < sideDistZ)
		{
			sideDistX += deltaDistX;
			cell.x += stepX;
			wallFacing = nonNegativeDirX ? WallFacing::NegativeX : WallFacing::PositiveX;
			voxelIsValid &= (cell.x >= 0) && (cell.x < voxelGrid.getWidth());
		}
		else
		{
			sideDistZ += deltaDistZ;
			cell.z += stepZ;
			wallFacing = nonNegativeDirZ ? WallFacing::NegativeZ : WallFacing::PositiveZ;
			voxelIsValid &= (cell.z >= 0) && (cell.z < voxelGrid.getDepth());
		}

		zDistance = ((wallFacing == WallFacing::PositiveX) || (wallFacing == WallFacing::NegativeX)) ?
			(static_cast<double>(cell.x) - eye.x + static_cast<double>((1 - stepX) / 2)) / dirX :
			(static_cast<double>(cell.z) - eye.z + static_cast<double>((1 - stepZ) / 2)) / dirZ;
	};

	// Step forward in the grid once to leave the initial voxel and update the Z distance.
	doDDAStep();

	// Step through the voxel grid while the current coordinate is valid and
	// the distance stepped is less than the distance at which fog is maximum.
	while (voxelIsValid && (zDistance < this->fogDistance))
	{
		// Store the cell coordinates, axis, and Z distance for wall rendering. The
		// loop needs to do another DDA step to calculate the far point.
		const int savedCellX = cell.x;
		const int savedCellZ = cell.z;
		const WallFacing savedNormal = wallFacing;
		const double wallDistance = zDistance;

		// Decide which voxel in the XZ plane to step to next, and update the Z distance.
		doDDAStep();

		// Near and far points in the XZ plane. The near point is where the wall is, and 
		// the far point is used with the near point for drawing the floor and ceiling.
		const Double2 nearPoint(
			eye.x + (dirX * wallDistance),
			eye.z + (dirZ * wallDistance));
		const Double2 farPoint(
			eye.x + (dirX * zDistance),
			eye.z + (dirZ * zDistance));

		// Draw all voxels in a column at the given XZ coordinate.
		SoftwareRenderer::drawVoxelColumn(x, savedCellX, savedCellZ, eye.y, savedNormal,
			nearPoint, farPoint, wallDistance, zDistance, transform, yShear, shadingInfo, 
			voxelGrid, this->textures, this->width, this->height, this->depthBuffer.data(),
			colorBuffer);
	}
}

void SoftwareRenderer::render(const Double3 &eye, const Double3 &direction, double fovY,
	double ambient, double daytimePercent, const VoxelGrid &voxelGrid, uint32_t *colorBuffer)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	// Camera values for rendering. We trick the 2.5D ray caster into thinking the player is 
	// always looking straight forward, but we use the Y component of the player's direction 
	// to offset projected coordinates via Y-shearing. Assume "direction" is normalized.
	const Double3 forwardXZ = Double3(direction.x, 0.0, direction.z).normalized();
	const Double3 rightXZ = forwardXZ.cross(Double3::UnitY).normalized();
	const Double3 up = Double3::UnitY;

	// Zoom of the camera, based on vertical field of view.
	const double zoom = 1.0 / std::tan((fovY * 0.5) * Constants::DegToRad);

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
	const double yShear = [&direction, zoom]()
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
		return std::tan(angleRadians) * zoom;
	}();

	// Camera values for generating 2D rays with.
	const Double2 forwardComp = Double2(forwardXZ.x, forwardXZ.z).normalized() * zoom;
	const Double2 right2D = Double2(rightXZ.x, rightXZ.z).normalized() * aspect;

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

	const ShadingInfo shadingInfo(horizonFogColor, zenithFogColor, sunColor, 
		sunDirection, ambient, this->fogDistance);

	// Lambda for rendering some columns of pixels. The voxel rendering portion uses 2.5D 
	// ray casting, which is the cheaper form of ray casting (although still not very 
	// efficient overall), and results in a "fake" 3D scene.
	auto renderColumns = [this, &eye, &voxelGrid, colorBuffer, &shadingInfo, widthReal, 
		&transform, yShear, &forwardComp, &right2D](int startX, int endX)
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

			// Cast the 2D ray and fill in the column's pixels with color.
			this->rayCast2D(x, eye, direction, transform, yShear, shadingInfo, 
				voxelGrid, colorBuffer);
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
			const SoftwareTexture &texture = textures[flat.textureID];

			const Double2 eye2D(eye.x, eye.z);

			SoftwareRenderer::drawFlat(startX, endX, flatFrame, flatNormal, flat.flipped, 
				eye2D, shadingInfo, texture, this->width, this->height, 
				this->depthBuffer.data(), colorBuffer);
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
	std::thread sortThread([this, &eye, &forwardXZ, &transform, yShear, aspect, zoom]
	{ 
		const Double2 eye2D(eye.x, eye.z);
		const Double2 direction2D(forwardXZ.x, forwardXZ.z);
		this->updateVisibleFlats(eye2D, direction2D, transform, yShear, aspect, zoom);
	});

	// Prepare render threads. These are used for clearing the frame buffer and rendering.
	std::vector<std::thread> renderThreads(this->renderThreadCount);

	// Start clearing the frame buffer with the render threads.
	for (size_t i = 0; i < renderThreads.size(); i++)
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
	for (size_t i = 0; i < renderThreads.size(); i++)
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
