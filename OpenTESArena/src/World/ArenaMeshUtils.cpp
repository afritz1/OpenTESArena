#include "ArenaMeshUtils.h"
#include "../Math/Constants.h"
#include "../Voxels/VoxelFacing2D.h"

void ArenaMeshUtils::WriteWallGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Wall);

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices =
	{
		// X=0
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		// X=1
		1.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0,
		// Y=0
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		// Y=1
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=0
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=1
		0.0, 1.0, 1.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// Y=0
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		// Y=1
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Y=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Y=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteWallIndexBuffers(BufferView<int32_t> outOpaqueSideIndices,
	BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Wall;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 3);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 0);

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> sideIndices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Z=0
		16, 17, 18,
		18, 19, 16,
		// Z=1
		20, 21, 22,
		22, 23, 20
	};

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 1)> bottomIndices =
	{
		// Y=0
		8, 9, 10,
		10, 11, 8
	};

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 2)> topIndices =
	{
		// Y=1
		12, 13, 14,
		14, 15, 12
	};

	std::copy(sideIndices.begin(), sideIndices.end(), outOpaqueSideIndices.get());
	std::copy(bottomIndices.begin(), bottomIndices.end(), outOpaqueBottomIndices.get());
	std::copy(topIndices.begin(), topIndices.end(), outOpaqueTopIndices.get());
}

void ArenaMeshUtils::WriteFloorGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Floor);

	constexpr std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices =
	{
		// Y=1
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals =
	{
		// Y=1
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		// Y=1
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteFloorIndexBuffers(BufferView<int32_t> outOpaqueIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Floor;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 1);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 0);

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> indices =
	{
		// Y=1
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outOpaqueIndices.get());
}

void ArenaMeshUtils::WriteCeilingGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Ceiling);

	constexpr std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices =
	{
		// Y=0
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals =
	{
		// Y=0
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		// Y=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteCeilingIndexBuffers(BufferView<int32_t> outOpaqueIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Ceiling;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 1);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 0);

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> indices =
	{
		// Y=0
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outOpaqueIndices.get());
}

void ArenaMeshUtils::WriteRaisedGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
	BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Raised);
	const double yBottom = yOffset;
	const double yTop = yOffset + ySize;

	// One quad per face (results in duplication; necessary for correct texture mapping).
	const std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices =
	{
		// X=0
		0.0, yTop, 0.0,
		0.0, yBottom, 0.0,
		0.0, yBottom, 1.0,
		0.0, yTop, 1.0,
		// X=1
		1.0, yTop, 1.0,
		1.0, yBottom, 1.0,
		1.0, yBottom, 0.0,
		1.0, yTop, 0.0,
		// Y=0
		0.0, yBottom, 0.0,
		1.0, yBottom, 0.0,
		1.0, yBottom, 1.0,
		0.0, yBottom, 1.0,
		// Y=1
		0.0, yTop, 1.0,
		1.0, yTop, 1.0,
		1.0, yTop, 0.0,
		0.0, yTop, 0.0,
		// Z=0
		1.0, yTop, 0.0,
		1.0, yBottom, 0.0,
		0.0, yBottom, 0.0,
		0.0, yTop, 0.0,
		// Z=1
		0.0, yTop, 1.0,
		0.0, yBottom, 1.0,
		1.0, yBottom, 1.0,
		1.0, yTop, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// Y=0
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		// Y=1
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	const std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		// X=0
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop,
		// X=1
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop,
		// Y=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Y=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop,
		// Z=1
		0.0, vTop,
		0.0, vBottom,
		1.0, vBottom,
		1.0, vTop
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteRaisedIndexBuffers(BufferView<int32_t> outAlphaTestedSideIndices,
	BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Raised;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 2);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 1);

	constexpr std::array<int32_t, GetAlphaTestedIndexCount(voxelType, 0)> sideIndices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Z=0
		16, 17, 18,
		18, 19, 16,
		// Z=1
		20, 21, 22,
		22, 23, 20
	};

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> bottomIndices =
	{
		// Y=0
		8, 9, 10,
		10, 11, 8
	};

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 1)> topIndices =
	{
		// Y=1
		12, 13, 14,
		14, 15, 12
	};

	std::copy(sideIndices.begin(), sideIndices.end(), outAlphaTestedSideIndices.get());
	std::copy(bottomIndices.begin(), bottomIndices.end(), outOpaqueBottomIndices.get());
	std::copy(topIndices.begin(), topIndices.end(), outOpaqueTopIndices.get());
}

void ArenaMeshUtils::WriteDiagonalGeometryBuffers(bool type1, BufferView<double> outVertices,
	BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Diagonal);

	constexpr std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> type1Vertices =
	{
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0,
	};

	constexpr std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> type2Vertices =
	{
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
	};
	
	constexpr double halfSqrt2 = Constants::HalfSqrt2;
	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> type1Normals =
	{
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> type2Normals =
	{
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2
	};

	constexpr std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	const auto &vertices = type1 ? type1Vertices : type2Vertices;
	const auto &normals = type1 ? type1Normals : type2Normals;

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteDiagonalIndexBuffers(BufferView<int32_t> outOpaqueIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Diagonal;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 1);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 0);

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> indices =
	{
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outOpaqueIndices.get());
}

void ArenaMeshUtils::WriteTransparentWallGeometryBuffers(BufferView<double> outVertices,
	BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::TransparentWall);

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices =
	{
		// X=0
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		// X=1
		1.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0,
		// Z=0
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=1
		0.0, 1.0, 1.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteTransparentWallIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::TransparentWall;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 0);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 1);

	constexpr std::array<int32_t, GetAlphaTestedIndexCount(voxelType, 0)> indices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Z=0
		8, 9, 10,
		10, 11, 8,
		// Z=1
		12, 13, 14,
		14, 15, 12
	};

	std::copy(indices.begin(), indices.end(), outAlphaTestedIndices.get());
}

void ArenaMeshUtils::WriteEdgeGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped,
	BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Edge);
	const double yBottom = yOffset;
	const double yTop = yOffset + 1.0;

	constexpr int positionComponentCount = vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;

	// @todo: might want to bias these towards the center of the voxel to avoid z-fighting.
	const std::array<double, positionComponentCount> nearXVertices =
	{
		// X=0
		0.0, yTop, 0.0,
		0.0, yBottom, 0.0,
		0.0, yBottom, 1.0,
		0.0, yTop, 1.0
	};

	const std::array<double, positionComponentCount> farXVertices =
	{
		// X=1
		1.0, yTop, 1.0,
		1.0, yBottom, 1.0,
		1.0, yBottom, 0.0,
		1.0, yTop, 0.0
	};

	const std::array<double, positionComponentCount> nearZVertices =
	{
		// Z=0
		1.0, yTop, 0.0,
		1.0, yBottom, 0.0,
		0.0, yBottom, 0.0,
		0.0, yTop, 0.0
	};

	const std::array<double, positionComponentCount> farZVertices =
	{
		// Z=1
		0.0, yTop, 1.0,
		0.0, yBottom, 1.0,
		1.0, yBottom, 1.0,
		1.0, yTop, 1.0
	};

	constexpr int normalComponentCount = vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	constexpr std::array<double, normalComponentCount> nearXNormals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> farXNormals =
	{
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> nearZNormals =
	{
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0
	};

	constexpr std::array<double, normalComponentCount> farZNormals =
	{
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	const std::array<double, positionComponentCount> *vertices = nullptr;
	const std::array<double, normalComponentCount> *normals = nullptr;
	switch (facing)
	{
	case VoxelFacing2D::PositiveX:
		vertices = &farXVertices;
		normals = &farXNormals;
		break;
	case VoxelFacing2D::NegativeX:
		vertices = &nearXVertices;
		normals = &nearXNormals;
		break;
	case VoxelFacing2D::PositiveZ:
		vertices = &farZVertices;
		normals = &farZNormals;
		break;
	case VoxelFacing2D::NegativeZ:
		vertices = &nearZVertices;
		normals = &nearZNormals;
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(facing)));
	}

	constexpr int texCoordCount = vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX;
	constexpr std::array<double, texCoordCount> unflippedTexCoords =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	constexpr std::array<double, texCoordCount> flippedTexCoords =
	{
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0
	};

	const std::array<double, texCoordCount> &texCoords = flipped ? flippedTexCoords : unflippedTexCoords;

	std::copy(vertices->begin(), vertices->end(), outVertices.get());
	std::copy(normals->begin(), normals->end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteEdgeIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Edge;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 0);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 1);

	constexpr std::array<int32_t, GetAlphaTestedIndexCount(voxelType, 0)> indices =
	{
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outAlphaTestedIndices.get());
}

void ArenaMeshUtils::WriteChasmGeometryBuffers(ArenaTypes::ChasmType chasmType,
	BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Chasm);
	const double yBottom = 0.0;
	const double yTop = 1.0;

	const std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices =
	{
		// Y=0 (guaranteed to exist)
		0.0, yBottom, 1.0,
		1.0, yBottom, 1.0,
		1.0, yBottom, 0.0,
		0.0, yBottom, 0.0,

		// X=0
		0.0, yTop, 1.0,
		0.0, yBottom, 1.0,
		0.0, yBottom, 0.0,
		0.0, yTop, 0.0,
		// X=1
		1.0, yTop, 0.0,
		1.0, yBottom, 0.0,
		1.0, yBottom, 1.0,
		1.0, yTop, 1.0,
		// Z=0
		0.0, yTop, 0.0,
		0.0, yBottom, 0.0,
		1.0, yBottom, 0.0,
		1.0, yTop, 0.0,
		// Z=1
		1.0, yTop, 1.0,
		1.0, yBottom, 1.0,
		0.0, yBottom, 1.0,
		0.0, yTop, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals =
	{
		// Y=0 (guaranteed to exist)
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,

		// X=0
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// X=1
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		// Z=1
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		// Y=0 (guaranteed to exist)
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,

		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteChasmFloorIndexBuffers(BufferView<int32_t> outOpaqueIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Chasm;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 1);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 0);

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> opaqueIndices =
	{
		// Y=0
		0, 1, 2,
		2, 3, 0
	};

	std::copy(opaqueIndices.begin(), opaqueIndices.end(), outOpaqueIndices.get());
}

void ArenaMeshUtils::WriteChasmWallIndexBuffers(ChasmWallIndexBuffer *outNorthIndices, ChasmWallIndexBuffer *outEastIndices,
	ChasmWallIndexBuffer *outSouthIndices, ChasmWallIndexBuffer *outWestIndices)
{
	if (outNorthIndices != nullptr) // X=0
	{
		(*outNorthIndices)[0] = 4;
		(*outNorthIndices)[1] = 5;
		(*outNorthIndices)[2] = 6;
		(*outNorthIndices)[3] = 6;
		(*outNorthIndices)[4] = 7;
		(*outNorthIndices)[5] = 4;
	}

	if (outEastIndices != nullptr) // Z=0
	{
		(*outEastIndices)[0] = 12;
		(*outEastIndices)[1] = 13;
		(*outEastIndices)[2] = 14;
		(*outEastIndices)[3] = 14;
		(*outEastIndices)[4] = 15;
		(*outEastIndices)[5] = 12;
	}

	if (outSouthIndices != nullptr) // X=1
	{
		(*outSouthIndices)[0] = 8;
		(*outSouthIndices)[1] = 9;
		(*outSouthIndices)[2] = 10;
		(*outSouthIndices)[3] = 10;
		(*outSouthIndices)[4] = 11;
		(*outSouthIndices)[5] = 8;
	}

	if (outWestIndices != nullptr) // Z=1
	{
		(*outWestIndices)[0] = 16;
		(*outWestIndices)[1] = 17;
		(*outWestIndices)[2] = 18;
		(*outWestIndices)[3] = 18;
		(*outWestIndices)[4] = 19;
		(*outWestIndices)[5] = 16;
	}
}

void ArenaMeshUtils::WriteDoorGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Door);

	// @todo: does this need to care about the door type or can we do all that in the vertex shader?

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, vertexCount * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices =
	{
		// X=0
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		// X=1
		1.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0,
		// Z=0
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		// Z=1
		0.0, 1.0, 1.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteDoorIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Door;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 0);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 1);

	constexpr std::array<int32_t, GetAlphaTestedIndexCount(voxelType, 0)> indices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Z=0
		8, 9, 10,
		10, 11, 8,
		// Z=1
		12, 13, 14,
		14, 15, 12
	};

	std::copy(indices.begin(), indices.end(), outAlphaTestedIndices.get());
}
