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
