#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstdint>
#include <vector>

#include "../Math/Float3.h"
#include "../Math/Int3.h"

// This class runs the CPU-based 3D rendering for the application.

// I originally used OpenCL with the graphics card for rendering, but this way 
// is much easier to prototype with. OpenCL was an Achilles' Heel anyway, since
// it's not a graphics API, and the overhead of passing frame buffers back and
// forth with the GPU was "hilarious" as one said. It was good practice, though!

class SoftwareRenderer
{
private:
	std::vector<uint32_t> colorBuffer;
	Float3d eye, forward; // Camera position and forward vector.
	Float3d startCellReal; // Initial voxel as a float type.
	Int3 startCell; // Initial voxel for ray casting.
	double fovY; // Vertical field of view.
	int width, height; // Dimensions of frame buffer.
	int renderThreadCount; // Number of threads to use for rendering.

	// Casts a ray from the default start point (eye) and returns the color.
	Float3d castRay(const Float3d &direction, const std::vector<char> &voxelGrid, 
		const int gridWidth, const int gridHeight, const int gridDepth) const;
public:
	SoftwareRenderer(int width, int height);
	~SoftwareRenderer();

	// Gets a pointer to the frame buffer's pixels in ARGB8888 format.
	// Intended for writing to a separate hardware texture with.
	const uint32_t *getPixels() const;

	// Methods for setting various camera values.
	void setEye(const Float3d &eye);
	void setForward(const Float3d &forward);
	void setFovY(double fovY);

	// Draws the scene to the internal frame buffer.
	void render(const std::vector<char> &voxelGrid, const int gridWidth,
		const int gridHeight, const int gridDepth);
};

#endif
