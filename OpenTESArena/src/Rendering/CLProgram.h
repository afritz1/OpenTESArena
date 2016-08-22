#ifndef CL_PROGRAM_H
#define CL_PROGRAM_H

#include <vector>

#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl2.hpp>

#include "../Math/Float3.h"

// The CLProgram manages all interactions of the application with the 3D graphics
// engine and the GPU compute schedule. 

// This object should be kept alive while the game data object is alive. Otherwise, 
// it would reload the kernel whenever the game world panel was re-entered, which 
// is unnecessary.

// It is important to remember that cl_float3 and cl_float4 are structurally
// equivalent.

// When resizing buffers, the old data either needs to be copied to a temp buffer, 
// or reloaded completely using the managed IDs to access data, etc..

// To effectively manage sprites, the CLProgram must know:
// - Sprite positions and directions.
// - List of voxel coordinates per sprite rectangle.
//   -> Order of rectangles per each sprite reference chunk does not matter.
// - Changes in voxel coordinates per rectangle, per frame.
//   -> If the compliment of the two lists' intersection is empty, then the sprite
//      did not move or turn between two frames.
// - Sizes of each sprite reference chunk (essentially, offset and count).

// After theorizing some (see SpriteReference.h comments), it appears that having
// single-indirection sprite references (i.e., offset + count -> rectangle) would be
// a better design than double-indirection (i.e., offset + count -> index -> rectangle)
// for rendering. Though single-indirection means having some duplicated geometry for
// each chunk pointed to by a sprite reference, it preserves the cache and prevents 
// global memory from being saturated with incoherent read requests.

// To keep from resizing buffers (like the rectangle array) too often, they should only 
// increase in size as needed. That is, no chunks in the buffer pointed to by a sprite 
// reference are ever shrunk to fit. Each chunk of space in the array should probably get 
// an arbitrary initial allowance of, say, room for two sprites. This chunk is expanded 
// if more sprites occupy the same voxel at once, thus shifting over all the other 
// rectangles in the rectangle array. The worst case would be when many sprites move 
// quickly together through several voxels, especially in coordinates closer to (0, 0, 0),
// though each resize would only happen once per voxel if all entities were in it.

// The more I think about sprite management, the more it feels like a heap manager. I'll
// probably need to draw this on paper to see how it really works out.

class Renderer;
class TextureManager;

struct SDL_Texture;

class CLProgram
{
private:
	static const std::string PATH;
	static const std::string FILENAME;
	static const std::string INTERSECT_KERNEL;
	static const std::string RAY_TRACE_KERNEL;
	static const std::string POST_PROCESS_KERNEL;
	static const std::string CONVERT_TO_RGB_KERNEL;

	cl::Device device; // The device selected from the devices list.
	cl::Context context;
	cl::CommandQueue commandQueue;
	cl::Program program;
	cl::Kernel intersectKernel, rayTraceKernel, convertToRGBKernel;
	cl::Buffer cameraBuffer, voxelRefBuffer, spriteRefBuffer, lightRefBuffer,
		rectangleBuffer, lightBuffer, textureBuffer, gameTimeBuffer, depthBuffer,
		normalBuffer, viewBuffer, pointBuffer, uvBuffer, rectangleIndexBuffer, 
		colorBuffer, outputBuffer;
	std::vector<char> outputData; // For receiving pixels from the device's output buffer.
	SDL_Texture *texture; // Streaming render texture for outputData to update.
	TextureManager &textureManager;
	int renderWidth, renderHeight, worldWidth, worldHeight, worldDepth;

	std::string getBuildReport() const;
	std::string getErrorString(cl_int error) const;

	// For testing purposes before using actual world data.
	void makeTestWorld();
public:
	// Constructor for the OpenCL render program.
	CLProgram(int worldWidth, int worldHeight, int worldDepth,
		TextureManager &textureManager, Renderer &renderer, double renderQuality);
	~CLProgram();

	CLProgram &operator=(CLProgram &&clProgram);

	// These are public in case the options menu is going to need to list them.
	// There should be a constructor that also takes a platform and device, then.
	static std::vector<cl::Platform> getPlatforms();
	static std::vector<cl::Device> getDevices(const cl::Platform &platform,
		cl_device_type type);

	void updateCamera(const Float3d &eye, const Float3d &direction, double fovY);

	// Give this method total ticks instead of delta time so the constructor doesn't
	// need a "start time". Also, this prevents any additive "double -> float" error.
	void updateGameTime(double gameTime);

	void render(Renderer &renderer);
};

#endif
