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
		bool containsTransparency; // For occlusion culling.
	};

	// Helper struct for keeping shading data organized in the renderer. These values are
	// computed once per frame.
	struct ShadingInfo
	{
		// Fog colors for the horizon and zenith to interpolate sky color between.
		Double3 horizonFogColor, zenithFogColor;

		// Light and direction of the sun.
		Double3 sunColor, sunDirection;

		// Global ambient light percent.
		double ambient;

		ShadingInfo(const Double3 &horizonFogColor, const Double3 &zenithFogColor,
			const Double3 &sunColor, const Double3 &sunDirection, double ambient);
	};

	// A flat is a 2D surface always facing perpendicular to the Y axis (not necessarily
	// facing the camera). It might be a door, sprite, store sign, etc..
	struct Flat
	{
		Double3 position; // Center of bottom edge.
		Double2 direction; // In XZ plane.
		double width, height;
		int textureID;
		bool flipped;

		// A flat's projection consists of two vertical line segments that are interpolated
		// between by the renderer.
		struct Projection
		{
			// An edge represents a projected column on the screen in XY screen coordinates 
			// (that is, (0, 0) is at the center). Z is true distance from the camera in the
			// XZ plane. U is for horizontal texture coordinates (in case of line clipping).
			struct Edge
			{
				double x, topY, bottomY, z, u;
			};

			Edge left, right;
		};
	};

	// Clipping planes for Z coordinates.
	static const double NEAR_PLANE;
	static const double FAR_PLANE;

	std::vector<double> zBuffer;
	std::unordered_map<int, Flat> flats;
	std::vector<std::pair<const Flat*, Flat::Projection>> visibleFlats;
	std::vector<TextureData> textures;
	std::vector<Double3> skyPalette; // Colors for each time of day.
	double fogDistance; // Distance at which fog is maximum.
	int width, height; // Dimensions of frame buffer.
	int renderThreadCount; // Number of threads to use for rendering.

	// Gets the fog color (based on the time of day). It returns a value instead of
	// a reference because it interpolates between two colors for a smoother transition.
	Double3 getFogColor(double daytimePercent) const;

	// Gets the current sun direction based on the time of day.
	Double3 getSunDirection(double daytimePercent) const;

	// Casts a 3D ray from the default start point (eye) and returns the color.
	// (Unused for now; keeping for reference).
	//Double3 castRay(const Double3 &direction, const VoxelGrid &voxelGrid) const;

	// Casts a 2D ray that steps through the current floor, rendering all voxels
	// in the XZ column of each voxel.
	void castColumnRay(int x, const Double3 &eye, const Double2 &direction,
		const Matrix4d &transform, double yShear, double daytimePercent,
		const Double3 &fogColor, const Double3 &sunDirection, const VoxelGrid &voxelGrid, 
		uint32_t *colorBuffer);

	// Refreshes the list of flats that are within the viewing frustum. "yShear" is the 
	// Y-shearing component of the projection plane, and "transform" is the projection * 
	// view matrix.
	void updateVisibleFlats(const Double3 &eye, double yShear, const Matrix4d &transform);
public:
	SoftwareRenderer(int width, int height);
	~SoftwareRenderer();

	// Adds a flat. Causes an error if the ID exists.
	void addFlat(int id, const Double3 &position, const Double2 &direction, double width,
		double height, int textureID);

	// Adds a light. Causes an error if the ID exists.
	void addLight(int id, const Double3 &point, const Double3 &color, double intensity);

	// Adds a texture and returns its assigned ID (index).
	int addTexture(const uint32_t *pixels, int width, int height);

	// Updates various data for a flat. If a value doesn't need updating, pass null.
	// Causes an error if no ID matches.
	void updateFlat(int id, const Double3 *position, const Double2 *direction,
		const double *width, const double *height, const int *textureID,
		const bool *flipped);

	// Updates various data for a light. If a value doesn't need updating, pass null.
	// Causes an error if no ID matches.
	void updateLight(int id, const Double3 *point, const Double3 *color,
		const double *intensity);

	// Sets the distance at which the fog is maximum.
	void setFogDistance(double fogDistance);

	// Sets the sky palette to use with sky colors based on the time of day.
	// For dungeons, this would probably just be one black pixel.
	void setSkyPalette(const uint32_t *colors, int count);

	// Removes a flat. Causes an error if no ID matches.
	void removeFlat(int id);

	// Removes a light. Causes an error if no ID matches.
	void removeLight(int id);

	// Resizes the frame buffer and related values.
	void resize(int width, int height);

	// Draws the scene to the output color buffer in ARGB8888 format.
	void render(const Double3 &eye, const Double3 &forward, double fovY, 
		double daytimePercent, const VoxelGrid &voxelGrid, uint32_t *colorBuffer);
};

#endif
