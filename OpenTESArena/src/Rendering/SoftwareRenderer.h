#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
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
	enum class Axis { X, Y, Z };

	struct TextureData
	{
		std::vector<Double4> pixels;
		int width, height;
	};

	// A flat is a 2D surface always facing perpendicular to the Y axis. It might be 
	// a door, sprite, store sign, etc..
	struct Flat
	{
		Double3 position; // Center of bottom edge.
		Double2 direction; // In XZ plane.
		double width, height;
		int textureID;

		struct ProjectionData
		{
			// Four corners of the flat projected onto the viewing plane. These aren't 
			// stored as 2-component vectors because there are some duplicates.
			double leftX, rightX;
			double topLeftY, topRightY, bottomLeftY, bottomRightY;

			// Z-distances for left edge and right edge, for distance comparisons.
			double leftZ, rightZ;
		};
	};

	// Clipping planes for Z coordinates.
	static const double NEAR_PLANE;
	static const double FAR_PLANE;

	std::vector<uint32_t> colorBuffer;
	std::vector<double> zBuffer;
	std::unordered_map<int, Flat> flats;
	std::vector<std::pair<const Flat*, Flat::ProjectionData>> visibleFlats;
	std::vector<TextureData> textures;
	std::vector<Double3> skyPalette; // Colors for each time of day.
	double fogDistance; // Distance at which fog is maximum.
	int width, height; // Dimensions of frame buffer.
	int renderThreadCount; // Number of threads to use for rendering.

	// Gets the fog color (usually based on the time of day).
	const Double3 &getFogColor() const;

	// Casts a 3D ray from the default start point (eye) and returns the color.
	// (Unused for now; keeping for reference).
	//Double3 castRay(const Double3 &direction, const VoxelGrid &voxelGrid) const;

	// Casts a 2D ray that steps through the current floor, rendering all voxels
	// in the XZ column of each voxel.
	void castColumnRay(int x, const Double3 &eye, const Double2 &direction,
		const Matrix4d &transform, double cameraElevation,
		const VoxelGrid &voxelGrid);

	// Refreshes the list of flats that are within the viewing frustum.
	// "cameraElevation" is the Y-shearing component of the projection plane, and 
	// "transform" is the projection + view matrix.
	void updateVisibleFlats(double cameraElevation, const Matrix4d &transform);
public:
	SoftwareRenderer(int width, int height);
	~SoftwareRenderer();

	// Gets a pointer to the frame buffer's pixels in ARGB8888 format.
	// Intended for writing to a separate hardware texture with.
	const uint32_t *getPixels() const;

	// Sets the distance at which the fog is maximum.
	void setFogDistance(double fogDistance);

	// Sets the sky palette to use with sky colors based on the time of day.
	// For dungeons, this would probably just be one black pixel.
	void setSkyPalette(const uint32_t *colors, int count);

	// Adds a texture and returns its assigned ID (index).
	int addTexture(const uint32_t *pixels, int width, int height);

	// Adds a flat and returns its assigned ID.
	int addFlat(const Double3 &position, const Double2 &direction, double width,
		double height, int textureID);

	// Updates various data for a flat. If a value doesn't need updating, pass null.
	// Causes an error if no ID matches.
	void updateFlat(int id, const Double3 *position, const Double2 *direction,
		const double *width, const double *height, const int *textureID);

	// Removes a flat. Causes an error if no ID matches.
	void removeFlat(int id);

	// Resizes the frame buffer and related values.
	void resize(int width, int height);

	// Draws the scene to the internal frame buffer.
	void render(const Double3 &eye, const Double3 &forward, double fovY, 
		double gameTime, const VoxelGrid &voxelGrid);
};

#endif
