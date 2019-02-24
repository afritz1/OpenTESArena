#pragma once
#ifdef HAVE_OPENGL
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../World/VoxelGrid.h"
#include "../World/VoxelDataType.h"

typedef unsigned int GLuint;

class HardwareRenderer
{
private:
	//SoftwareRenderer Dupe
	struct VoxelTexel
	{
		double r, g, b, emission;
		bool transparent; // Voxel texels only support alpha testing, not alpha blending.

		VoxelTexel();
	};

	//Almost Duplicate of SoftwareRenderer::VoxelTexure
	struct VoxelTexture
	{
		static const int WIDTH = 64;
		static const int HEIGHT = VoxelTexture::WIDTH;
		static const int TEXEL_COUNT = VoxelTexture::WIDTH * VoxelTexture::HEIGHT;

		GLuint ID;
		bool hasAlpha = false;
		std::array<VoxelTexel, VoxelTexture::TEXEL_COUNT> texels;
		std::vector<Int2> lightTexels; // Black during the day, yellow at night.
	};

	//(Near) Duplicate of SoftwareRenderer::Camera
	struct Camera
	{
		Double3 eye; // Camera position.
		Double3 eyeVoxelReal; // 'eye' with each component floored.
		Double3 direction; // 3D direction the camera is facing.
		Int3 eyeVoxel; // 'eyeVoxelReal' converted to integers.
		Matrix4f transform; // Perspective transformation matrix. Floats instead of Doubles
		double forwardX, forwardZ; // Forward components.
		double forwardZoomedX, forwardZoomedZ; // Forward * zoom components.
		double rightX, rightZ; // Right components.
		double rightAspectedX, rightAspectedZ; // Right * aspect components.
		double frustumLeftX, frustumLeftZ; // Components of left edge of 2D frustum.
		double frustumRightX, frustumRightZ; // Components of right edge of 2D frustum.
		double fovY, zoom, aspect;
		double yAngleRadians; // Angle of the camera above or below the horizon.
		double yShear; // Projected Y-coordinate translation.

		Camera(const Double3& eye, const Double3& direction, double fovY, double aspect,
			double projectionModifier);

		// Gets the camera's Y voxel coordinate after compensating for ceiling height.
		int getAdjustedEyeVoxelY(double ceilingHeight) const;
	};

	//OpenGL ID's for various objects
	GLuint frameBuffer, colourBuffer, renderBuffer, shaderID, voxelVAO, voxelVBO, voxelEBO, cubemapArray;
	int width, height;
	std::vector<VoxelTexture> voxelTextures;
	bool firstrun = true;
	int amount = 0;

	void generateCubeMap(int index, int width, int height, const uint32_t* srcTexels);

public:

	HardwareRenderer();
	~HardwareRenderer();
	//Initialise the renderer
	void init(int width, int height);
	//Render to internal framebuffer and read into colourBuffer
	void render(const Double3 &eye, const Double3 &direction, double fovY, double ceilingHeight, const VoxelGrid &voxelGrid, uint32_t* colourBuffer);
	void setVoxelTexture(int id, const uint32_t* srcTexels);
	void createMap(const VoxelGrid& voxelGrid, int adjustedY, double ceilingHeight);
};
#endif