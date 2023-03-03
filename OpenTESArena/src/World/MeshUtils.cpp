#include "MeshUtils.h"
#include "../Voxels/ArenaChasmUtils.h"
#include "../Voxels/VoxelMeshDefinition.h"

double MeshUtils::getScaledVertexY(double meshY, VoxelMeshScaleType scaleType, double ceilingScale)
{
	switch (scaleType)
	{
	case VoxelMeshScaleType::ScaledFromMin:
		return meshY * ceilingScale;
	case VoxelMeshScaleType::UnscaledFromMin:
		return meshY;
	case VoxelMeshScaleType::UnscaledFromMax:
	{
		constexpr double chasmHeight = ArenaChasmUtils::DEFAULT_HEIGHT;
		return (meshY * chasmHeight) + (ceilingScale - chasmHeight);
	}
	default:
		DebugUnhandledReturnMsg(double, std::to_string(static_cast<int>(scaleType)));
	}
}
