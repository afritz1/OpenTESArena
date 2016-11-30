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

#include "ArrayReference.h"
#include "Light.h"
#include "RectData.h"
#include "../Math/Float3.h"
#include "../Math/Int3.h"
#include "../Math/Rect3D.h"

// The CLProgram manages all interactions of the application with the 3D graphics
// engine and the GPU compute schedule. It should be refreshed when the window is
// resized or when the game world's dimensions change.

// It is important to remember that cl_float3 and cl_float4 are structurally
// equivalent (cl_float * 4, or 16 bytes).

// It appears that having single-indirection sprite references (i.e., offset + count -> 
// rectangle) is faster than double-indirection (i.e., offset + count -> index -> 
// rectangle) for rendering. Though single-indirection means having some duplicated 
// geometry for each chunk pointed to by a sprite reference, it preserves the cache 
// and prevents global memory from being saturated with incoherent read requests. 
// Double-indirection would also mean passing another index buffer to the kernel.

class BufferView;

class CLProgram
{
private:
	static const std::string PATH;
	static const std::string FILENAME;
	static const std::string INTERSECT_KERNEL;
	static const std::string RAY_TRACE_KERNEL;
	static const std::string POST_PROCESS_KERNEL;
	static const std::string CONVERT_TO_RGB_KERNEL;

	cl::Device device;
	cl::Context context;
	cl::CommandQueue commandQueue;
	cl::Program program;
	cl::Kernel intersectKernel, rayTraceKernel, convertToRGBKernel;
	cl::Buffer cameraBuffer, voxelRefBuffer, spriteRefBuffer, lightRefBuffer,
		rectangleBuffer, lightBuffer, textureBuffer, gameTimeBuffer, depthBuffer,
		normalBuffer, viewBuffer, pointBuffer, uvBuffer, rectangleIndexBuffer,
		colorBuffer, outputBuffer;

	// For receiving pixels from the device's output buffer.
	std::vector<uint8_t> outputData;

	// Byte offset for voxel groups.
	std::unordered_map<Int3, size_t> voxelOffsets;

	// Byte offset and sprite ID list for sprite groups.
	std::unordered_map<Int3, std::pair<size_t, std::vector<int>>> spriteGroups;

	// Byte offset and light ID list for light groups.
	std::unordered_map<Int3, std::pair<size_t, std::vector<int>>> lightGroups;

	// Byte offset and rect index list for owner groups.
	std::unordered_map<Int3, std::pair<size_t, std::vector<int>>> ownerGroups;

	// Geometry queues. Each mapping is a rect group to be updated in device memory.
	// Each rect data object carries the rectangle and its texture index.
	std::unordered_map<Int3, std::vector<RectData>> voxelQueue, spriteQueue;

	// Light queue. Each mapping is a light group to be updated in device memory.
	std::unordered_map<Int3, std::vector<Light>> lightQueue;

	// Index queue. Each mapping is a rect index group to be updated in device memory.
	std::unordered_map<Int3, std::vector<int>> ownerQueue;

	// Reference queues. Each mapping is a reference to be updated in device memory.
	// Each queue is cleared after updating is complete for the frame.
	std::unordered_map<Int3, VoxelReference> voxelRefQueue;
	std::unordered_map<Int3, SpriteReference> spriteRefQueue;
	std::unordered_map<Int3, LightReference> lightRefQueue;

	// Voxel coordinates of a sprite's or light's touched voxels.
	std::unordered_map<int, std::vector<Int3>> spriteTouchedVoxels, lightTouchedVoxels;

	// A copy of each sprite's rectangle and texture ID.
	std::unordered_map<int, RectData> spriteData;

	// A copy of each light's data.
	std::unordered_map<int, Light> lightData;

	// For managing allocations in the rectangle buffer and light buffer.
	std::unique_ptr<BufferView> rectBufferView, lightBufferView;

	// Offsets and sizes of textures in device memory.
	std::vector<TextureReference> textureRefs;

	int renderWidth, renderHeight, worldWidth, worldHeight, worldDepth;

	std::string getBuildReport() const;
	std::string getErrorString(cl_int error) const;

	// Sets a buffer's values to zero.
	void zeroBuffer(cl::Buffer &buffer);

	// Reallocates a device buffer with the given size while retaining the previous data.
	// If the new size is smaller, the previous data is truncated. Any kernel arguments
	// using the buffer must be refreshed by the caller.
	void resizeBuffer(cl::Buffer &buffer, cl::size_type newSize);

	// Helper method for resizing the rectangle buffer and setting kernel arguments.
	void resizeRectBuffer(cl::size_type requiredSize);

	// Returns a vector of voxel coordinates for all voxels that a rectangle touches,
	// with the option to only get voxels within the world bounds. Intended only for 
	// sprites.
	std::vector<Int3> getTouchedVoxels(const Rect3D &rect, bool clampBounds) const;

	// Returns a vector of voxel coordinates for all voxels that a light reaches,
	// with the option to only get voxels within the world bounds. Intended only for 
	// point lights.
	std::vector<Int3> getTouchedVoxels(const Float3d &point, double intensity,
		bool clampBounds) const;

	// Helper method for writing a rectangle and texture reference to a temp buffer.
	void writeRect(const Rect3D &rect, const TextureReference &textureRef,
		std::vector<cl_char> &buffer, size_t byteOffset) const;

	// Helper method for writing a light to a temp buffer.
	void writeLight(const Light &light, std::vector<cl_char> &buffer, size_t byteOffset) const;

	// Helper method for adding a sprite to a sprite group in device memory.
	void addSpriteToVoxel(int spriteID, const Int3 &voxel);

	// Helper method for removing a sprite from a sprite group in device memory.
	void removeSpriteFromVoxel(int spriteID, const Int3 &voxel);

	// Helper method for updating a sprite's rectangle and texture reference in 
	// a given voxel in device memory.
	void updateSpriteInVoxel(int spriteID, const Int3 &voxel);

	// Helper method for adding a light to a light group in device memory.
	void addLightToVoxel(int lightID, const Int3 &voxel);

	// Helper method for removing a light from a light group in device memory.
	void removeLightFromVoxel(int lightID, const Int3 &voxel);

	// Helper method for updating a light's data in a given voxel in device memory.
	void updateLightInVoxel(int lightID, const Int3 &voxel);

	// Queues a voxel reference for updating in device memory.
	void queueVoxelRefUpdate(int x, int y, int z, int offset, int count);

	// Queues a sprite reference for updating in device memory.
	void queueSpriteRefUpdate(int x, int y, int z, int offset, int count);

	// Queues a light reference for updating in device memory.
	void queueLightRefUpdate(int x, int y, int z, int offset, int count);
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

	// Queues a voxel's geometry for updating. The given data vectors can be empty,
	// in which case the voxel becomes empty as well.
	void queueVoxelUpdate(int x, int y, int z, const std::vector<Rect3D> &rects,
		const std::vector<int> &textureIndices);
	void queueVoxelUpdate(int x, int y, int z, const std::vector<Rect3D> &rects,
		int textureIndex);

	// Queues a sprite for updating if it exists, and adds the sprite if it doesn't exist.
	void queueSpriteUpdate(int spriteID, const Rect3D &rect, int textureIndex);

	// Queues a sprite for removal from device memory.
	void queueSpriteRemoval(int spriteID);

	// Queues a light for updating if it exists, and adds it if it doesn't exist.
	// The owner ID is the sprite ID associated with the light, if any.
	void queueLightUpdate(int lightID, const Float3d &point, const Float3d &color,
		const int *ownerID, double intensity);

	// Queues a light for removal from device memory.
	void queueLightRemoval(int lightID);

	// Consumes all voxel and sprite updates in the queue and writes to device memory.
	void pushUpdates();

	// Runs the OpenCL program and returns a pointer to the resulting frame buffer.
	const void *render();
};

#endif
