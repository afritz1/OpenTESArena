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
	this->eye = Double3();
	this->forward = Double3();
	this->fovY = 0.0;

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

Double3 SoftwareRenderer::castRay(const Double3 &direction,
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

	const Double3 dirSquared(
		direction.x * direction.x,
		direction.y * direction.y,
		direction.z * direction.z);

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
		sideDist.y = (this->startCellReal.y + 1.0 - this->eye.y) * deltaDist.y;
	}
	else
	{
		step.y = -1;
		sideDist.y = (this->eye.y - this->startCellReal.y) * deltaDist.y;
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

	// Get initial voxel coordinates.
	Int3 cell = this->startCell;

	// ID of a hit voxel. Zero (air) by default.
	char hitID = 0;

	// Axis of a hit voxel's side. X by default.
	enum class Axis { X, Y, Z };
	Axis axis = Axis::X;

	// Step through the voxel grid while the current coordinate is valid.
	// - Another condition should eventually be used instead. It should
	//   check whether the ray has stepped far enough (cmp distance squared?),
	//   and "voxelIsValid" should be used to 'continue' instead if not valid.
	bool voxelIsValid = (cell.x >= 0) && (cell.y >= 0) && (cell.z >= 0) &&
		(cell.x < gridWidth) && (cell.y < gridHeight) && (cell.z < gridDepth);

	while (voxelIsValid)
	{
		const int gridIndex = cell.x + (cell.y * gridWidth) +
			(cell.z * gridWidth * gridHeight);

		// Check if the current voxel is solid.
		const char voxelID = voxelGrid[gridIndex];

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
			voxelIsValid = (cell.x >= 0) && (cell.x < gridWidth);
		}
		else if (sideDist.y < sideDist.z)
		{
			sideDist.y += deltaDist.y;
			cell.y += step.y;
			axis = Axis::Y;
			voxelIsValid = (cell.y >= 0) && (cell.y < gridHeight);
		}
		else
		{
			sideDist.z += deltaDist.z;
			cell.z += step.z;
			axis = Axis::Z;
			voxelIsValid = (cell.z >= 0) && (cell.z < gridDepth);
		}
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

		// Assign to distance based on which axis was hit (x, y, z).
		distance = (static_cast<double>(cell[axisIndex]) - this->eye[axisIndex] +
			static_cast<double>((1 - step[axisIndex]) / 2)) / direction[axisIndex];
	}

	// Simple fog color.
	const Double3 fog(0.45, 0.75, 1.0);

	// If there was a hit, get the shaded color.
	if (hitID > 0)
	{
		// Intersection point on the voxel.
		const Double3 hitPoint = this->eye + (direction * distance);

		// Texture coordinates.
		// - To do: use "stoppedInFirstVoxel" condition here to make sure coordinates
		//   inside voxels are flipped correctly.
		double u, v;
		if (axis == Axis::X)
		{
			if (nonNegativeDirX)
			{
				u = hitPoint.z - std::floor(hitPoint.z);
				v = 1.0 - (hitPoint.y - std::floor(hitPoint.y));
			}
			else
			{
				u = 1.0 - (hitPoint.z - std::floor(hitPoint.z));
				v = 1.0 - (hitPoint.y - std::floor(hitPoint.y));
			}
		}
		else if (axis == Axis::Y)
		{
			if (nonNegativeDirY)
			{
				u = hitPoint.z - std::floor(hitPoint.z);
				v = hitPoint.x - std::floor(hitPoint.x);
			}
			else
			{
				u = hitPoint.z - std::floor(hitPoint.z);
				v = 1.0 - (hitPoint.x - std::floor(hitPoint.x));
			}
		}
		else
		{
			if (nonNegativeDirZ)
			{
				u = 1.0 - (hitPoint.x - std::floor(hitPoint.x));
				v = 1.0 - (hitPoint.y - std::floor(hitPoint.y));
			}
			else
			{
				u = hitPoint.x - std::floor(hitPoint.x);
				v = 1.0 - (hitPoint.y - std::floor(hitPoint.y));
			}
		}

		// Simple UVW placeholder color.
		const Double3 color(u, v, 1.0 - u - v);

		// Linearly interpolate with some depth.
		const double maxDist = 15.0;
		const double depth = std::min(distance, maxDist) / maxDist;
		return color.lerp(fog, depth);
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
	const Double3 forward = this->forward.normalized();
	const Double3 right = forward.cross(Double3(0.0, 1.0, 0.0)).normalized();
	const Double3 up = right.cross(forward).normalized();

	// Zoom of the camera, based on vertical field of view.
	const double zoom = 1.0 / std::tan((this->fovY * 0.5) * DEG_TO_RAD);

	// "Forward" component of the camera for generating rays with.
	const Double3 forwardComp = forward * zoom;

	// Constant DDA-related values.
	this->startCellReal = Double3(
		std::floor(this->eye.x),
		std::floor(this->eye.y),
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
	auto renderRows = [this, &voxelGrid, gridWidth, gridHeight, gridDepth, widthReal,
		heightReal, aspect, &forwardComp, &up, &right, pixels](int startY, int endY)
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
				const Double3 color = this->castRay(direction, voxelGrid,
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
