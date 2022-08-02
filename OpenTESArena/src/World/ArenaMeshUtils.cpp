#include "ArenaMeshUtils.h"
#include "VoxelFacing2D.h"

void ArenaMeshUtils::WriteWallMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Wall);

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> vertices =
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

	constexpr std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
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
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteWallMeshIndexBuffers(BufferView<int32_t> outOpaqueSideIndices,
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

void ArenaMeshUtils::WriteFloorMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Floor);

	constexpr std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> vertices =
	{
		// Y=1
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	constexpr std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
	{
		// Y=1
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteFloorMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

void ArenaMeshUtils::WriteCeilingMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Ceiling);

	constexpr std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> vertices =
	{
		// Y=0
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
	};

	constexpr std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
	{
		// Y=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteCeilingMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

void ArenaMeshUtils::WriteRaisedMeshGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
	BufferView<double> outVertices, BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Raised);
	const double yBottom = yOffset;
	const double yTop = (yOffset + ySize);

	// One quad per face (results in duplication; necessary for correct texture mapping).
	const std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> vertices =
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

	const std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
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
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteRaisedMeshIndexBuffers(BufferView<int32_t> outAlphaTestedSideIndices,
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

void ArenaMeshUtils::WriteDiagonalMeshGeometryBuffers(bool type1, BufferView<double> outVertices,
	BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Diagonal);

	constexpr std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> type1Vertices =
	{
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		1.0, 0.0, 1.0,
		1.0, 1.0, 1.0,
	};

	constexpr std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> type2Vertices =
	{
		1.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
	};

	const std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> &vertices = type1 ? type1Vertices : type2Vertices;

	constexpr std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	std::copy(vertices.begin(), vertices.end(), outVertices.get());
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteDiagonalMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

void ArenaMeshUtils::WriteTransparentWallMeshGeometryBuffers(BufferView<double> outVertices,
	BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::TransparentWall);

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> vertices =
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

	constexpr std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
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
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteTransparentWallMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
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

void ArenaMeshUtils::WriteEdgeMeshGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped,
	BufferView<double> outVertices, BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Edge);
	const double yBottom = yOffset;
	const double yTop = yOffset + 1.0;

	constexpr int componentCount = vertexCount * MeshUtils::COMPONENTS_PER_VERTEX;

	// @todo: might want to bias these towards the center of the voxel to avoid z-fighting.
	const std::array<double, componentCount> nearXVertices =
	{
		// X=0
		0.0, yTop, 0.0,
		0.0, yBottom, 0.0,
		0.0, yBottom, 1.0,
		0.0, yTop, 1.0
	};

	const std::array<double, componentCount> farXVertices =
	{
		// X=1
		1.0, yTop, 1.0,
		1.0, yBottom, 1.0,
		1.0, yBottom, 0.0,
		1.0, yTop, 0.0
	};

	const std::array<double, componentCount> nearZVertices =
	{
		// Z=0
		1.0, yTop, 0.0,
		1.0, yBottom, 0.0,
		0.0, yBottom, 0.0,
		0.0, yTop, 0.0
	};

	const std::array<double, componentCount> farZVertices =
	{
		// Z=1
		0.0, yTop, 1.0,
		0.0, yBottom, 1.0,
		1.0, yBottom, 1.0,
		1.0, yTop, 1.0
	};

	const std::array<double, componentCount> *vertices = nullptr;
	switch (facing)
	{
	case VoxelFacing2D::PositiveX:
		vertices = &farXVertices;
		break;
	case VoxelFacing2D::NegativeX:
		vertices = &nearXVertices;
		break;
	case VoxelFacing2D::PositiveZ:
		vertices = &farZVertices;
		break;
	case VoxelFacing2D::NegativeZ:
		vertices = &nearZVertices;
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(facing)));
	}

	constexpr int attributeCount = vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX;
	constexpr std::array<double, attributeCount> unflippedAttributes =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	constexpr std::array<double, attributeCount> flippedAttributes =
	{
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0
	};

	const std::array<double, attributeCount> &attributes = flipped ? flippedAttributes : unflippedAttributes;

	std::copy(vertices->begin(), vertices->end(), outVertices.get());
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteEdgeMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
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

void ArenaMeshUtils::WriteChasmMeshGeometryBuffers(ArenaTypes::ChasmType chasmType,
	BufferView<double> outVertices, BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Chasm);
	const double yBottom = 0.0;
	const double yTop = 1.0;

	const std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> vertices =
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

	constexpr std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
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
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteChasmMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices,
	BufferView<int32_t> outAlphaTestedIndices)
{
	constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Chasm;
	static_assert(GetOpaqueIndexBufferCount(voxelType) == 1);
	static_assert(GetAlphaTestedIndexBufferCount(voxelType) == 0); // @temp

	constexpr std::array<int32_t, GetOpaqueIndexCount(voxelType, 0)> opaqueIndices =
	{
		// Y=0
		0, 1, 2,
		2, 3, 0
	};

	// @temp: not writing chasm walls until later
	/*constexpr std::array<int32_t, GetAlphaTestedIndexCount(ArenaTypes::VoxelType::Chasm)> alphaTestedIndices =
	{
		// X=0
		4, 5, 6,
		6, 7, 4,
		// X=1
		8, 9, 10,
		10, 11, 8,
		// Z=0
		12, 13, 14,
		14, 15, 12,
		// Z=1
		16, 17, 18,
		18, 19, 16
	};*/

	std::copy(opaqueIndices.begin(), opaqueIndices.end(), outOpaqueIndices.get());
	//std::copy(alphaTestedIndices.begin(), alphaTestedIndices.end(), outAlphaTestedIndices.get()); // @todo: figure out override index buffer support (allocate all combinations ahead of time, use bitwise lookup to get the right index buffer ID?).
}

void ArenaMeshUtils::WriteDoorMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
{
	constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Door);

	// @todo: does this need to care about the door type or can we do all that in the vertex shader?

	// One quad per face (results in duplication; necessary for correct texture mapping).
	constexpr std::array<double, vertexCount * MeshUtils::COMPONENTS_PER_VERTEX> vertices =
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

	constexpr std::array<double, vertexCount * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes =
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
	std::copy(attributes.begin(), attributes.end(), outAttributes.get());
}

void ArenaMeshUtils::WriteDoorMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
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
