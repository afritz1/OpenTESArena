#include "VoxelDefinition.h"
#include "VoxelMeshDefinition.h"
#include "VoxelFacing2D.h"

#include "components/debug/Debug.h"

namespace
{
	// The "ideal" vertices per voxel (no duplication).
	constexpr int GetUniqueVertexCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
			return 0;
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Door:
			return 8;
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Edge:
			return 4;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// The actual vertices per voxel used by the renderer due to how vertex attributes work.
	constexpr int GetRendererVertexCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
			return 0;
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Raised:
			return 24;
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
			return 16;
		case ArenaTypes::VoxelType::Chasm:
			return 20;
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Edge:
			return 4;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetRendererVertexComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * VoxelMeshDefinition::COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexAttributeCount(ArenaTypes::VoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX;
	}

	constexpr int GetOpaqueIndexBufferCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
			return 3;
		case ArenaTypes::VoxelType::Raised:
			return 2;
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Chasm:
			return 1;
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Door:
			return 0;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetOpaqueIndexCount(ArenaTypes::VoxelType voxelType, int bufferIndex)
	{
		int triangleCount = -1;

		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
		case ArenaTypes::VoxelType::Edge:
			// @todo: should this static_assert false instead?
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
		case ArenaTypes::VoxelType::Wall:
			if (bufferIndex == 0)
			{
				triangleCount = 8;
			}
			else if ((bufferIndex == 1) || (bufferIndex == 2))
			{
				triangleCount = 2;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaTypes::VoxelType::Raised:
			if ((bufferIndex == 0) || (bufferIndex == 1))
			{
				triangleCount = 4;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
			if (bufferIndex == 0)
			{
				triangleCount = 2;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}

		return triangleCount * VoxelMeshDefinition::INDICES_PER_TRIANGLE;
	}

	constexpr int GetAlphaTestedIndexBufferCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Chasm:
			return 0;
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Door:
			return 1;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetAlphaTestedIndexCount(ArenaTypes::VoxelType voxelType, int bufferIndex)
	{
		int triangleCount = -1;

		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Chasm:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
		case ArenaTypes::VoxelType::Raised:
			if (bufferIndex == 0)
			{
				triangleCount = 12;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
			if (bufferIndex == 0)
			{
				triangleCount = 8;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaTypes::VoxelType::Edge:
			if (bufferIndex == 0)
			{
				triangleCount = 2;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}

		return triangleCount * VoxelMeshDefinition::INDICES_PER_TRIANGLE;
	}

	bool AllowsBackFacingGeometry(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
			return false;
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Chasm:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Mesh writing functions. All of these are in model space, and are eventually scaled by ceilingScale.
	void WriteWallMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Wall);

		// One quad per face (results in duplication; necessary for correct texture mapping).
		constexpr std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> vertices =
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

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
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

	void WriteWallMeshIndexBuffers(BufferView<int32_t> outOpaqueSideIndices, BufferView<int32_t> outOpaqueBottomIndices,
		BufferView<int32_t> outOpaqueTopIndices)
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

	void WriteFloorMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Floor);

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> vertices =
		{
			// Y=1
			0.0, 1.0, 1.0,
			1.0, 1.0, 1.0,
			1.0, 1.0, 0.0,
			0.0, 1.0, 0.0
		};

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
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

	void WriteFloorMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

	void WriteCeilingMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Ceiling);

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> vertices =
		{
			// Y=0
			0.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
		};

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
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

	void WriteCeilingMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

	void WriteRaisedMeshGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Raised);
		const double yBottom = yOffset;
		const double yTop = (yOffset + ySize);

		// One quad per face (results in duplication; necessary for correct texture mapping).
		const std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> vertices =
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

		const std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
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

	void WriteRaisedMeshIndexBuffers(BufferView<int32_t> outAlphaTestedSideIndices,
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

	void WriteDiagonalMeshGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Diagonal);

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> type1Vertices =
		{
			0.0, 1.0, 0.0,
			0.0, 0.0, 0.0,
			1.0, 0.0, 1.0,
			1.0, 1.0, 1.0,
		};

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> type2Vertices =
		{
			1.0, 1.0, 0.0,
			1.0, 0.0, 0.0,
			0.0, 0.0, 1.0,
			0.0, 1.0, 1.0,
		};

		const std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> &vertices = type1 ? type1Vertices : type2Vertices;

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
		{
			0.0, 0.0,
			0.0, 1.0,
			1.0, 1.0,
			1.0, 0.0
		};

		std::copy(vertices.begin(), vertices.end(), outVertices.get());
		std::copy(attributes.begin(), attributes.end(), outAttributes.get());
	}

	void WriteDiagonalMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices)
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

	void WriteTransparentWallMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::TransparentWall);

		// One quad per face (results in duplication; necessary for correct texture mapping).
		constexpr std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> vertices =
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

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
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

	void WriteTransparentWallMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
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

	void WriteEdgeMeshGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped, BufferView<double> outVertices,
		BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Edge);
		const double yBottom = yOffset;
		const double yTop = yOffset + 1.0;

		constexpr int componentCount = vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX;

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

		constexpr int attributeCount = vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX;
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

	void WriteEdgeMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
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

	void WriteChasmMeshGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices,
		BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Chasm);
		const double yBottom = 0.0;
		const double yTop = 1.0;

		const std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> vertices =
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

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
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

	void WriteChasmMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices, BufferView<int32_t> outAlphaTestedIndices)
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

	void WriteDoorMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetRendererVertexCount(ArenaTypes::VoxelType::Door);

		// @todo: does this need to care about the door type or can we do all that in the vertex shader?

		// One quad per face (results in duplication; necessary for correct texture mapping).
		constexpr std::array<double, vertexCount * VoxelMeshDefinition::COMPONENTS_PER_VERTEX> vertices =
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

		constexpr std::array<double, vertexCount * VoxelMeshDefinition::ATTRIBUTES_PER_VERTEX> attributes =
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

	void WriteDoorMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
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

	void WriteGeometryBuffers(const VoxelDefinition &voxelDef, BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		const ArenaTypes::VoxelType voxelType = voxelDef.type;
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
			WriteWallMeshGeometryBuffers(outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Floor:
			WriteFloorMeshGeometryBuffers(outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Ceiling:
			WriteCeilingMeshGeometryBuffers(outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Raised:
		{
			const VoxelDefinition::RaisedData &raised = voxelDef.raised;
			WriteRaisedMeshGeometryBuffers(raised.yOffset, raised.ySize, raised.vBottom, raised.vTop, outVertices, outAttributes);
			break;
		}
		case ArenaTypes::VoxelType::Diagonal:
		{
			const VoxelDefinition::DiagonalData &diagonal = voxelDef.diagonal;
			WriteDiagonalMeshGeometryBuffers(diagonal.type1, outVertices, outAttributes);
			break;
		}
		case ArenaTypes::VoxelType::TransparentWall:
			WriteTransparentWallMeshGeometryBuffers(outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Edge:
		{
			const VoxelDefinition::EdgeData &edge = voxelDef.edge;
			WriteEdgeMeshGeometryBuffers(edge.facing, edge.yOffset, edge.flipped, outVertices, outAttributes);
			break;
		}
		case ArenaTypes::VoxelType::Chasm:
		{
			const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
			WriteChasmMeshGeometryBuffers(chasm.type, outVertices, outAttributes);
			break;
		}
		case ArenaTypes::VoxelType::Door:
			WriteDoorMeshGeometryBuffers(outVertices, outAttributes);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}

	void WriteIndexBuffers(ArenaTypes::VoxelType voxelType, BufferView<int32_t> outOpaqueIndices0,
		BufferView<int32_t> outOpaqueIndices1, BufferView<int32_t> outOpaqueIndices2,
		BufferView<int32_t> outAlphaTestedIndices)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
			WriteWallMeshIndexBuffers(outOpaqueIndices0, outOpaqueIndices1, outOpaqueIndices2);
			break;
		case ArenaTypes::VoxelType::Floor:
			WriteFloorMeshIndexBuffers(outOpaqueIndices0);
			break;
		case ArenaTypes::VoxelType::Ceiling:
			WriteCeilingMeshIndexBuffers(outOpaqueIndices0);
			break;
		case ArenaTypes::VoxelType::Raised:
			WriteRaisedMeshIndexBuffers(outAlphaTestedIndices, outOpaqueIndices0, outOpaqueIndices1);
			break;
		case ArenaTypes::VoxelType::Diagonal:
			WriteDiagonalMeshIndexBuffers(outOpaqueIndices0);
			break;
		case ArenaTypes::VoxelType::TransparentWall:
			WriteTransparentWallMeshIndexBuffers(outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Edge:
			WriteEdgeMeshIndexBuffers(outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Chasm:
			WriteChasmMeshIndexBuffers(outOpaqueIndices0, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Door:
			WriteDoorMeshIndexBuffers(outAlphaTestedIndices);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}
}

VoxelMeshDefinition::VoxelMeshDefinition()
{
	// Default to air voxel.
	this->uniqueVertexCount = 0;
	this->rendererVertexCount = 0;
	this->opaqueIndicesListCount = 0;
	this->alphaTestedIndicesListCount = 0;
	this->allowsBackFaces = false;
}

void VoxelMeshDefinition::initClassic(const VoxelDefinition &voxelDef)
{
	const ArenaTypes::VoxelType voxelType = voxelDef.type;

	this->uniqueVertexCount = GetUniqueVertexCount(voxelType);
	this->rendererVertexCount = GetRendererVertexCount(voxelType);
	this->opaqueIndicesListCount = GetOpaqueIndexBufferCount(voxelType);
	this->alphaTestedIndicesListCount = GetAlphaTestedIndexBufferCount(voxelType);
	this->allowsBackFaces = AllowsBackFacingGeometry(voxelType);

	if (voxelType != ArenaTypes::VoxelType::None)
	{
		this->rendererVertices.resize(GetRendererVertexComponentCount(voxelType));
		this->rendererAttributes.resize(GetRendererVertexAttributeCount(voxelType));
		
		for (int i = 0; i < this->opaqueIndicesListCount; i++)
		{
			std::vector<int32_t> &opaqueIndexBuffer = this->getOpaqueIndicesList(i);
			opaqueIndexBuffer.resize(GetOpaqueIndexCount(voxelType, i));
		}

		if (this->alphaTestedIndicesListCount > 0)
		{
			this->alphaTestedIndices.resize(GetAlphaTestedIndexCount(voxelType, 0));
		}

		BufferView<double> rendererVerticesView(this->rendererVertices.data(), static_cast<int>(this->rendererVertices.size()));
		BufferView<double> rendererAttributesView(this->rendererAttributes.data(), static_cast<int>(this->rendererAttributes.size()));
		WriteGeometryBuffers(voxelDef, rendererVerticesView, rendererAttributesView);

		BufferView<int32_t> opaqueIndices0View(this->opaqueIndices0.data(), static_cast<int>(this->opaqueIndices0.size()));
		BufferView<int32_t> opaqueIndices1View(this->opaqueIndices1.data(), static_cast<int>(this->opaqueIndices1.size()));
		BufferView<int32_t> opaqueIndices2View(this->opaqueIndices2.data(), static_cast<int>(this->opaqueIndices2.size()));
		BufferView<int32_t> alphaTestedIndicesView(this->alphaTestedIndices.data(), static_cast<int>(this->alphaTestedIndices.size()));
		WriteIndexBuffers(voxelType, opaqueIndices0View, opaqueIndices1View, opaqueIndices2View, alphaTestedIndicesView);
	}
}

bool VoxelMeshDefinition::isEmpty() const
{
	return this->uniqueVertexCount == 0;
}

std::vector<int32_t> &VoxelMeshDefinition::getOpaqueIndicesList(int index)
{
	const std::array<std::vector<int32_t>*, 3> ptrs = { &this->opaqueIndices0, &this->opaqueIndices1, &this->opaqueIndices2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

const std::vector<int32_t> &VoxelMeshDefinition::getOpaqueIndicesList(int index) const
{
	const std::array<const std::vector<int32_t>*, 3> ptrs = { &this->opaqueIndices0, &this->opaqueIndices1, &this->opaqueIndices2 };
	DebugAssertIndex(ptrs, index);
	return *ptrs[index];
}

void VoxelMeshDefinition::writeRendererGeometryBuffers(double ceilingScale, BufferView<double> outVertices,
	BufferView<double> outAttributes) const
{
	static_assert(COMPONENTS_PER_VERTEX == 3);
	DebugAssert(outVertices.getCount() >= this->rendererVertices.size());
	DebugAssert(outAttributes.getCount() >= this->rendererAttributes.size());

	for (int i = 0; i < this->rendererVertexCount; i++)
	{
		const int index = i * COMPONENTS_PER_VERTEX;
		const double srcX = this->rendererVertices[index];
		const double srcY = this->rendererVertices[index + 1];
		const double srcZ = this->rendererVertices[index + 2];
		const double dstX = srcX;
		const double dstY = srcY * ceilingScale;
		const double dstZ = srcZ;
		outVertices.set(index, dstX);
		outVertices.set(index + 1, dstY);
		outVertices.set(index + 2, dstZ);
	}

	std::copy(this->rendererAttributes.begin(), this->rendererAttributes.end(), outAttributes.get());
}

void VoxelMeshDefinition::writeRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices0,
	BufferView<int32_t> outOpaqueIndices1, BufferView<int32_t> outOpaqueIndices2,
	BufferView<int32_t> outAlphaTestedIndices) const
{
	if (!this->opaqueIndices0.empty())
	{
		std::copy(this->opaqueIndices0.begin(), this->opaqueIndices0.end(), outOpaqueIndices0.get());
	}

	if (!this->opaqueIndices1.empty())
	{
		std::copy(this->opaqueIndices1.begin(), this->opaqueIndices1.end(), outOpaqueIndices1.get());
	}

	if (!this->opaqueIndices2.empty())
	{
		std::copy(this->opaqueIndices2.begin(), this->opaqueIndices2.end(), outOpaqueIndices2.get());
	}

	if (!this->alphaTestedIndices.empty())
	{
		std::copy(this->alphaTestedIndices.begin(), this->alphaTestedIndices.end(), outAlphaTestedIndices.get());
	}
}
