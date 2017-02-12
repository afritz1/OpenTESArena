#include <algorithm>
#include <cassert>
#include <cmath>
#include <thread>

#include "SoftwareRenderer.h"

#include "../Math/Constants.h"
#include "../Utilities/Debug.h"
#include "../World/VoxelData.h"
#include "../World/VoxelGrid.h"

SoftwareRenderer::SoftwareRenderer(int width, int height)
{
	// Initialize 2D frame buffer.
	const int pixelCount = width * height;
	this->colorBuffer = std::vector<uint32_t>(pixelCount);
	std::fill(this->colorBuffer.begin(), this->colorBuffer.end(), 0);

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
	this->eye = Double3();
	this->forward = Double3();
	this->fovY = 0.0;

	this->viewDistance = 0.0;
	this->viewDistSquared = 0.0;

	// Initialize start cell to "empty".
	this->startCellReal = Double3();
	this->startCell = Int3();
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
	this->forward = forward;
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

void SoftwareRenderer::resize(int width, int height)
{
	const int pixelCount = width * height;
	this->colorBuffer.resize(pixelCount);
	std::fill(this->colorBuffer.begin(), this->colorBuffer.end(), 0);

	this->width = width;
	this->height = height;
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

	// Simple fog color.
	const Double3 fog(0.45, 0.75, 1.0);

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
		return color.lerp(fog, depth);
	}
	else
	{
		// No intersection. Return sky color.
		return fog;
	}
}

void SoftwareRenderer::castRay(const Double2 &direction,
	const VoxelGrid &voxelGrid, int x, uint32_t *pixels) const
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

	// Pointer to voxel grid data.
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
			// - Maybe avoid a square root with direction?
			if (initialSideDistX < initialSideDistZ)
			{
				//zDistance = initialSideDistX;
				zDistance = 0.01;
				axis = Axis::X;
			}
			else
			{
				//zDistance = initialSideDistZ;
				zDistance = 0.01;
				axis = Axis::Z;
			}
		}
		else
		{
			zDistance = (axis == Axis::X) ?
				(static_cast<double>(cellX) - this->eye.x + static_cast<double>((1 - stepX) / 2)) / dirX :
				(static_cast<double>(cellZ) - this->eye.z + static_cast<double>((1 - stepZ) / 2)) / dirZ;
		}

		// Height in pixels on the screen.
		// "Pixel height" looks like a very simple form of projection, and will need to be 
		// expanded for more flexibility.
		const double heightReal = static_cast<double>(this->height);
		const int pixelHeight = static_cast<int>(std::round(heightReal / zDistance));

		// Find the Y coordinates of where the wall starts and stops on the screen.
		// - Dividing pixelHeight by 4 instead of 2 compensates for aspect vs. aspect * 0.5.
		const int drawStart = std::max(0, (-pixelHeight / 4) + (this->height / 2));
		const int drawEnd = std::min(this->height, (pixelHeight / 4) + (this->height / 2));

		// Calculate linearly interpolated fog.
		const Double3 fogColor(0.40, 0.65, 1.0);
		const double fogPercent = std::min(zDistance, this->viewDistance) / this->viewDistance;
		
		for (int y = drawStart; y < drawEnd; ++y)
		{
			const Double4 &texel = this->textures[hitID - 1].pixels[0];
			const Double3 color(texel.x, texel.y, texel.z);

			const int index = x + (y * this->width);
			pixels[index] = color.lerp(fogColor, fogPercent).toRGB();
		}
	}
}

void SoftwareRenderer::render(const VoxelGrid &voxelGrid)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	// Constant camera values. "(0.0, 1.0, 0.0)" is the "global up" vector.
	const Double3 forward = this->forward.normalized();
	const Double3 right = forward.cross(Double3(0.0, 1.0, 0.0)).normalized();
	const Double3 up = right.cross(forward).normalized();

	// Constant camera values for 2D (camera elevation would be forward.y).
	const Double2 forward2D = Double2(forward.x, forward.z).normalized();
	const Double2 right2D = Double2(right.x, right.z).normalized();

	// Zoom of the camera, based on vertical field of view.
	const double zoom = 1.0 / std::tan((this->fovY * 0.5) * DEG_TO_RAD);

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

	// Output color buffer.
	uint32_t *pixels = this->colorBuffer.data();

	// Lambda for rendering some rows of pixels using 3D ray casting. While this is 
	// far more expensive than 2.5D ray casting, it does allow the scene to be 
	// represented in true 3D instead of "fake" 3D.
	/*auto renderRows = [this, &voxelGrid, widthReal, heightReal, aspect,
		&forwardComp, &up, &right, pixels](int startY, int endY)
	{
		for (int y = startY; y < endY; ++y)
		{
			// Y percent across the screen.
			const double yPercent = static_cast<double>(y) / heightReal;

			// "Up" component of the ray direction, based on current screen Y.
			const Double3 upComp = up * ((2.0 * yPercent) - 1.0);

			for (int x = 0; x < this->width; ++x)
			{
				// X percent across the screen.
				const double xPercent = static_cast<double>(x) / widthReal;

				// "Right" component of the ray direction, based on current screen X.
				const Double3 rightComp = right * (aspect * ((2.0 * xPercent) - 1.0));

				// Calculate the ray direction through the pixel.
				// - If un-normalized, it uses the Z distance, but the insides of voxels
				//   don't look right then.
				const Double3 direction = (forwardComp + rightComp - upComp).normalized();

				// Get the resulting color of the ray, starting from the eye.
				const Double3 color = this->castRay(direction, voxelGrid);

				// Convert to 0x00RRGGBB.
				const int index = x + (y * this->width);
				pixels[index] = color.clamped().toRGB();
			}
		}
	};*/

	// Lambda for rendering some columns of pixels using 2.5D ray casting. This is
	// the cheaper form of ray casting (although still not very efficient), and results
	// in a "fake" 3D scene.
	auto renderColumns = [this, &voxelGrid, widthReal, aspect, &forwardComp,
		&right2D, pixels](int startX, int endX)
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
			this->castRay(direction, voxelGrid, x, pixels);
		}
	};

	// Clear screen (this could potentially be multi-threaded).
	const Double3 skyColor(0.40, 0.65, 1.0);
	std::fill(this->colorBuffer.begin(), this->colorBuffer.end(), skyColor.toRGB());

	// Prepare render threads.
	std::vector<std::thread> renderThreads(this->renderThreadCount);

	// Start the render threads. "blockSize" is the approximate number of columns per thread.
	// Rounding is involved so the start and stop coordinates are correct for all resolutions.
	const double blockSize = widthReal / static_cast<double>(this->renderThreadCount);
	for (int i = 0; i < this->renderThreadCount; ++i)
	{
		const int startX = static_cast<int>(std::round(static_cast<double>(i) * blockSize));
		const int endX = static_cast<int>(std::round(static_cast<double>(i + 1) * blockSize));

		// Make sure the rounding is correct.
		assert(startX >= 0);
		assert(endX <= this->width);

		renderThreads[i] = std::thread(renderColumns, startX, endX);
	}

	// Wait for the render threads to finish.
	for (auto &thread : renderThreads)
	{
		thread.join();
	}
}
