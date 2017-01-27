#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstdint>
#include <vector>

// This class runs the CPU-based 3D rendering for the application.

// I originally used OpenCL with the graphics card for rendering, but this way 
// is much easier to prototype with. OpenCL was an Achilles' Heel anyway, since
// it's not a graphics API, and the overhead of passing frame buffers back and
// forth with the GPU was "hilarious" as one said. It was good practice, though!

class SoftwareRenderer
{
private:
	std::vector<uint32_t> pixels;
	int width, height;
public:
	SoftwareRenderer(int width, int height);
	~SoftwareRenderer();

	// Gets the width of the frame buffer.
	int getWidth() const;

	// Gets the height of the frame buffer.
	int getHeight() const;

	// Gets a pointer to the frame buffer's pixels in ARGB8888 format.
	// Intended for writing to a separate hardware texture with.
	const uint32_t *getPixels() const;

	// Draws the scene to the internal frame buffer.
	void render();
};

#endif
