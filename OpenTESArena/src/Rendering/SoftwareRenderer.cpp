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

SoftwareRenderer::SoftwareRenderer(int width, int height)
{
	// Initialize 2D frame buffers.
	const int pixelCount = width * height;
	this->colorBuffer = std::vector<uint32_t>(pixelCount);
	this->zBuffer = std::vector<double>(pixelCount);
	std::fill(this->colorBuffer.begin(), this->colorBuffer.end(), 0);
	std::fill(this->zBuffer.begin(), this->zBuffer.end(), 0);

	this->width = width;
	this->height = height;

	// Obtain the number of threads to use. "hardware_concurrency()" might return 0,
	// so it needs to be clamped positive.
	this->renderThreadCount = static_cast<int>(std::thread::hardware_concurrency());
	if (this->renderThreadCount == 0)
	{
		Debug::mention("Software Renderer", "hardware_concurrency() returned 0.");
		this->renderThreadCount = 1;
	}

	// Initialize camera values to "empty".
	this->transform = Matrix4d();
	this->eye = Double3();
	this->forward = Double3();
	this->fovY = 0.0;

	this->viewDistance = 0.0;
	this->viewDistSquared = 0.0;

	// Initialize start cell to "empty".
	this->startCellReal = Double3();
	this->startCell = Int3();

	// Pick an arbitrary fog color. Later, the DAYTIME.COL palette should be used
	// for sky color interpolation as the day progresses.
	this->fogColor = Double3(0.45, 0.75, 1.0);

	// -- test --
	// Throw some test flats into the world.
	for (int k = 4; k < 16; ++k)
	{
		for (int i = 4; i < 16; ++i)
		{
			this->addFlat(Double3(i, 1.0, k), Double2(-1.0, 0.0), 0.8, 0.9, i % 3);
		}
	}
	// -- end test --
}

SoftwareRenderer::~SoftwareRenderer()
{

}

const uint32_t *SoftwareRenderer::getPixels() const
{
	return this->colorBuffer.data();
}

void SoftwareRenderer::setEye(const Double3 &eye)
{
	this->eye = eye;
}

void SoftwareRenderer::setForward(const Double3 &forward)
{
	// All camera axes should be normalized.
	this->forward = forward.normalized();
}

void SoftwareRenderer::setFovY(double fovY)
{
	this->fovY = fovY;
}

void SoftwareRenderer::setViewDistance(double viewDistance)
{
	this->viewDistance = viewDistance;
	this->viewDistSquared = viewDistance * viewDistance;
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

int SoftwareRenderer::addFlat(const Double3 &position, const Double2 &direction,
	double width, double height, int textureID)
{
	// Search for the next available flat ID.
	int id = 0;
	while (this->flats.find(id) != this->flats.end())
	{
		id++;
	}

	SoftwareRenderer::Flat flat;
	flat.position = position;
	flat.direction = direction;
	flat.width = width;
	flat.height = height;
	flat.textureID = textureID;

	// Add the flat (sprite, door, store sign, etc.).
	this->flats.insert(std::make_pair(id, flat));

	return id;
}

void SoftwareRenderer::updateFlat(int id, const Double3 *position, const Double2 *direction,
	const double *width, const double *height, const int *textureID)
{
	const auto flatIter = this->flats.find(id);
	Debug::check(flatIter != this->flats.end(), "Software Renderer",
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
}

void SoftwareRenderer::removeFlat(int id)
{
	// Make sure the flat exists before removing it.
	const auto flatIter = this->flats.find(id);
	Debug::check(flatIter != this->flats.end(), "Software Renderer",
		"Cannot remove a non-existent flat (" + std::to_string(id) + ").");

	this->flats.erase(flatIter);
}

void SoftwareRenderer::resize(int width, int height)
{
	const int pixelCount = width * height;
	this->colorBuffer.resize(pixelCount);
	this->zBuffer.resize(pixelCount);
	std::fill(this->colorBuffer.begin(), this->colorBuffer.end(), 0);
	std::fill(this->zBuffer.begin(), this->zBuffer.end(), 0);

	this->width = width;
	this->height = height;
}

void SoftwareRenderer::updateVisibleFlats()
{
	this->visibleFlats.clear();

	for (const auto &pair : this->flats)
	{
		const Flat &flat = pair.second;

		// Get the flat's axes. UnitY is "global up".
		const Double3 flatForward = Double3(flat.direction.x, 0.0, flat.direction.y).normalized();
		const Double3 flatUp = Double3::UnitY;
		const Double3 flatRight = flatForward.cross(flatUp).normalized();

		const Double3 flatRightScaled = flatRight * (flat.width * 0.50);
		const Double3 flatUpScaled = flatUp * flat.height;

		// Calculate the four corners of the flat in world space.
		const Double3 topLeft = flat.position - flatRightScaled + flatUpScaled;
		const Double3 topRight = flat.position + flatRightScaled + flatUpScaled;
		const Double3 bottomLeft = flat.position - flatRightScaled;
		const Double3 bottomRight = flat.position + flatRightScaled;

		// Transform the points to camera space (projection * view).
		Double4 p1 = this->transform * Double4(topLeft.x, topLeft.y, topLeft.z, 1.0);
		Double4 p2 = this->transform * Double4(topRight.x, topRight.y, topRight.z, 1.0);
		Double4 p3 = this->transform * Double4(bottomLeft.x, bottomLeft.y, bottomLeft.z, 1.0);
		Double4 p4 = this->transform * Double4(bottomRight.x, bottomRight.y, bottomRight.z, 1.0);

		// Create fresh projection data for the flat by projecting the points to the 
		// viewing plane. Also take camera elevation into account.
		Flat::ProjectionData projectionData;
		const double cameraElevation = this->forward.y;

		// Get Z distances.
		projectionData.leftZ = p1.z;
		projectionData.rightZ = p2.z;

		// Convert to normalized coordinates.
		p1 = p1 / p1.w;
		p2 = p2 / p2.w;
		p3 = p3 / p3.w;
		p4 = p4 / p4.w;

		// Translate coordinates on the screen relative to the middle (0.5, 0.5).
		// Multiply by 0.5 to apply the correct aspect ratio.
		projectionData.leftX = 0.50 + (p1.x * 0.50);
		projectionData.rightX = 0.50 + (p2.x * 0.50);
		projectionData.topLeftY = (0.50 + cameraElevation) - (p1.y * 0.50);
		projectionData.topRightY = (0.50 + cameraElevation) - (p2.y * 0.50);
		projectionData.bottomLeftY = (0.50 + cameraElevation) - (p3.y * 0.50);
		projectionData.bottomRightY = (0.50 + cameraElevation) - (p4.y * 0.50);

		// The flat is visible if at least one of the Z values is positive and
		// the vertical edges are within bounds.
		const bool leftZPositive = projectionData.leftZ > 0.0;
		const bool rightZPositive = projectionData.rightZ > 0.0;
		const bool rightEdgeVisible =
			(projectionData.rightX >= 0.0) || (projectionData.rightX < 1.0) &&
			((projectionData.topRightY < 1.0) || (projectionData.bottomRightY >= 0.0));
		const bool leftEdgeVisible =
			(projectionData.leftX >= 0.0) || (projectionData.leftX < 1.0) &&
			((projectionData.topLeftY < 1.0) || (projectionData.bottomLeftY >= 0.0));

		// - To do: make this code more correct. It should not reject points
		//   when an edge is visible (i.e., top is above screen, bottom is below screen).
		// - I also think some more robust rasterization practices might need to be included,
		//   so that flats intersecting the viewing plane are rendered correctly. For example,
		//   clipping anything with negative Z and interpolating the new texture coordinates...? 
		//   Just an idea. Right now it throws away flats partially behind the view plane.
		if ((leftZPositive && rightZPositive) && (rightEdgeVisible || leftEdgeVisible))
		{
			this->visibleFlats.push_back(std::make_pair(&flat, projectionData));
		}
	}

	// Sort the visible flat data farthest to nearest (this may be relevant for
	// transparencies).
	std::sort(this->visibleFlats.begin(), this->visibleFlats.end(),
		[](const std::pair<const Flat*, Flat::ProjectionData> &a,
			const std::pair<const Flat*, Flat::ProjectionData> &b)
	{
		return std::min(a.second.leftZ, a.second.rightZ) >
			std::min(b.second.leftZ, b.second.rightZ);
	});
}

Double3 SoftwareRenderer::castRay(const Double3 &direction,
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
}

void SoftwareRenderer::castRay(const Double2 &direction,
	const VoxelGrid &voxelGrid, int x)
{
	// This is the "classic" 2.5D version of ray casting, based on Lode Vandevenne's 
	// ray caster. It will also need to allow multiple floors, variable eye height,
	// and flats with arbitrary angles around the Y axis (doors, etc.). I think a
	// 2D depth buffer will be required, too.

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

	int stepX, stepZ;
	double sideDistX, sideDistZ;
	if (nonNegativeDirX)
	{
		stepX = 1;
		sideDistX = (this->startCellReal.x + 1.0 - this->eye.x) * deltaDistX;
	}
	else
	{
		stepX = -1;
		sideDistX = (this->eye.x - this->startCellReal.x) * deltaDistX;
	}

	if (nonNegativeDirZ)
	{
		stepZ = 1;
		sideDistZ = (this->startCellReal.z + 1.0 - this->eye.z) * deltaDistZ;
	}
	else
	{
		stepZ = -1;
		sideDistZ = (this->eye.z - this->startCellReal.z) * deltaDistZ;
	}

	// Make a copy of the initial side distances for the special case of ending in
	// the first voxel. This is the oblique distance though, so some changes will
	// be necessary.
	const double initialSideDistX = sideDistX;
	const double initialSideDistZ = sideDistZ;

	const int gridWidth = voxelGrid.getWidth();
	const int gridHeight = voxelGrid.getHeight();
	const int gridDepth = voxelGrid.getDepth();

	int cellX = this->startCell.x;
	const int cellY = this->startCell.y; // Constant for now.
	int cellZ = this->startCell.z;

	// Axis enum. Eventually, Y will be added for floors and ceilings.
	enum class Axis { X, Z };
	Axis axis = Axis::X;

	// Pointer to voxel ID grid data.
	const char *voxels = voxelGrid.getVoxels();

	// Voxel ID of a hit voxel, if any. Zero is "air".
	char hitID = 0;

	// Step through the voxel grid while the current coordinate is valid.
	bool voxelIsValid = (cellX >= 0) && (cellY >= 0) && (cellZ >= 0) &&
		(cellX < gridWidth) && (cellY < gridHeight) && (cellZ < gridDepth);

	while (voxelIsValid)
	{
		// Get the index of the current voxel in the voxel grid.
		const int gridIndex = cellX + (cellY * gridWidth) +
			(cellZ * gridWidth * gridHeight);

		// Check if the current voxel is solid.
		const char voxelID = voxels[gridIndex];

		if (voxelID > 0)
		{
			hitID = voxelID;
			break;
		}

		if (sideDistX < sideDistZ)
		{
			sideDistX += deltaDistX;
			cellX += stepX;
			axis = Axis::X;
			voxelIsValid &= (cellX >= 0) && (cellX < gridWidth);
		}
		else
		{
			sideDistZ += deltaDistZ;
			cellZ += stepZ;
			axis = Axis::Z;
			voxelIsValid &= (cellZ >= 0) && (cellZ < gridDepth);
		}
	}

	// If hit ID is positive, a wall was hit.
	if (hitID > 0)
	{
		// Boolean for whether the stepping stopped in the first voxel.
		const bool stoppedInFirstVoxel = (cellX == this->startCell.x) &&
			(cellZ == this->startCell.z);

		// The "z-distance" from the camera to the wall. It's a special case if the
		// stepping stopped in the first voxel.
		double zDistance;
		if (stoppedInFirstVoxel)
		{
			// To do: figure out the correct initial side distance calculation.
			// - Maybe try to avoid a square root with direction?
			if (initialSideDistX < initialSideDistZ)
			{
				zDistance = initialSideDistX;
				axis = Axis::X;
			}
			else
			{
				zDistance = initialSideDistZ;
				axis = Axis::Z;
			}
		}
		else
		{
			zDistance = (axis == Axis::X) ?
				(static_cast<double>(cellX) - this->eye.x + static_cast<double>((1 - stepX) / 2)) / dirX :
				(static_cast<double>(cellZ) - this->eye.z + static_cast<double>((1 - stepZ) / 2)) / dirZ;
		}

		// Boolean for whether the intersected voxel face is a back face.
		const bool backFace = stoppedInFirstVoxel;

		// 3D hit point on the wall from casting a ray along the XZ plane.
		const Double3 hitPoint(
			this->eye.x + (dirX * zDistance),
			this->eye.y,
			this->eye.z + (dirZ * zDistance));

		// Horizontal texture coordinate (constant for all wall pixels in a column).
		// - Remember to watch out for the edge cases where u == 1.0, resulting in an
		//   out-of-bounds texel access. Maybe use std::nextafter() for ~0.9999999?
		double u;
		if (axis == Axis::X)
		{
			const double uVal = hitPoint.z - std::floor(hitPoint.z);
			u = (nonNegativeDirX ^ backFace) ? uVal : (1.0 - uVal);
		}
		else
		{
			const double uVal = hitPoint.x - std::floor(hitPoint.x);
			u = (nonNegativeDirZ ^ backFace) ? (1.0 - uVal) : uVal;
		}

		// Generate a point on the ceiling edge and floor edge of the wall relative
		// to the hit point.
		const Double3 ceilingPoint(hitPoint.x, std::ceil(hitPoint.y), hitPoint.z);
		const Double3 floorPoint(hitPoint.x, std::floor(hitPoint.y), hitPoint.z);

		// Transform the points to camera space (projection * view).
		Double4 p1 = this->transform * Double4(ceilingPoint.x, ceilingPoint.y, ceilingPoint.z, 1.0);
		Double4 p2 = this->transform * Double4(floorPoint.x, floorPoint.y, floorPoint.z, 1.0);

		// Convert to normalized coordinates.
		p1 = p1 / p1.w;
		p2 = p2 / p2.w;

		// Camera elevation, as I call it, is simply the Y component of the player's 3D direction.
		// In 2.5D rendering, it affects "fake" looking up and down (a.k.a. Y-shearing), and 
		// its magnitude must be clamped less than 1 because 1 would imply the player is looking 
		// straight up or down, which is impossible (the viewing frustum would have a volume of 0). 
		// Its value could be clamped within some range, like [-0.3, 0.3], depending on how far 
		// the player should be able to look up and down, and how tolerable the skewing is.
		// - This value will usually be non-zero in the "modern" interface mode.
		// - Maybe it should involve the angle between horizontal and vertical, because if
		//   the player is looking halfway between, then the elevation would be sqrt(2) / 2.
		const double cameraElevation = this->forward.y;

		// Translate the Y coordinates relative to the center of Y projection (y == 0.5).
		// Add camera elevation for "fake" looking up and down. Multiply by 0.5 to apply the 
		// correct aspect ratio.
		// - Since the ray cast guarantees the intersection to be in the correct column
		//   of the screen, only the Y coordinates need to be projected.
		const double projectedY1 = (0.50 + cameraElevation) - (p1.y * 0.50);
		const double projectedY2 = (0.50 + cameraElevation) - (p2.y * 0.50);

		// Get the start and end Y pixel coordinates of the projected points (potentially
		// outside the top or bottom of the screen).
		const double heightReal = static_cast<double>(this->height);
		const int projectedStart = static_cast<int>(std::round(projectedY1 * heightReal));
		const int projectedEnd = static_cast<int>(std::round(projectedY2 * heightReal));

		// Clamp the Y coordinates for where the wall starts and stops on the screen.
		const int drawStart = std::max(0, projectedStart);
		const int drawEnd = std::min(this->height, projectedEnd);

		// The texture associated with the voxel ID. Subtract 1 because the lowest hit ID
		// is 1 and textures start at 0.
		const TextureData &texture = this->textures[hitID - 1];

		// X position in texture (temporarily using modulo to protect against edge cases 
		// where u == 1.0; it should be fixed in the u calculation instead).
		const int textureX = static_cast<int>(u *
			static_cast<double>(texture.width)) % texture.width;

		// Linearly interpolated fog.
		const double fogPercent = std::min(zDistance / this->viewDistance, 1.0);

		// Draw each wall pixel in the column.
		uint32_t *pixels = this->colorBuffer.data();
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
				const Double3 color(texel.x, texel.y, texel.z);

				pixels[index] = color.lerp(this->fogColor, fogPercent).clamped().toRGB();
				depth[index] = zDistance;
			}
		}
	}

	// Floor/ceiling...
	// - I wonder how it should start if no wall was hit. Maybe project the floor based on
	//   where the stepping stopped to get a Y coordinate?
	// - Do not depend on when a wall on the current floor is hit. Floor/ceiling casting
	//   can continue as far as it needs to.

	// Sprites.
	// - Sprites *could* be done outside the ray casting loop, though they would need to be
	//   parallelized a bit differently then. Here, it is easy to parallelize by column,
	//   so it might be faster in practice, even if a little redundant work is done.

	// - To do: go through all of this again and verify the math for correctness.
	for (const auto &pair : this->visibleFlats)
	{
		const Flat &flat = *pair.first;
		const Flat::ProjectionData &projectionData = pair.second;

		// X percent across the screen.
		const double xPercent = static_cast<double>(x) /
			static_cast<double>(this->width);

		// Find where the column is within the X range of the flat.
		const double xRangePercent = (xPercent - projectionData.rightX) /
			(projectionData.leftX - projectionData.rightX);

		// Don't render the flat if the X range percent is invalid.
		if ((xRangePercent < 0.0) || (xRangePercent >= 1.0))
		{
			continue;
		}

		// Horizontal texture coordinate in the flat.
		const double u = xRangePercent;

		// Interpolate the projected Y coordinates based on the X range.
		const double projectedY1 = projectionData.topRightY +
			((projectionData.topLeftY - projectionData.topRightY) * xRangePercent);
		const double projectedY2 = projectionData.bottomRightY +
			((projectionData.bottomLeftY - projectionData.bottomRightY) * xRangePercent);

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
		const int textureX = static_cast<int>(u *
			static_cast<double>(texture.width)) % texture.width;

		const double nearZ = std::min(projectionData.leftZ, projectionData.rightZ);
		const double farZ = std::max(projectionData.leftZ, projectionData.rightZ);
		const double zDistance = nearZ + ((farZ - nearZ) * xRangePercent);

		// Linearly interpolated fog.
		const double fogPercent = std::min(zDistance / this->viewDistance, 1.0);

		uint32_t *pixels = this->colorBuffer.data();
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
				const Double3 color(texel.x, texel.y, texel.z);

				pixels[index] = color.lerp(this->fogColor, fogPercent).clamped().toRGB();
				depth[index] = zDistance;
			}
		}
	}
}

void SoftwareRenderer::render(const VoxelGrid &voxelGrid)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	// Constant camera values. UnitY is the "global up" vector.
	// Assume "this->forward" is normalized.
	const Double3 right = this->forward.cross(Double3::UnitY).normalized();
	const Double3 up = right.cross(this->forward).normalized();

	// Zoom of the camera, based on vertical field of view.
	const double zoom = 1.0 / std::tan((this->fovY * 0.5) * DEG_TO_RAD);

	// Refresh transformation matrix (model matrix isn't required because it's just the
	// identity matrix, and the near plane in the perspective matrix doesn't really matter).
	Matrix4d view = Matrix4d::view(this->eye, this->forward, right, up);
	Matrix4d projection = Matrix4d::perspective(this->fovY, aspect, 0.001, this->viewDistance);
	this->transform = projection * view;

	// Constant camera values for 2D (camera elevation is this->forward.y).
	const Double2 forward2D = Double2(this->forward.x, this->forward.z).normalized();
	const Double2 right2D = Double2(right.x, right.z).normalized();

	// "Forward" component of the camera for generating rays with.
	const Double2 forwardComp = forward2D * zoom;

	// Constant DDA-related values. The Y component also needs to take voxel height
	// into account because voxel height is a level-dependent variable.
	this->startCellReal = Double3(
		std::floor(this->eye.x),
		std::floor(this->eye.y / voxelGrid.getVoxelHeight()),
		std::floor(this->eye.z));
	this->startCell = Int3(
		static_cast<int>(this->startCellReal.x),
		static_cast<int>(this->startCellReal.y),
		static_cast<int>(this->startCellReal.z));

	// Lambda for rendering some columns of pixels using 2.5D ray casting. This is
	// the cheaper form of ray casting (although still not very efficient), and results
	// in a "fake" 3D scene.
	auto renderColumns = [this, &voxelGrid, widthReal, aspect, &forwardComp,
		&right2D](int startX, int endX)
	{
		for (int x = startX; x < endX; ++x)
		{
			// X percent across the screen.
			const double xPercent = static_cast<double>(x) / widthReal;

			// "Right" component of the ray direction, based on current screen X.
			const Double2 rightComp = right2D * (aspect * ((2.0 * xPercent) - 1.0));

			// Calculate the ray direction through the pixel.
			// - If un-normalized, it uses the Z distance, but the insides of voxels
			//   don't look right then.
			const Double2 direction = forwardComp + rightComp;

			// Cast the 2D ray and fill in the column's pixels with color.
			this->castRay(direction, voxelGrid, x);
		}
	};

	// Lambda for clearing some rows on the frame buffer quickly.
	auto clearRows = [this](int startY, int endY)
	{
		const int startIndex = startY * this->width;
		const int endIndex = endY * this->width;

		// Clear some color rows.
		const auto colorBegin = this->colorBuffer.begin() + startIndex;
		const auto colorEnd = this->colorBuffer.begin() + endIndex;
		std::fill(colorBegin, colorEnd, this->fogColor.toRGB());

		// Clear some depth rows.
		const auto depthBegin = this->zBuffer.begin() + startIndex;
		const auto depthEnd = this->zBuffer.begin() + endIndex;
		std::fill(depthBegin, depthEnd, std::numeric_limits<double>::infinity());
	};

	// Start a thread for refreshing the visible flats. This should erase the old list,
	// calculate a new list, and sort it by depth.
	std::thread sortThread([this] { this->updateVisibleFlats(); });

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
