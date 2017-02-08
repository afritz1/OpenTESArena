#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstdint>
#include <vector>

#include "../Math/Vector3.h"
#include "../Math/Vector4.h"

// This class runs the CPU-based 3D rendering for the application.

// I originally used OpenCL with the graphics card for rendering, but this way 
// is much easier to prototype with. OpenCL was an Achilles' Heel anyway, since
// it's not a graphics API, and the overhead of passing frame buffers back and
// forth with the GPU was "hilarious" as one said. It was good practice, though!

class VoxelGrid;

class SoftwareRenderer
{
private:
	struct TextureData
	{
		std::vector<Double4> pixels;
		int width, height;
	};

	std::vector<uint32_t> colorBuffer;
	std::vector<TextureData> textures;
	Double3 eye, forward; // Camera position and forward vector.
	Double3 startCellReal; // Initial voxel as a float type.
	Int3 startCell; // Initial voxel for ray casting.
	double fovY; // Vertical field of view.
	double viewDistance; // Max render distance (usually at 100% fog).
	double viewDistSquared; // For comparing with cell distance squared.
	int width, height; // Dimensions of frame buffer.
	int renderThreadCount; // Number of threads to use for rendering.

	// Casts a ray from the default start point (eye) and returns the color.
	Double3 castRay(const Double3 &direction, const VoxelGrid &voxelGrid) const;
public:
	SoftwareRenderer(int width, int height);
	~SoftwareRenderer();

	// Gets a pointer to the frame buffer's pixels in ARGB8888 format.
	// Intended for writing to a separate hardware texture with.
	const uint32_t *getPixels() const;

	// Methods for setting various camera values.
	void setEye(const Double3 &eye);
	void setForward(const Double3 &forward);
	void setFovY(double fovY);
	void setViewDistance(double viewDistance);

	// Adds a texture and returns its assigned ID (index).
	int addTexture(const uint32_t *pixels, int width, int height);

	// Resizes the frame buffer and related values.
	void resize(int width, int height);

	// Draws the scene to the internal frame buffer.
	void render(const VoxelGrid &voxelGrid);
};

#endif
