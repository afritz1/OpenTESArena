#ifndef ARENA_MESH_UTILS_H
#define ARENA_MESH_UTILS_H

#include <array>

#include "MeshUtils.h"
#include "../Assets/ArenaTypes.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"

// Note that the original game doesn't actually have meshes - this is just a convenient way to define things.

enum class VoxelFacing2D;

namespace ArenaMeshUtils
{
	static constexpr int MAX_VERTICES = 24;
	static constexpr int MAX_INDICES = 36;

	struct InitCache
	{
		std::array<double, MAX_VERTICES * MeshUtils::COMPONENTS_PER_VERTEX> vertices;
		std::array<double, MAX_VERTICES * MeshUtils::ATTRIBUTES_PER_VERTEX> attributes;
		std::array<int32_t, MAX_INDICES> opaqueIndices0, opaqueIndices1, opaqueIndices2;
		std::array<int32_t, MAX_INDICES> alphaTestedIndices0;
		std::array<const decltype(opaqueIndices0)*, 3> opaqueIndicesPtrs;

		BufferView<double> verticesView, attributesView;
		BufferView<int32_t> opaqueIndices0View, opaqueIndices1View, opaqueIndices2View, alphaTestedIndices0View;

		InitCache()
		{
			this->vertices.fill(0.0);
			this->attributes.fill(0.0);
			this->opaqueIndices0.fill(-1);
			this->opaqueIndices1.fill(-1);
			this->opaqueIndices2.fill(-1);
			this->alphaTestedIndices0.fill(-1);
			this->opaqueIndicesPtrs = { &this->opaqueIndices0, &this->opaqueIndices1, &this->opaqueIndices2 };

			this->verticesView = BufferView<double>(this->vertices.data(), static_cast<int>(this->vertices.size()));
			this->attributesView = BufferView<double>(this->attributes.data(), static_cast<int>(this->attributes.size()));
			this->opaqueIndices0View = BufferView<int32_t>(this->opaqueIndices0.data(), static_cast<int>(this->opaqueIndices0.size()));
			this->opaqueIndices1View = BufferView<int32_t>(this->opaqueIndices1.data(), static_cast<int>(this->opaqueIndices1.size()));
			this->opaqueIndices2View = BufferView<int32_t>(this->opaqueIndices2.data(), static_cast<int>(this->opaqueIndices2.size()));
			this->alphaTestedIndices0View = BufferView<int32_t>(this->alphaTestedIndices0.data(), static_cast<int>(this->alphaTestedIndices0.size()));
		}
	};

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
		return GetRendererVertexCount(voxelType) * MeshUtils::COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexAttributeCount(ArenaTypes::VoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * MeshUtils::ATTRIBUTES_PER_VERTEX;
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

		return triangleCount * MeshUtils::INDICES_PER_TRIANGLE;
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

		return triangleCount * MeshUtils::INDICES_PER_TRIANGLE;
	}

	constexpr bool AllowsBackFacingGeometry(ArenaTypes::VoxelType voxelType)
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

	constexpr bool EnablesNeighborVoxelGeometry(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Chasm:
			return false;
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Door:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Mesh writing functions. All of these are in unscaled model space.
	void WriteWallMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes);
	void WriteWallMeshIndexBuffers(BufferView<int32_t> outOpaqueSideIndices, BufferView<int32_t> outOpaqueBottomIndices,
		BufferView<int32_t> outOpaqueTopIndices);
	void WriteFloorMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes);
	void WriteFloorMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteCeilingMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes);
	void WriteCeilingMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteRaisedMeshGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop,
		BufferView<double> outVertices, BufferView<double> outAttributes);
	void WriteRaisedMeshIndexBuffers(BufferView<int32_t> outAlphaTestedSideIndices,
		BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices);
	void WriteDiagonalMeshGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outAttributes);
	void WriteDiagonalMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteTransparentWallMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes);
	void WriteTransparentWallMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteEdgeMeshGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped, BufferView<double> outVertices,
		BufferView<double> outAttributes);
	void WriteEdgeMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteChasmMeshGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices,
		BufferView<double> outAttributes);
	void WriteChasmMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices, BufferView<int32_t> outAlphaTestedIndices);
	void WriteDoorMeshGeometryBuffers(BufferView<double> outVertices, BufferView<double> outAttributes);
	void WriteDoorMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
}

#endif
