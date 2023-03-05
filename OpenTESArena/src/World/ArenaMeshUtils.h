#ifndef ARENA_MESH_UTILS_H
#define ARENA_MESH_UTILS_H

#include <array>
#include <cstdint>

#include "MeshUtils.h"
#include "../Assets/ArenaTypes.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"

// Note that the original game doesn't actually have meshes - this is just a convenient way to define things.

enum class VoxelFacing2D;

namespace ArenaMeshUtils
{
	static constexpr int MAX_RENDERER_VERTICES = 24;
	static constexpr int MAX_RENDERER_INDICES = 36;

	static constexpr int MAX_COLLISION_VERTICES = 8;
	static constexpr int MAX_COLLISION_FACES = 6;
	static constexpr int MAX_COLLISION_INDICES = 72;

	static constexpr int CHASM_WALL_NORTH = 0x1;
	static constexpr int CHASM_WALL_EAST = 0x2;
	static constexpr int CHASM_WALL_SOUTH = 0x4;
	static constexpr int CHASM_WALL_WEST = 0x8;
	static constexpr int CHASM_WALL_COMBINATION_COUNT = CHASM_WALL_NORTH | CHASM_WALL_EAST | CHASM_WALL_SOUTH | CHASM_WALL_WEST;

	using ChasmWallIndexBuffer = std::array<int32_t, 6>; // Two triangles per buffer.

	struct RenderMeshInitCache
	{
		// Per-vertex data (duplicated to some extent, compared to the minimum-required).
		std::array<double, MAX_RENDERER_VERTICES * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices;
		std::array<double, MAX_RENDERER_VERTICES * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals;
		std::array<double, MAX_RENDERER_VERTICES * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords;
		std::array<int32_t, MAX_RENDERER_INDICES> opaqueIndices0, opaqueIndices1, opaqueIndices2;
		std::array<int32_t, MAX_RENDERER_INDICES> alphaTestedIndices0;
		std::array<const decltype(opaqueIndices0)*, 3> opaqueIndicesPtrs;

		BufferView<double> verticesView, normalsView, texCoordsView;
		BufferView<int32_t> opaqueIndices0View, opaqueIndices1View, opaqueIndices2View, alphaTestedIndices0View;

		RenderMeshInitCache()
		{
			this->vertices.fill(0.0);
			this->normals.fill(0.0);
			this->texCoords.fill(0.0);
			this->opaqueIndices0.fill(-1);
			this->opaqueIndices1.fill(-1);
			this->opaqueIndices2.fill(-1);
			this->alphaTestedIndices0.fill(-1);
			this->opaqueIndicesPtrs = { &this->opaqueIndices0, &this->opaqueIndices1, &this->opaqueIndices2 };

			this->verticesView.init(this->vertices);
			this->normalsView.init(this->normals);
			this->texCoordsView.init(this->texCoords);
			this->opaqueIndices0View.init(this->opaqueIndices0);
			this->opaqueIndices1View.init(this->opaqueIndices1);
			this->opaqueIndices2View.init(this->opaqueIndices2);
			this->alphaTestedIndices0View.init(this->alphaTestedIndices0);
		}
	};

	struct CollisionMeshInitCache
	{
		// Minimum-required geometry (not per vertex, unlike renderer geometry) with indices in a tuple format
		// (two indices per vertex: first index for vertex XYZ, second for normal XYZ).
		std::array<double, MAX_COLLISION_VERTICES * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices;
		std::array<double, MAX_COLLISION_FACES * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals;
		std::array<int32_t, MAX_COLLISION_INDICES> indices;

		BufferView<double> verticesView, normalsView;
		BufferView<int32_t> indicesView;

		CollisionMeshInitCache()
		{
			this->vertices.fill(0.0);
			this->normals.fill(0.0);
			this->indices.fill(-1);

			this->verticesView = BufferView<double>(this->vertices);
			this->normalsView = BufferView<double>(this->normals);
			this->indicesView = BufferView<int32_t>(this->indices);
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

	constexpr int GetUniqueFaceCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
			return 0;
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Raised:
			return 6;
		case ArenaTypes::VoxelType::Chasm:
			return 5;
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
			return 4;
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
			return 1;
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Edge:
			return 2;
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
			return 16;
		case ArenaTypes::VoxelType::Chasm:
			return 20;
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Door:
			return 4;
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Edge:
			return 8;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetUniqueVertexPositionComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetUniqueVertexCount(voxelType) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetUniqueFaceNormalComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetUniqueFaceCount(voxelType) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexPositionComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexNormalComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexTexCoordComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * MeshUtils::TEX_COORDS_PER_VERTEX;
	}

	constexpr int GetCollisionVertexPositionComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetUniqueVertexPositionComponentCount(voxelType);
	}

	constexpr int GetCollisionFaceNormalComponentCount(ArenaTypes::VoxelType voxelType)
	{
		return GetUniqueFaceNormalComponentCount(voxelType);
	}

	constexpr int GetCollisionIndexCount(ArenaTypes::VoxelType voxelType)
	{
		int indexCount = -1;
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
			indexCount = 0;
			break;
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Raised:
			indexCount = 6 * MeshUtils::INDICES_PER_FACE;
			break;
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Chasm:
			indexCount = 1 * MeshUtils::INDICES_PER_FACE;
			break;
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::Edge:
			indexCount = 2 * MeshUtils::INDICES_PER_FACE;
			break;
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
			indexCount = 4 * MeshUtils::INDICES_PER_FACE;
			break;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}

		// This is doubled due to the index tuple design (first index: position, second index: normal).
		return indexCount * 2;
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
			if (bufferIndex == 0)
			{
				triangleCount = 2;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaTypes::VoxelType::Diagonal:
			if (bufferIndex == 0)
			{
				triangleCount = 4;
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
				triangleCount = 4;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaTypes::VoxelType::Door:
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

	constexpr int GetChasmWallIndex(bool north, bool east, bool south, bool west)
	{
		const int index = (north ? CHASM_WALL_NORTH : 0) | (east ? CHASM_WALL_EAST : 0) | (south ? CHASM_WALL_SOUTH : 0) | (west ? CHASM_WALL_WEST : 0);
		return index - 1;
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

	constexpr bool HasContextSensitiveGeometry(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Door:
			return false;
		case ArenaTypes::VoxelType::Chasm:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Mesh writing functions. All of these are in unscaled model space.
	void WriteWallUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteWallRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteWallRendererIndexBuffers(BufferView<int32_t> outOpaqueSideIndices, BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices);
	void WriteWallCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteWallCollisionIndexBuffers(BufferView<int32_t> outIndices);
	void WriteFloorUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteFloorRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteFloorCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteFloorCollisionIndexBuffers(BufferView<int32_t> outIndices);
	void WriteCeilingUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteCeilingRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteCeilingRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteCeilingCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteCeilingCollisionIndexBuffers(BufferView<int32_t> outIndices);
	void WriteRaisedUniqueGeometryBuffers(double yOffset, double ySize, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteRaisedRendererIndexBuffers(BufferView<int32_t> outAlphaTestedSideIndices, BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices);
	void WriteRaisedCollisionGeometryBuffers(double yOffset, double ySize, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteRaisedCollisionIndexBuffers(BufferView<int32_t> outIndices);
	void WriteDiagonalUniqueGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteDiagonalRendererGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteDiagonalRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteDiagonalCollisionGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteDiagonalCollisionIndexBuffers(BufferView<int32_t> outIndices);
	void WriteTransparentWallUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteTransparentWallRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteTransparentWallRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteTransparentWallCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteTransparentWallCollisionIndexBuffers(BufferView<int32_t> outIndices);
	void WriteEdgeUniqueGeometryBuffers(VoxelFacing2D facing, double yOffset, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteEdgeRendererGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteEdgeRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteEdgeCollisionGeometryBuffers(VoxelFacing2D facing, double yOffset, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteEdgeCollisionIndexBuffers(BufferView<int32_t> outIndices);
	void WriteChasmUniqueGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteChasmRendererGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteChasmFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices); // Chasm walls are separate because they're conditionally enabled.
	void WriteChasmWallRendererIndexBuffers(ChasmWallIndexBuffer *outNorthIndices, ChasmWallIndexBuffer *outEastIndices, ChasmWallIndexBuffer *outSouthIndices, ChasmWallIndexBuffer *outWestIndices);
	void WriteChasmCollisionGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteChasmFloorCollisionIndexBuffers(BufferView<int32_t> outIndices); // Chasm walls are separate because they're conditionally enabled.
	void WriteChasmWallCollisionIndexBuffers(ChasmWallIndexBuffer *outNorthIndices, ChasmWallIndexBuffer *outEastIndices, ChasmWallIndexBuffer *outSouthIndices, ChasmWallIndexBuffer *outWestIndices);
	void WriteDoorUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteDoorRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteDoorRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteDoorCollisionGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteDoorCollisionIndexBuffers(BufferView<int32_t> outIndices);
}

#endif
