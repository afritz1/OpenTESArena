#include "MeshUtils.h"
#include "../Voxels/ArenaChasmUtils.h"
#include "../Voxels/VoxelShapeDefinition.h"

double MeshUtils::getScaledVertexY(double meshY, VoxelShapeScaleType scaleType, double ceilingScale)
{
	switch (scaleType)
	{
	case VoxelShapeScaleType::ScaledFromMin:
		return meshY * ceilingScale;
	case VoxelShapeScaleType::UnscaledFromMin:
		return meshY;
	case VoxelShapeScaleType::UnscaledFromMax:
	{
		constexpr double chasmHeight = ArenaChasmUtils::DEFAULT_HEIGHT;
		return (meshY * chasmHeight) + (ceilingScale - chasmHeight);
	}
	default:
		DebugUnhandledReturnMsg(double, std::to_string(static_cast<int>(scaleType)));
	}
}
