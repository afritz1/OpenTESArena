#ifndef VOXEL_SHADING_DEFINITION_H
#define VOXEL_SHADING_DEFINITION_H

#include "../Rendering/RenderShaderUtils.h"

struct VoxelShadingDefinition
{
	static constexpr int MAX_FRAGMENT_SHADERS = 3; // For top/middle/bottom of some voxels like raised platforms.

	VertexShaderType vertexShaderType;
	FragmentShaderType fragmentShaderTypes[MAX_FRAGMENT_SHADERS];
	int fragmentShaderCount;

	VoxelShadingDefinition();

	void init(VertexShaderType vertexShaderType, FragmentShaderType fragmentShaderType);
	void init(VertexShaderType vertexShaderType);

	void addFragmentShaderType(FragmentShaderType fragmentShaderType);
};

#endif
