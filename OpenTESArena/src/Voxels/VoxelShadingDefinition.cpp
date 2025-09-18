#include <algorithm>

#include "VoxelShadingDefinition.h"

#include "components/debug/Debug.h"

VoxelShadingDefinition::VoxelShadingDefinition()
{
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	std::fill(std::begin(this->fragmentShaderTypes), std::end(this->fragmentShaderTypes), static_cast<FragmentShaderType>(-1));
	this->fragmentShaderCount = 0;
}

void VoxelShadingDefinition::init(VertexShaderType vertexShaderType, FragmentShaderType fragmentShaderType)
{
	this->vertexShaderType = vertexShaderType;
	this->fragmentShaderTypes[0] = fragmentShaderType;
	this->fragmentShaderCount = 1;
}

void VoxelShadingDefinition::init(VertexShaderType vertexShaderType)
{
	this->vertexShaderType = vertexShaderType;
	this->fragmentShaderCount = 0;
}

void VoxelShadingDefinition::addFragmentShaderType(FragmentShaderType fragmentShaderType)
{
	if (this->fragmentShaderCount == MAX_FRAGMENT_SHADERS)
	{
		DebugLogErrorFormat("Too many fragment shaders in voxel shading definition, can't add type %d.", fragmentShaderType);
		return;
	}

	this->fragmentShaderTypes[this->fragmentShaderCount] = fragmentShaderType;
	this->fragmentShaderCount++;
}
