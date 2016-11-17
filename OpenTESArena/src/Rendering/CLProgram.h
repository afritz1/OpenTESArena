#ifndef CL_PROGRAM_H
#define CL_PROGRAM_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl2.hpp>

#include "../Math/Float3.h"
#include "../Math/Int3.h"

// The CLProgram manages all interactions of the application with the 3D graphics
// engine and the GPU compute schedule. It should be refreshed when the window is
// resized or when the game world's dimensions change.

// It is important to remember that cl_float3 and cl_float4 are structurally
// equivalent (cl_float * 4, or 16 bytes).

// After theorizing some (see SpriteReference.h comments), it appears that having
// single-indirection sprite references (i.e., offset + count -> rectangle) would be
// faster than double-indirection (i.e., offset + count -> index -> rectangle) for 
// rendering. Though single-indirection means having some duplicated geometry for each 
// chunk pointed to by a sprite reference, it preserves the cache and prevents global 
// memory from being saturated with incoherent read requests. Double-indirection would 
// also mean passing another index buffer to the kernel.

// The more I think about sprite management, the more it feels like a heap manager. I'll
// probably need to draw this on paper to see how it really works out.

class BufferView;
class Rect3D;
class TextureReference;

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
	std::vector<uint8_t> outputData; // For receiving pixels from the device's output buffer.
	std::unordered_map<Int3, size_t> voxelOffsets; // Byte offsets into rectBuffer for each voxel.
	std::unique_ptr<BufferView> rectBufferView; // For managing allocations in rectBuffer.
	std::vector<TextureReference> textureRefs;
	int renderWidth, renderHeight, worldWidth, worldHeight, worldDepth;

	std::string getBuildReport() const;
	std::string getErrorString(cl_int error) const;

	// Reallocates a device buffer with the given size while retaining the previous data.
	// If the new size is smaller, the previous data is truncated. Any kernel arguments
	// using the buffer must be refreshed by the caller.
	void resizeBuffer(cl::Buffer &buffer, cl::size_type newSize);

	// Returns a vector of voxel coordinates for all voxels that a rectangle touches,
	// with the option to only get voxels within the world bounds. Intended only for 
	// sprites.
	std::vector<Int3> getTouchedVoxels(const Rect3D &rect, bool clampBounds) const;

	// Updates a voxel reference's offset and count in device memory.
	void updateVoxelRef(int x, int y, int z, int offset, int count);
public:
	// Constructor for the OpenCL render program.
	CLProgram(int worldWidth, int worldHeight, int worldDepth,
		int renderWidth, int renderHeight);
	CLProgram(const CLProgram&) = delete;
	~CLProgram();

	CLProgram &operator=(const CLProgram&) = delete;
	CLProgram &operator=(CLProgram&&) = delete;

	// These are public in case the options menu is going to need to list them.
	// There should be a constructor that also takes a platform and device, then.
	static std::vector<cl::Platform> getPlatforms();
	static std::vector<cl::Device> getDevices(const cl::Platform &platform,
		cl_device_type type);

	// Updates the render dimensions.
	void resize(int renderWidth, int renderHeight);

	// Updates the camera data in device memory.
	void updateCamera(const Float3d &eye, const Float3d &direction, double fovY);

	// Updates the game time in device memory. Give this method total ticks instead of 
	// delta time so the constructor doesn't need a "start time". Also, this prevents 
	// any additive "double -> cl_float" error.
	void updateGameTime(double gameTime);

	// Creates a new texture in device memory and returns the texture index for when 
	// adding new geometry data. Pixel data is expected to be in ARGB8888 format.
	int addTexture(uint32_t *pixels, int width, int height);

	// Updates a voxel's geometry in device memory. Currently restricted to up to 6 
	// rectangles, and all rectangles must be updated at the same time.
	void updateVoxel(int x, int y, int z, const std::vector<Rect3D> &rects,
		const std::vector<int> &textureIndices);
	void updateVoxel(int x, int y, int z, const std::vector<Rect3D> &rects,
		int textureIndex);

	// Updates a sprite in device memory if it exists, and adds the sprite if it 
	// doesn't exist.
	void updateSprite(int spriteID, const Rect3D &rect, int textureIndex);

	// Removes a sprite from device memory.
	void removeSprite(int spriteID);

	// Runs the OpenCL program and returns a pointer to the resulting frame buffer.
	const void *render();
};

#endif
