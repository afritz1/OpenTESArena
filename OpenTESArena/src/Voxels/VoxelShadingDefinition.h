#ifndef VOXEL_SHADING_DEFINITION_H
#define VOXEL_SHADING_DEFINITION_H

#include "../Rendering/RenderShaderUtils.h"

struct VoxelShadingDefinition
{
	static constexpr int MAX_PIXEL_SHADERS = 3; // For top/middle/bottom of some voxels like raised platforms.

	VertexShaderType vertexShaderType;
	PixelShaderType pixelShaderTypes[MAX_PIXEL_SHADERS];
	int pixelShaderCount;

	VoxelShadingDefinition();

	void init(VertexShaderType vertexShaderType, PixelShaderType pixelShaderType);
	void init(VertexShaderType vertexShaderType);

	void addPixelShaderType(PixelShaderType pixelShaderType);
};

#endif
