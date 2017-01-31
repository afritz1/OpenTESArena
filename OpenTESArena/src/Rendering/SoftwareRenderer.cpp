#include <algorithm>
#include <cassert>
#include <cmath>
#include <thread>

#include "SoftwareRenderer.h"

#include "../Math/Constants.h"
#include "../Utilities/Debug.h"

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
	this->eye = Float3d(0.0, 0.0, 0.0);
	this->forward = Float3d(0.0, 0.0, 0.0);
	this->fovY = 0.0;

	// Initialize start cell to "empty".
	this->startCellReal = Float3d(0.0, 0.0, 0.0);
	this->startCell = Int3(0, 0, 0);
}

SoftwareRenderer::~SoftwareRenderer()
{

}

const uint32_t *SoftwareRenderer::getPixels() const
{
	return this->colorBuffer.data();
}

void SoftwareRenderer::setEye(const Float3d &eye)
{
	this->eye = eye;
}

void SoftwareRenderer::setForward(const Float3d &forward)
{
	this->forward = forward;
}

void SoftwareRenderer::setFovY(double fovY)
{
	this->fovY = fovY;
}

Float3d SoftwareRenderer::castRay(const Float3d &direction,
	const std::vector<char> &voxelGrid, const int gridWidth,
	const int gridHeight, const int gridDepth) const
{
	// This is an extension of Lode Vandevenne's DDA algorithm from 2D to 3D.
	// Technically, it could be considered a "3D-DDA" algorithm. It will eventually 
	// have some additional features so all of Arena's geometry can be represented.

	// Once variable-sized voxels start getting used, there will need to be several
	// things changed here. First, some constants like "1.0" will become parameters.
	// Second, the distance calculation will need to be modified accordingly. Third, 
	// the texture coordinates will need some modifications so they span the range 
	// of the voxel face (maybe subtracting the floor() won't be sufficient?).

	// Some floating point behavior assumptions:
	// -> (value / 0.0) == infinity
	// -> (value / infinity) == 0.0
	// -> (int)(-0.8) == 0
	// -> (int)floor(-0.8) == -1
	// -> (int)ceil(-0.8) == 0

	const double dirXSquared = direction.getX() * direction.getX();
	const double dirYSquared = direction.getY() * direction.getY();
	const double dirZSquared = direction.getZ() * direction.getZ();

	// Calculate delta distances along each axis. These determine how far
	// the ray has to go until the next X, Y, or Z side is hit, respectively.
	const double deltaDistX = std::sqrt(1.0 + (dirYSquared / dirXSquared) +
		(dirZSquared / dirXSquared));
	const double deltaDistY = std::sqrt(1.0 + (dirXSquared / dirYSquared) +
		(dirZSquared / dirYSquared));
	const double deltaDistZ = std::sqrt(1.0 + (dirXSquared / dirZSquared) +
		(dirYSquared / dirZSquared));

	// Booleans for whether a ray component is non-negative. Used with step directions 
	// and texture coordinates.
	const bool nonNegativeDirX = direction.getX() >= 0.0;
	const bool nonNegativeDirY = direction.getY() >= 0.0;
	const bool nonNegativeDirZ = direction.getZ() >= 0.0;

	// Calculate step directions and initial side distances.
	int stepX;
	double sideDistX;
	if (nonNegativeDirX)
	{
		stepX = 1;
		sideDistX = (this->startCellReal.getX() + 1.0 - this->eye.getX()) * deltaDistX;
	}
	else
	{
		stepX = -1;
		sideDistX = (this->eye.getX() - this->startCellReal.getX()) * deltaDistX;
	}

	int stepY;
	double sideDistY;
	if (nonNegativeDirY)
	{
		stepY = 1;
		sideDistY = (this->startCellReal.getY() + 1.0 - this->eye.getY()) * deltaDistY;
	}
	else
	{
		stepY = -1;
		sideDistY = (this->eye.getY() - this->startCellReal.getY()) * deltaDistY;
	}

	int stepZ;
	double sideDistZ;
	if (nonNegativeDirZ)
	{
		stepZ = 1;
		sideDistZ = (this->startCellReal.getZ() + 1.0 - this->eye.getZ()) * deltaDistZ;
	}
	else
	{
		stepZ = -1;
		sideDistZ = (this->eye.getZ() - this->startCellReal.getZ()) * deltaDistZ;
	}

	// Make a copy of the initial side distances. They are used for the special case 
	// of the ray ending in the same voxel it started in.
	const double initialSideDistX = sideDistX;
	const double initialSideDistY = sideDistY;
	const double initialSideDistZ = sideDistZ;

	// Get initial voxel coordinates.
	int cellX = this->startCell.getX();
	int cellY = this->startCell.getY();
	int cellZ = this->startCell.getZ();

	// ID of a hit voxel. Zero (air) by default.
	char hitID = 0;

	// Axis of a hit voxel's side. X by default.
	enum class Axis { X, Y, Z };
	Axis axis = Axis::X;

	// Step through the voxel grid while the current coordinate is valid.
	// - Another condition should eventually be used instead. It should
	//   check whether the ray has stepped far enough.
	bool voxelIsValid = (cellX >= 0) && (cellY >= 0) && (cellZ >= 0) &&
		(cellX < gridWidth) && (cellY < gridHeight) && (cellZ < gridDepth);

	while (voxelIsValid)
	{
		const int gridIndex = cellX + (cellY * gridWidth) +
			(cellZ * gridWidth * gridHeight);

		// Check if the current voxel is solid.
		const char voxelID = voxelGrid[gridIndex];

		if (voxelID > 0)
		{
			hitID = voxelID;
			break;
		}

		if ((sideDistX < sideDistY) && (sideDistX < sideDistZ))
		{
			sideDistX += deltaDistX;
			cellX += stepX;
			axis = Axis::X;
			voxelIsValid = (cellX >= 0) && (cellX < gridWidth);
		}
		else if (sideDistY < sideDistZ)
		{
			sideDistY += deltaDistY;
			cellY += stepY;
			axis = Axis::Y;
			voxelIsValid = (cellY >= 0) && (cellY < gridHeight);
		}
		else
		{
			sideDistZ += deltaDistZ;
			cellZ += stepZ;
			axis = Axis::Z;
			voxelIsValid = (cellZ >= 0) && (cellZ < gridDepth);
		}
	}

	// Boolean for whether the ray ended in the same voxel it started in.
	const bool stoppedInFirstVoxel =
		(cellX == this->startCell.getX()) &&
		(cellY == this->startCell.getY()) &&
		(cellZ == this->startCell.getZ());

	// Get the distance from the camera to the hit point. It is a special case
	// if the ray stopped in the first voxel.
	double distance;
	if (stoppedInFirstVoxel)
	{
		if ((initialSideDistX < initialSideDistY) &&
			(initialSideDistX < initialSideDistZ))
		{
			distance = initialSideDistX;
			axis = Axis::X;
		}
		else if (initialSideDistY < initialSideDistZ)
		{
			distance = initialSideDistY;
			axis = Axis::Y;
		}
		else
		{
			distance = initialSideDistZ;
			axis = Axis::Z;
		}
	}
	else
	{
		if (axis == Axis::X)
		{
			distance = (static_cast<double>(cellX) - this->eye.getX() +
				static_cast<double>((1 - stepX) / 2)) / direction.getX();
		}
		else if (axis == Axis::Y)
		{
			distance = (static_cast<double>(cellY) - this->eye.getY() +
				static_cast<double>((1 - stepY) / 2)) / direction.getY();
		}
		else
		{
			distance = (static_cast<double>(cellZ) - this->eye.getZ() +
				static_cast<double>((1 - stepZ) / 2)) / direction.getZ();
		}
	}

	// Simple fog color.
	const Float3d fog(0.45, 0.75, 1.0);

	// If there was a hit, get the shaded color.
	if (hitID > 0)
	{
		// Intersection point on the voxel.
		Float3d hitPoint = this->eye + (direction * distance);

		// Texture coordinates.
		// - To do: use "stoppedInFirstVoxel" condition here to make sure coordinates
		//   inside voxels are flipped correctly.
		double u, v;
		if (axis == Axis::X)
		{
			if (nonNegativeDirX)
			{
				u = hitPoint.getZ() - std::floor(hitPoint.getZ());
				v = 1.0 - (hitPoint.getY() - std::floor(hitPoint.getY()));
			}
			else
			{
				u = 1.0 - (hitPoint.getZ() - std::floor(hitPoint.getZ()));
				v = 1.0 - (hitPoint.getY() - std::floor(hitPoint.getY()));
			}
		}
		else if (axis == Axis::Y)
		{
			if (nonNegativeDirY)
			{
				u = hitPoint.getZ() - std::floor(hitPoint.getZ());
				v = hitPoint.getX() - std::floor(hitPoint.getX());
			}
			else
			{
				u = hitPoint.getZ() - std::floor(hitPoint.getZ());
				v = 1.0 - (hitPoint.getX() - std::floor(hitPoint.getX()));
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				u = 1.0 - (hitPoint.getX() - std::floor(hitPoint.getX()));
				v = 1.0 - (hitPoint.getY() - std::floor(hitPoint.getY()));
			}
			else
			{
				u = hitPoint.getX() - std::floor(hitPoint.getX());
				v = 1.0 - (hitPoint.getY() - std::floor(hitPoint.getY()));
			}
		}

		// Simple UVW placeholder color.
		const Float3d color(u, v, 1.0 - u - v);

		// Linearly interpolate with some depth.
		const double maxDist = 15.0;
		const double depth = std::min(distance, maxDist) / maxDist;
		return color + ((fog - color) * depth);
	}
	else
	{
		// No intersection. Return sky color.
		return fog;
	}
}

void SoftwareRenderer::render(const std::vector<char> &voxelGrid,
	const int gridWidth, const int gridHeight, const int gridDepth)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	// Constant camera values. "(0.0, 1.0, 0.0)" is the "global up" vector.
	const Float3d forward = this->forward.normalized();
	const Float3d right = forward.cross(Float3d(0.0, 1.0, 0.0)).normalized();
	const Float3d up = right.cross(forward).normalized();

	// Zoom of the camera, based on vertical field of view.
	const double zoom = 1.0 / std::tan((this->fovY * 0.5) * DEG_TO_RAD);

	// "Forward" component of the camera for generating rays with.
	const Float3d forwardComp = forward * zoom;

	// Constant DDA-related values.
	this->startCellReal = Float3d(
		std::floor(this->eye.getX()),
		std::floor(this->eye.getY()),
		std::floor(this->eye.getZ()));
	this->startCell = Int3(
		static_cast<int>(this->startCellReal.getX()),
		static_cast<int>(this->startCellReal.getY()),
		static_cast<int>(this->startCellReal.getZ()));

	// Output color buffer.
	uint32_t *pixels = this->colorBuffer.data();

	// Lambda for rendering some rows of pixels using 3D ray casting. While this is 
	// far more expensive than 2.5D ray casting, it does allow the scene to be 
	// represented in true 3D instead of "fake" 3D.
	auto renderRows = [this, &voxelGrid, gridWidth, gridHeight, gridDepth, widthReal,
		heightReal, aspect, &forwardComp, &up, &right, pixels](int startY, int endY)
	{
		for (int y = startY; y < endY; ++y)
		{
			// Y percent across the screen.
			const double yPercent = static_cast<double>(y) / heightReal;

			// "Up" component of the ray direction, based on current screen Y.
			const Float3d upComp = up * ((2.0 * yPercent) - 1.0);

			for (int x = 0; x < this->width; ++x)
			{
				// X percent across the screen.
				const double xPercent = static_cast<double>(x) / widthReal;

				// "Right" component of the ray direction, based on current screen X.
				const Float3d rightComp = right * (aspect * ((2.0 * xPercent) - 1.0));

				// Calculate the ray direction through the pixel.
				// - If un-normalized, it uses the Z distance, but the insides of voxels
				//   don't look right then.
				const Float3d direction = (forwardComp + rightComp - upComp).normalized();

				// Get the resulting color of the ray, starting from the eye.
				const Float3d color = this->castRay(direction, voxelGrid,
					gridWidth, gridHeight, gridDepth);

				// Convert to 0x00RRGGBB.
				const int index = x + (y * this->width);
				pixels[index] = color.clamped().toRGB();
			}
		}
	};

	// Prepare render threads.
	std::vector<std::thread> renderThreads(this->renderThreadCount);

	// Start the render threads. "blockSize" is the approximate number of rows per thread.
	// Rounding is involved so the start and stop rows are correct for all resolutions.
	const double blockSize = heightReal / static_cast<double>(this->renderThreadCount);
	for (int i = 0; i < this->renderThreadCount; ++i)
	{
		const int startY = static_cast<int>(std::round(static_cast<double>(i) * blockSize));
		const int endY = static_cast<int>(std::round(static_cast<double>(i + 1) * blockSize));

		// Make sure the rounding is correct.
		assert(startY >= 0);
		assert(endY <= this->height);

		renderThreads[i] = std::thread(renderRows, startY, endY);
	}

	// Wait for the render threads to finish.
	for (auto &thread : renderThreads)
	{
		thread.join();
	}
}
