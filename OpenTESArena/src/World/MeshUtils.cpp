#include <cmath>

#include "MeshUtils.h"
#include "../Math/MathUtils.h"
#include "../Voxels/ArenaChasmUtils.h"
#include "../Voxels/VoxelShapeDefinition.h"

int MeshUtils::getVertexCount(BufferView<const double> components, int componentsPerVertex)
{
	return components.getCount() / componentsPerVertex;
}

int MeshUtils::getTriangleCount(BufferView<const double> components, int componentsPerVertex)
{
	const int componentsPerTriangle = componentsPerVertex * 3;
	return components.getCount() / componentsPerTriangle;
}

bool MeshUtils::isEmpty(BufferView<const double> components)
{
	return components.getCount() == 0;
}

bool MeshUtils::isFinite(BufferView<const double> components)
{
	for (int i = 0; i < components.getCount(); i++)
	{
		const double component = components[i];
		if (!std::isfinite(component))
		{
			return false;
		}
	}

	return true;
}

bool MeshUtils::isValid(BufferView<const double> components)
{
	if (MeshUtils::isEmpty(components))
	{
		return false;
	}

	if (!MeshUtils::isFinite(components))
	{
		return false;
	}

	return true;
}

bool MeshUtils::isComplete(BufferView<const double> components, int componentsPerVertex)
{
	const int componentCount = components.getCount();
	const int componentsPerTriangle = componentsPerVertex * 3;
	return (componentCount > 0) && (componentCount % componentsPerTriangle) == 0;
}

bool MeshUtils::hasValidNormals(BufferView<const double> normals)
{
	if (!MeshUtils::isValid(normals))
	{
		return false;
	}

	constexpr int componentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	const int normalCount = MeshUtils::getVertexCount(normals, componentsPerVertex);
	for (int i = 0; i < normalCount; i++)
	{
		const int componentIndex = i * componentsPerVertex;
		const double normalX = normals[componentIndex];
		const double normalY = normals[componentIndex + 1];
		const double normalZ = normals[componentIndex + 2];
		const Double3 normal(normalX, normalY, normalZ);
		if (!normal.isNormalized())
		{
			return false;
		}
	}

	return true;
}

bool MeshUtils::hasValidTexCoords(BufferView<const double> uvs, double maxU, double maxV)
{
	if (!MeshUtils::isValid(uvs))
	{
		return false;
	}

	constexpr int componentsPerVertex = MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
	const int texCoordCount = MeshUtils::getVertexCount(uvs, componentsPerVertex);
	for (int i = 0; i < texCoordCount; i++)
	{
		const int componentIndex = i * componentsPerVertex;
		const double u = uvs[componentIndex];
		const double v = uvs[componentIndex + 1];
		if ((u < 0.0) || (u > maxU))
		{
			return false;
		}
		else if ((v < 0.0) || (v > maxU))
		{
			return false;
		}
	}

	return true;
}

Double3 MeshUtils::getVertexPositionAtIndex(BufferView<const double> positions, int vertexIndex)
{
	const int componentIndex = vertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	const double x = positions[componentIndex];
	const double y = positions[componentIndex + 1];
	const double z = positions[componentIndex + 2];
	return Double3(x, y, z);
}

Double3 MeshUtils::getVertexNormalAtIndex(BufferView<const double> normals, int vertexIndex)
{
	const int componentIndex = vertexIndex * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	const double x = normals[componentIndex];
	const double y = normals[componentIndex + 1];
	const double z = normals[componentIndex + 2];
	return Double3(x, y, z);
}

Double2 MeshUtils::getVertexTexCoordAtIndex(BufferView<const double> uvs, int vertexIndex)
{
	const int componentIndex = vertexIndex * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
	const double u = uvs[componentIndex];
	const double v = uvs[componentIndex + 1];
	return Double2(u, v);
}

int MeshUtils::findDuplicateVertexPosition(BufferView<const double> positions, double x, double y, double z, int startVertexIndex)
{
	const int componentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	const int vertexCount = MeshUtils::getVertexCount(positions, componentsPerVertex);
	for (int i = startVertexIndex; i < vertexCount; i++)
	{
		const int componentIndex = i * componentsPerVertex;
		const double currentX = positions[componentIndex];
		const double currentY = positions[componentIndex + 1];
		const double currentZ = positions[componentIndex + 2];
		const bool matchesX = MathUtils::almostEqual(currentX, x);
		const bool matchesY = MathUtils::almostEqual(currentY, y);
		const bool matchesZ = MathUtils::almostEqual(currentZ, z);
		if (matchesX && matchesY && matchesZ)
		{
			return i;
		}
	}

	return -1;
}

Double3 MeshUtils::createVertexNormalAtIndex(BufferView<const double> positions, int vertexIndex)
{
	const int componentIndex = vertexIndex * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	const double v0X = positions[componentIndex];
	const double v0Y = positions[componentIndex + 1];
	const double v0Z = positions[componentIndex + 2];
	const double v1X = positions[componentIndex + 3];
	const double v1Y = positions[componentIndex + 4];
	const double v1Z = positions[componentIndex + 5];
	const double v2X = positions[componentIndex + 6];
	const double v2Y = positions[componentIndex + 7];
	const double v2Z = positions[componentIndex + 8];
	const Double3 v0(v0X, v0Y, v0Z);
	const Double3 v1(v1X, v1Y, v1Z);
	const Double3 v2(v2X, v2Y, v2Z);
	const Double3 v0v1 = v1 - v0;
	const Double3 v1v2 = v2 - v1;
	const Double3 cross = v0v1.cross(v1v2).normalized();
	return cross;
}

void MeshUtils::createVoxelFaceQuadPositionsModelSpace(const VoxelInt3 &min, const VoxelInt3 &max, VoxelFacing3D facing, double ceilingScale, BufferView<Double3> outPositions)
{
	DebugAssert(outPositions.getCount() == 4);

	const VoxelInt3 voxelDiff = max - min;
	const VoxelDouble3 voxelDiffReal(
		static_cast<SNDouble>(voxelDiff.x),
		static_cast<double>(voxelDiff.y),
		static_cast<WEDouble>(voxelDiff.z));

	Double3 tlModelSpacePoint;
	Double3 tlBlDelta;
	Double3 tlBrDelta;

	switch (facing)
	{
	case VoxelFacing3D::PositiveX:
		tlModelSpacePoint = Double3(1.0, ceilingScale, 1.0 + voxelDiffReal.z);
		tlBlDelta = Double3(0.0, -ceilingScale, 0.0);
		tlBrDelta = Double3(0.0, 0.0, -1.0 - voxelDiffReal.z);
		break;
	case VoxelFacing3D::NegativeX:
		tlModelSpacePoint = Double3(0.0, ceilingScale, 0.0);
		tlBlDelta = Double3(0.0, -ceilingScale, 0.0);
		tlBrDelta = Double3();
		break;
	case VoxelFacing3D::PositiveY:
		tlModelSpacePoint = Double3(0.0, ceilingScale, 0.0);
		tlBlDelta = Double3(0.0, 0.0, voxelDiffReal.z);
		tlBrDelta = Double3(voxelDiffReal.x, 0.0, 0.0);
		break;
	case VoxelFacing3D::NegativeY:
		tlModelSpacePoint = Double3(0.0, 0.0, 0.0);
		tlBlDelta = Double3(1.0 + voxelDiffReal.x, 0.0, 0.0);
		tlBrDelta = Double3(0.0, 0.0, 1.0 + voxelDiffReal.z);
		break;
	case VoxelFacing3D::PositiveZ:
		tlModelSpacePoint = Double3(0.0, ceilingScale, 1.0);
		tlBlDelta = Double3(0.0, -ceilingScale, 0.0);
		tlBrDelta = Double3(1.0 + voxelDiffReal.x, 0.0, 0.0);
		break;
	case VoxelFacing3D::NegativeZ:
		tlModelSpacePoint = Double3(1.0 + voxelDiffReal.x, ceilingScale, 0.0);
		tlBlDelta = Double3(0.0, -ceilingScale, 0.0);
		tlBrDelta = Double3(-1.0 - voxelDiffReal.x, 0.0, 0.0);
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(facing)));
	}

	const Double3 v0 = tlModelSpacePoint;
	const Double3 v1 = v0 + tlBlDelta;
	const Double3 v2 = v0 + tlBlDelta + tlBrDelta;
	const Double3 v3 = v0 + tlBrDelta;
	outPositions[0] = v0;
	outPositions[1] = v1;
	outPositions[2] = v2;
	outPositions[3] = v3;
}

void MeshUtils::createVoxelFaceQuadNormals(VoxelFacing3D facing, BufferView<Double3> outNormals)
{
	DebugAssert(outNormals.getCount() == 4);

	const Double3 normal = VoxelUtils::getNormal(facing);
	outNormals[0] = normal;
	outNormals[1] = normal;
	outNormals[2] = normal;
	outNormals[3] = normal;
}

void MeshUtils::createVoxelFaceQuadTexCoords(int width, int height, BufferView<Double2> outUVs)
{
	DebugAssert(width >= 1);
	DebugAssert(height >= 1);
	DebugAssert(outUVs.getCount() == 4);

	const double uMin = 0.0;
	const double uMax = 1.0; // @todo for GL_REPEAT support, change to width
	const double vMin = 0.0;
	const double vMax = 1.0; // @todo for GL_REPEAT support, change to height
	outUVs[0] = Double2(uMin, vMin);
	outUVs[1] = Double2(uMin, vMax);
	outUVs[2] = Double2(uMax, vMax);
	outUVs[3] = Double2(uMax, vMin);
}

void MeshUtils::createVoxelFaceQuadIndices(BufferView<int32_t> outIndices)
{
	DebugAssert(outIndices.getCount() == 6);
	outIndices[0] = 0;
	outIndices[1] = 1;
	outIndices[2] = 2;
	outIndices[3] = 2;
	outIndices[4] = 3;
	outIndices[5] = 0;
}

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
