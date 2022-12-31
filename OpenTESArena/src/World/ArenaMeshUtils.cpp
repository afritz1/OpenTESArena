#include "ArenaMeshUtils.h"
#include "../Math/Constants.h"
#include "../Voxels/VoxelFacing2D.h"

// Unique geometry vertices are lexicographically ordered for separation of responsibility from how
// they're used, and renderer vertices are ordered in the way they're consumed when being converted
// to triangles.

void ArenaMeshUtils::WriteWallUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Wall;
	constexpr std::array<double, GetUniqueVertexPositionComponentCount(voxelType)> vertices =
	{
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, GetUniqueFaceNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		// Y=0
		0.0, -1.0, 0.0,
		// Y=1
		0.0, 1.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteWallRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Wall;
	// @todo: depend on unique geometry buffers to avoid redundant data in this file

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> vertices =
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

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
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

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
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

void ArenaMeshUtils::WriteWallRendererIndexBuffers(BufferView<int32_t> outOpaqueSideIndices,
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

void ArenaMeshUtils::WriteWallCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteWallUniqueGeometryBuffers(outVertices, outNormals);
}

void ArenaMeshUtils::WriteWallCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Wall;

	// In index tuple format (position XYZ, normal XYZ, position XYZ, ...).
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// X=0
		2, 0,
		0, 0,
		4, 0,
		4, 0,
		6, 0,
		2, 0,
		// X=1
		7, 1,
		5, 1,
		1, 1,
		1, 1,
		3, 1,
		7, 1,
		// Y=0
		3, 2,
		7, 2,
		6, 2,
		6, 2,
		2, 2,
		3, 2,
		// Y=1
		4, 3,
		0, 3,
		1, 3,
		1, 3,
		5, 3,
		4, 3,
		// Z=0
		2, 4,
		0, 4,
		1, 4,
		1, 4,
		3, 4,
		2, 4,
		// Z=1
		7, 5,
		5, 5,
		4, 5,
		4, 5,
		6, 5,
		7, 5
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteFloorUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Floor;
	constexpr std::array<double, GetUniqueVertexPositionComponentCount(voxelType)> vertices =
	{
		0.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, GetUniqueFaceNormalComponentCount(voxelType)> normals =
	{
		// Y=1
		0.0, 1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteFloorRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Floor;
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> vertices =
	{
		// Y=1
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// Y=1
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
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

void ArenaMeshUtils::WriteFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

void ArenaMeshUtils::WriteFloorCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteFloorUniqueGeometryBuffers(outVertices, outNormals);
}

void ArenaMeshUtils::WriteFloorCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Floor;

	// In index tuple format (position XYZ, normal XYZ, position XYZ, ...).
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// Y=1
		2, 0,
		3, 0,
		1, 0,
		1, 0,
		0, 0,
		2, 0
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteCeilingUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Ceiling;
	constexpr std::array<double, GetUniqueVertexPositionComponentCount(voxelType)> vertices =
	{
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0
	};

	constexpr std::array<double, GetUniqueFaceNormalComponentCount(voxelType)> normals =
	{
		// Y=0
		0.0, -1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteCeilingRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Ceiling;
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> vertices =
	{
		// Y=0
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// Y=0
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
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

void ArenaMeshUtils::WriteCeilingRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

void ArenaMeshUtils::WriteCeilingCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteCeilingUniqueGeometryBuffers(outVertices, outNormals);
}

void ArenaMeshUtils::WriteCeilingCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Ceiling;

	// In index tuple format (position XYZ, normal XYZ, position XYZ, ...).
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// Y=1
		0, 0,
		1, 0,
		3, 0,
		3, 0,
		2, 0,
		0, 0
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteRaisedUniqueGeometryBuffers(double yOffset, double ySize, BufferView<double> outVertices,
	BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Raised;
	const double yBottom = yOffset;
	const double yTop = yOffset + ySize;

	const std::array<double, GetUniqueVertexPositionComponentCount(voxelType)> vertices =
	{
		0.0, yBottom, 0.0,
		1.0, yBottom, 0.0,
		0.0, yTop, 0.0,
		1.0, yTop, 0.0,
		0.0, yBottom, 1.0,
		1.0, yBottom, 1.0,
		0.0, yTop, 1.0,
		1.0, yTop, 1.0
	};

	constexpr std::array<double, GetUniqueFaceNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		// Y=0
		0.0, -1.0, 0.0,
		// Y=1
		0.0, 1.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
	BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Raised;
	const double yBottom = yOffset;
	const double yTop = yOffset + ySize;

	// One quad per face (results in duplication; necessary for correct texture mapping).
	const std::array<double, GetRendererVertexPositionComponentCount(voxelType)> vertices =
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

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
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

	const std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
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
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		// Y=1
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
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

void ArenaMeshUtils::WriteRaisedRendererIndexBuffers(BufferView<int32_t> outAlphaTestedSideIndices,
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

void ArenaMeshUtils::WriteRaisedCollisionGeometryBuffers(double yOffset, double ySize, BufferView<double> outVertices,
	BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteRaisedUniqueGeometryBuffers(yOffset, ySize, outVertices, outNormals);
}

void ArenaMeshUtils::WriteRaisedCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Raised;

	// In index tuple format (position XYZ, normal XYZ, position XYZ, ...).
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// X=0
		2, 0,
		0, 0,
		4, 0,
		4, 0,
		6, 0,
		2, 0,
		// X=1
		7, 1,
		5, 1,
		1, 1,
		1, 1,
		3, 1,
		7, 1,
		// Y=0
		3, 2,
		7, 2,
		6, 2,
		6, 2,
		2, 2,
		3, 2,
		// Y=1
		4, 3,
		0, 3,
		1, 3,
		1, 3,
		5, 3,
		4, 3,
		// Z=0
		2, 4,
		0, 4,
		1, 4,
		1, 4,
		3, 4,
		2, 4,
		// Z=1
		7, 5,
		5, 5,
		4, 5,
		4, 5,
		6, 5,
		7, 5
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteDiagonalUniqueGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Diagonal;
	constexpr int positionComponentCount = GetUniqueVertexPositionComponentCount(voxelType);
	constexpr std::array<double, positionComponentCount> type1Vertices =
	{
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, positionComponentCount> type2Vertices =
	{
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0
	};

	constexpr int normalComponentsCount = GetUniqueFaceNormalComponentCount(voxelType);
	constexpr double halfSqrt2 = Constants::HalfSqrt2;
	constexpr std::array<double, normalComponentsCount> type1Normals =
	{
		// Front
		-halfSqrt2, 0.0, halfSqrt2,
		// Back
		halfSqrt2, 0.0, -halfSqrt2
	};

	constexpr std::array<double, normalComponentsCount> type2Normals =
	{
		// Front
		-halfSqrt2, 0.0, -halfSqrt2,
		// Back
		halfSqrt2, 0.0, halfSqrt2
	};

	const auto &vertices = type1 ? type1Vertices : type2Vertices;
	const auto &normals = type1 ? type1Normals : type2Normals;

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteDiagonalRendererGeometryBuffers(bool type1, BufferView<double> outVertices,
	BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Diagonal;
	constexpr int positionComponentCount = GetRendererVertexPositionComponentCount(voxelType);
	constexpr std::array<double, positionComponentCount> type1Vertices =
	{
		// Front
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0,

		// Back
		1.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, positionComponentCount> type2Vertices =
	{
		// Front
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,

		// Back
		0.0, 1.0, 1.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0
	};

	constexpr int normalComponentsCount = GetRendererVertexNormalComponentCount(voxelType);
	constexpr double halfSqrt2 = Constants::HalfSqrt2;
	constexpr std::array<double, normalComponentsCount> type1Normals =
	{
		// Front
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,
		-halfSqrt2, 0.0, halfSqrt2,

		// Back
		halfSqrt2, 0.0, -halfSqrt2,
		halfSqrt2, 0.0, -halfSqrt2,
		halfSqrt2, 0.0, -halfSqrt2,
		halfSqrt2, 0.0, -halfSqrt2
	};

	constexpr std::array<double, normalComponentsCount> type2Normals =
	{
		// Front
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,
		-halfSqrt2, 0.0, -halfSqrt2,

		// Back
		halfSqrt2, 0.0, halfSqrt2,
		halfSqrt2, 0.0, halfSqrt2,
		halfSqrt2, 0.0, halfSqrt2,
		halfSqrt2, 0.0, halfSqrt2
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// Front
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,

		// Back
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0
	};

	const auto &vertices = type1 ? type1Vertices : type2Vertices;
	const auto &normals = type1 ? type1Normals : type2Normals;

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteDiagonalRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Diagonal;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 1);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 0);

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> indices =
	{
		// Front
		0, 1, 2,
		2, 3, 0,

		// Back
		4, 5, 6,
		6, 7, 4
	};

	std::copy(indices.begin(), indices.end(), outOpaqueIndices.get());
}

void ArenaMeshUtils::WriteDiagonalCollisionGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteDiagonalUniqueGeometryBuffers(type1, outVertices, outNormals);
}

void ArenaMeshUtils::WriteDiagonalCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Diagonal;
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// Front
		0, 0,
		1, 0,
		2, 0,
		2, 0,
		3, 0,
		0, 0,

		// Back (same as front due to there only being four vertices per diagonal variant)
		0, 1,
		1, 1,
		2, 1,
		2, 1,
		3, 1,
		0, 1
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteTransparentWallUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::TransparentWall;
	constexpr std::array<double, GetUniqueVertexPositionComponentCount(voxelType)> vertices =
	{
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, GetUniqueFaceNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteTransparentWallRendererGeometryBuffers(BufferView<double> outVertices,
	BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::TransparentWall;

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> vertices =
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

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
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

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
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

void ArenaMeshUtils::WriteTransparentWallRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
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

void ArenaMeshUtils::WriteTransparentWallCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteTransparentWallUniqueGeometryBuffers(outVertices, outNormals);
}

void ArenaMeshUtils::WriteTransparentWallCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::TransparentWall;

	// In index tuple format (position XYZ, normal XYZ, position XYZ, ...).
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// X=0
		2, 0,
		0, 0,
		4, 0,
		4, 0,
		6, 0,
		2, 0,
		// X=1
		7, 1,
		5, 1,
		1, 1,
		1, 1,
		3, 1,
		7, 1,
		// Z=0
		3, 2,
		7, 2,
		6, 2,
		6, 2,
		2, 2,
		3, 2,
		// Z=1
		4, 3,
		0, 3,
		1, 3,
		1, 3,
		5, 3,
		4, 3
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteEdgeUniqueGeometryBuffers(VoxelFacing2D facing, double yOffset, BufferView<double> outVertices,
	BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Edge;

	// Bias the geometry towards the center of the voxel to avoid Z-fighting.
	constexpr double xBiasMin = Constants::Epsilon;
	constexpr double xBiasMax = 1.0 - Constants::Epsilon;
	const double yBottom = yOffset;
	const double yTop = yOffset + 1.0;
	constexpr double zBiasMin = xBiasMin;
	constexpr double zBiasMax = xBiasMax;

	constexpr int positionComponentCount = GetUniqueVertexPositionComponentCount(voxelType);
	const std::array<double, positionComponentCount> nearXVertices =
	{
		// X=0
		xBiasMin, yTop, 0.0,
		xBiasMin, yBottom, 0.0,
		xBiasMin, yBottom, 1.0,
		xBiasMin, yTop, 1.0
	};

	const std::array<double, positionComponentCount> farXVertices =
	{
		// X=1
		xBiasMax, yTop, 1.0,
		xBiasMax, yBottom, 1.0,
		xBiasMax, yBottom, 0.0,
		xBiasMax, yTop, 0.0
	};

	const std::array<double, positionComponentCount> nearZVertices =
	{
		// Z=0
		1.0, yTop, zBiasMin,
		1.0, yBottom, zBiasMin,
		0.0, yBottom, zBiasMin,
		0.0, yTop, zBiasMin
	};

	const std::array<double, positionComponentCount> farZVertices =
	{
		// Z=1
		0.0, yTop, zBiasMax,
		0.0, yBottom, zBiasMax,
		1.0, yBottom, zBiasMax,
		1.0, yTop, zBiasMax
	};

	constexpr int normalComponentCount = GetUniqueFaceNormalComponentCount(voxelType);
	constexpr std::array<double, normalComponentCount> nearXNormals =
	{
		// X=0 Front
		-1.0, 0.0, 0.0,
		// X=0 Back
		1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> farXNormals =
	{
		// X=1 Front
		1.0, 0.0, 0.0,
		// X=1 Back
		-1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> nearZNormals =
	{
		// Z=0 Front
		0.0, 0.0, -1.0,
		// Z=0 Back
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, normalComponentCount> farZNormals =
	{
		// Z=1 Front
		0.0, 0.0, 1.0,
		// Z=1 Back
		0.0, 0.0, -1.0
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

	std::copy(vertices->begin(), vertices->end(), outVertices.get());
	std::copy(normals->begin(), normals->end(), outNormals.get());
}

void ArenaMeshUtils::WriteEdgeRendererGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped,
	BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Edge;

	// Bias the geometry towards the center of the voxel to avoid Z-fighting.
	constexpr double xBiasMin = Constants::Epsilon;
	constexpr double xBiasMax = 1.0 - Constants::Epsilon;
	const double yBottom = yOffset;
	const double yTop = yOffset + 1.0;
	constexpr double zBiasMin = xBiasMin;
	constexpr double zBiasMax = xBiasMax;

	constexpr int positionComponentCount = GetRendererVertexPositionComponentCount(voxelType);
	const std::array<double, positionComponentCount> nearXVertices =
	{
		// X=0 Front
		xBiasMin, yTop, 0.0,
		xBiasMin, yBottom, 0.0,
		xBiasMin, yBottom, 1.0,
		xBiasMin, yTop, 1.0,

		// X=0 Back
		xBiasMin, yTop, 1.0,
		xBiasMin, yBottom, 1.0,
		xBiasMin, yBottom, 0.0,
		xBiasMin, yTop, 0.0
	};

	const std::array<double, positionComponentCount> farXVertices =
	{
		// X=1 Front
		xBiasMax, yTop, 1.0,
		xBiasMax, yBottom, 1.0,
		xBiasMax, yBottom, 0.0,
		xBiasMax, yTop, 0.0,

		// X=1 Back
		xBiasMax, yTop, 0.0,
		xBiasMax, yBottom, 0.0,
		xBiasMax, yBottom, 1.0,
		xBiasMax, yTop, 1.0
	};

	const std::array<double, positionComponentCount> nearZVertices =
	{
		// Z=0 Front
		1.0, yTop, zBiasMin,
		1.0, yBottom, zBiasMin,
		0.0, yBottom, zBiasMin,
		0.0, yTop, zBiasMin,

		// Z=0 Back
		0.0, yTop, zBiasMin,
		0.0, yBottom, zBiasMin,
		1.0, yBottom, zBiasMin,
		1.0, yTop, zBiasMin
	};

	const std::array<double, positionComponentCount> farZVertices =
	{
		// Z=1 Front
		0.0, yTop, zBiasMax,
		0.0, yBottom, zBiasMax,
		1.0, yBottom, zBiasMax,
		1.0, yTop, zBiasMax,

		// Z=1 Back
		1.0, yTop, zBiasMax,
		1.0, yBottom, zBiasMax,
		0.0, yBottom, zBiasMax,
		0.0, yTop, zBiasMax
	};

	constexpr int normalComponentCount = GetRendererVertexNormalComponentCount(voxelType);
	constexpr std::array<double, normalComponentCount> nearXNormals =
	{
		// X=0 Front
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,

		// X=0 Back
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> farXNormals =
	{
		// X=1 Front
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,

		// X=1 Back
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0
	};

	constexpr std::array<double, normalComponentCount> nearZNormals =
	{
		// Z=0 Front
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,

		// Z=0 Back
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr std::array<double, normalComponentCount> farZNormals =
	{
		// Z=1 Front
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,

		// Z=1 Back
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0
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

	constexpr int texCoordCount = GetRendererVertexTexCoordComponentCount(voxelType);
	constexpr std::array<double, texCoordCount> unflippedTexCoords =
	{
		// Front
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,

		// Back
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0
	};

	constexpr std::array<double, texCoordCount> flippedTexCoords =
	{
		// Front
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,

		// Back
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	const std::array<double, texCoordCount> &texCoords = flipped ? flippedTexCoords : unflippedTexCoords;

	std::copy(vertices->begin(), vertices->end(), outVertices.get());
	std::copy(normals->begin(), normals->end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteEdgeRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Edge;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 0);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 1);

	constexpr std::array<int32_t, GetAlphaTestedIndexCount(voxelType, 0)> indices =
	{
		// Front
		0, 1, 2,
		2, 3, 0,

		// Back
		4, 5, 6,
		6, 7, 4
	};

	std::copy(indices.begin(), indices.end(), outAlphaTestedIndices.get());
}

void ArenaMeshUtils::WriteEdgeCollisionGeometryBuffers(VoxelFacing2D facing, double yOffset, BufferView<double> outVertices,
	BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteEdgeUniqueGeometryBuffers(facing, yOffset, outVertices, outNormals);
}

void ArenaMeshUtils::WriteEdgeCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Edge;
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// Front
		0, 0,
		1, 0,
		2, 0,
		2, 0,
		3, 0,
		0, 0,

		// Back
		4, 1,
		5, 1,
		6, 1,
		6, 1,
		7, 1,
		4, 1
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteChasmUniqueGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices,
	BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Chasm;
	const std::array<double, GetUniqueVertexPositionComponentCount(voxelType)> vertices =
	{
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, GetUniqueFaceNormalComponentCount(voxelType)> normals =
	{
		// Y=0 (guaranteed to exist)
		0.0, 1.0, 0.0,

		// X=0
		1.0, 0.0, 0.0,
		// X=1
		-1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, 1.0,
		// Z=1
		0.0, 0.0, -1.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteChasmRendererGeometryBuffers(ArenaTypes::ChasmType chasmType,
	BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Chasm;
	const double yBottom = 0.0;
	const double yTop = 1.0;

	const std::array<double, GetRendererVertexPositionComponentCount(voxelType)> vertices =
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

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
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

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
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

void ArenaMeshUtils::WriteChasmFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

void ArenaMeshUtils::WriteChasmWallRendererIndexBuffers(ChasmWallIndexBuffer *outNorthIndices, ChasmWallIndexBuffer *outEastIndices,
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

void ArenaMeshUtils::WriteChasmCollisionGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices, BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteChasmUniqueGeometryBuffers(chasmType, outVertices, outNormals);
}

void ArenaMeshUtils::WriteChasmFloorCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Chasm;

	// In index tuple format (position XYZ, normal XYZ, position XYZ, ...).
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// Y=0
		3, 2,
		7, 2,
		6, 2,
		6, 2,
		2, 2,
		3, 2
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}

void ArenaMeshUtils::WriteChasmWallCollisionIndexBuffers(ChasmWallIndexBuffer *outNorthIndices, ChasmWallIndexBuffer *outEastIndices,
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

void ArenaMeshUtils::WriteDoorUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Door;
	constexpr std::array<double, GetUniqueVertexPositionComponentCount(voxelType)> vertices =
	{
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0
	};

	constexpr std::array<double, GetUniqueFaceNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		// X=1
		1.0, 0.0, 0.0,
		// Z=0
		0.0, 0.0, -1.0,
		// Z=1
		0.0, 0.0, 1.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
}

void ArenaMeshUtils::WriteDoorRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals,
	BufferView<double> outTexCoords)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Door;

	// @todo: this will probably have double the vertices for splitting doors.

	// One quad that gets translated/rotated per face.
	constexpr std::array<double, GetRendererVertexPositionComponentCount(voxelType)> vertices =
	{
		// X=0
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0
	};

	constexpr std::array<double, GetRendererVertexNormalComponentCount(voxelType)> normals =
	{
		// X=0
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0
	};

	constexpr std::array<double, GetRendererVertexTexCoordComponentCount(voxelType)> texCoords =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(normals.begin(), normals.end(), outNormals.get());
	std::copy(texCoords.begin(), texCoords.end(), outTexCoords.get());
}

void ArenaMeshUtils::WriteDoorRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Door;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 0);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 1);

	constexpr std::array<int32_t, GetAlphaTestedIndexCount(voxelType, 0)> indices =
	{
		// X=0
		0, 1, 2,
		2, 3, 0
	};

	std::copy(indices.begin(), indices.end(), outAlphaTestedIndices.get());
}

void ArenaMeshUtils::WriteDoorCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals)
{
	ArenaMeshUtils::WriteDoorUniqueGeometryBuffers(outVertices, outNormals);
}

void ArenaMeshUtils::WriteDoorCollisionIndexBuffers(BufferView<int32_t> outIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Door;

	// In index tuple format (position XYZ, normal XYZ, position XYZ, ...).
	constexpr std::array<int32_t, GetCollisionIndexCount(voxelType)> indices =
	{
		// X=0
		2, 0,
		0, 0,
		4, 0,
		4, 0,
		6, 0,
		2, 0,
		// X=1
		7, 1,
		5, 1,
		1, 1,
		1, 1,
		3, 1,
		7, 1,
		// Z=0
		3, 2,
		1, 2,
		0, 2,
		0, 2,
		2, 2,
		3, 2,
		// Z=1
		6, 3,
		4, 3,
		5, 3,
		5, 3,
		7, 3,
		6, 3
	};

	std::copy(indices.begin(), indices.end(), outIndices.get());
}
