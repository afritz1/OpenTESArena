#include <algorithm>

#include "VoxelShadingDefinition.h"

#include "components/debug/Debug.h"

VoxelShadingDefinition::VoxelShadingDefinition()
{
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	std::fill(std::begin(this->pixelShaderTypes), std::end(this->pixelShaderTypes), static_cast<PixelShaderType>(-1));
	this->pixelShaderCount = 0;
}

void VoxelShadingDefinition::init(VertexShaderType vertexShaderType, PixelShaderType pixelShaderType)
{
	this->vertexShaderType = vertexShaderType;
	this->pixelShaderTypes[0] = pixelShaderType;
	this->pixelShaderCount = 1;
}

void VoxelShadingDefinition::init(VertexShaderType vertexShaderType)
{
	this->vertexShaderType = vertexShaderType;
	this->pixelShaderCount = 0;
}

void VoxelShadingDefinition::addPixelShaderType(PixelShaderType pixelShaderType)
{
	if (this->pixelShaderCount == MAX_PIXEL_SHADERS)
	{
		DebugLogErrorFormat("Too many pixel shaders in voxel shading definition, can't add type %d.", pixelShaderType);
		return;
	}

	this->pixelShaderTypes[this->pixelShaderCount] = pixelShaderType;
	this->pixelShaderCount++;
}
