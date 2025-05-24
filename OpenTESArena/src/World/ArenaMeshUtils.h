#ifndef ARENA_MESH_UTILS_H
#define ARENA_MESH_UTILS_H

#include <array>
#include <cstdint>

#include "MeshUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Collision/Physics.h"
#include "../Voxels/ArenaChasmUtils.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"

enum class VoxelFacing2D;

// The original game doesn't actually have meshes - this is just a convenient way to define things.
namespace ArenaMeshUtils
{
	static constexpr int MAX_RENDERER_VERTICES = 24;
	static constexpr int MAX_RENDERER_INDICES = 36;

	static constexpr int CHASM_WALL_NORTH = 0x1;
	static constexpr int CHASM_WALL_EAST = 0x2;
	static constexpr int CHASM_WALL_SOUTH = 0x4;
	static constexpr int CHASM_WALL_WEST = 0x8;
	static constexpr int CHASM_WALL_COMBINATION_COUNT = CHASM_WALL_NORTH | CHASM_WALL_EAST | CHASM_WALL_SOUTH | CHASM_WALL_WEST;

	using ChasmWallIndexBuffer = std::array<int32_t, 6>; // Two triangles per buffer.

	// Provides init values for physics shape and render mesh, comes from map generation.
	struct ShapeInitCache
	{
		// Simplified box shape values.
		double boxWidth;
		double boxHeight;
		double boxDepth;
		double boxYOffset;
		Radians boxYRotation;

		// Per-vertex data (duplicated to some extent, compared to the minimum-required).
		std::array<double, MAX_RENDERER_VERTICES * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> vertices;
		std::array<double, MAX_RENDERER_VERTICES * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals;
		std::array<double, MAX_RENDERER_VERTICES * MeshUtils::TEX_COORDS_PER_VERTEX> texCoords;
		std::array<int32_t, MAX_RENDERER_INDICES> indices0, indices1, indices2;
		std::array<const decltype(indices0)*, 3> indicesPtrs;

		BufferView<double> verticesView, normalsView, texCoordsView;
		BufferView<int32_t> indices0View, indices1View, indices2View;

		ShapeInitCache()
		{
			this->boxWidth = 0.0;
			this->boxHeight = 0.0;
			this->boxDepth = 0.0;
			this->boxYOffset = 0.0;
			this->boxYRotation = 0.0;

			this->vertices.fill(0.0);
			this->normals.fill(0.0);
			this->texCoords.fill(0.0);
			this->indices0.fill(-1);
			this->indices1.fill(-1);
			this->indices2.fill(-1);
			this->indicesPtrs = { &this->indices0, &this->indices1, &this->indices2 };

			this->verticesView.init(this->vertices);
			this->normalsView.init(this->normals);
			this->texCoordsView.init(this->texCoords);
			this->indices0View.init(this->indices0);
			this->indices1View.init(this->indices1);
			this->indices2View.init(this->indices2);
		}

		void initDefaultBoxValues()
		{
			this->boxWidth = 1.0;
			this->boxHeight = 1.0;
			this->boxDepth = 1.0;
			this->boxYOffset = 0.0;
			this->boxYRotation = 0.0;
		}

		void initRaisedBoxValues(double height, double yOffset)
		{
			this->boxWidth = 1.0;
			this->boxHeight = height;
			this->boxDepth = 1.0;
			this->boxYOffset = yOffset;
			this->boxYRotation = 0.0;
		}
		
		void initChasmBoxValues(bool isDryChasm)
		{
			// Offset below the chasm floor so the collider isn't infinitely thin.
			// @todo: this doesn't seem right for wet chasms
			this->boxWidth = 1.0;
			this->boxHeight = 0.10;
			if (!isDryChasm)
			{
				this->boxHeight += 1.0 - ArenaChasmUtils::DEFAULT_HEIGHT;
			}

			this->boxDepth = 1.0;
			this->boxYOffset = -0.10;
			this->boxYRotation = 0.0;
		}

		void initDiagonalBoxValues(bool isRightDiag)
		{
			constexpr Radians diagonalAngle = Constants::Pi / 4.0;
			constexpr double diagonalThickness = 0.050; // Arbitrary thin wall thickness
			static_assert(diagonalThickness > (Physics::BoxConvexRadius * 2.0));

			this->boxWidth = Constants::Sqrt2 - diagonalThickness; // Fit the edges of the voxel exactly
			this->boxHeight = 1.0;
			this->boxDepth = diagonalThickness;
			this->boxYOffset = 0.0;
			this->boxYRotation = isRightDiag ? -diagonalAngle : diagonalAngle;
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

	constexpr int GetIndexBufferCount(ArenaTypes::VoxelType voxelType)
	{
		return GetOpaqueIndexBufferCount(voxelType) + GetAlphaTestedIndexBufferCount(voxelType);
	}

	constexpr int GetIndexBufferIndexCount(ArenaTypes::VoxelType voxelType, int indexBufferIndex)
	{
		constexpr std::pair<ArenaTypes::VoxelType, int> IndexBuffer0FaceCounts[] =
		{
			{ ArenaTypes::VoxelType::None, 0 },
			{ ArenaTypes::VoxelType::Wall, 4 },
			{ ArenaTypes::VoxelType::Floor, 1 },
			{ ArenaTypes::VoxelType::Ceiling, 1 },
			{ ArenaTypes::VoxelType::Raised, 4 },
			{ ArenaTypes::VoxelType::Diagonal, 2 },
			{ ArenaTypes::VoxelType::TransparentWall, 4 },
			{ ArenaTypes::VoxelType::Edge, 2 },
			{ ArenaTypes::VoxelType::Chasm, 1 },
			{ ArenaTypes::VoxelType::Door, 4 }
		};

		constexpr std::pair<ArenaTypes::VoxelType, int> IndexBuffer1FaceCounts[] =
		{
			{ ArenaTypes::VoxelType::None, 0 },
			{ ArenaTypes::VoxelType::Wall, 1 },
			{ ArenaTypes::VoxelType::Floor, 0 },
			{ ArenaTypes::VoxelType::Ceiling, 0 },
			{ ArenaTypes::VoxelType::Raised, 1 },
			{ ArenaTypes::VoxelType::Diagonal, 0 },
			{ ArenaTypes::VoxelType::TransparentWall, 0 },
			{ ArenaTypes::VoxelType::Edge, 0 },
			{ ArenaTypes::VoxelType::Chasm, 0 },
			{ ArenaTypes::VoxelType::Door, 0 }
		};

		constexpr std::pair<ArenaTypes::VoxelType, int> IndexBuffer2FaceCounts[] =
		{
			{ ArenaTypes::VoxelType::None, 0 },
			{ ArenaTypes::VoxelType::Wall, 1 },
			{ ArenaTypes::VoxelType::Floor, 0 },
			{ ArenaTypes::VoxelType::Ceiling, 0 },
			{ ArenaTypes::VoxelType::Raised, 1 },
			{ ArenaTypes::VoxelType::Diagonal, 0 },
			{ ArenaTypes::VoxelType::TransparentWall, 0 },
			{ ArenaTypes::VoxelType::Edge, 0 },
			{ ArenaTypes::VoxelType::Chasm, 0 },
			{ ArenaTypes::VoxelType::Door, 0 }
		};

		const std::pair<ArenaTypes::VoxelType, int> *indexBufferFaceCounts = nullptr;
		switch (indexBufferIndex)
		{
		case 0:
			indexBufferFaceCounts = IndexBuffer0FaceCounts;
			break;
		case 1:
			indexBufferFaceCounts = IndexBuffer1FaceCounts;
			break;
		case 2:
			indexBufferFaceCounts = IndexBuffer2FaceCounts;
			break;
		default:
			DebugNotImplemented();
			break;
		}

		constexpr int voxelTypeCount = static_cast<int>(std::size(IndexBuffer0FaceCounts));

		int faceCount = 0;
		for (int i = 0; i < voxelTypeCount; i++)
		{
			const std::pair<ArenaTypes::VoxelType, int> &pair = indexBufferFaceCounts[i];
			if (pair.first == voxelType)
			{
				faceCount = pair.second;
				break;
			}
		}

		return faceCount * MeshUtils::INDICES_PER_FACE;
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

	constexpr bool AllowsAdjacentDoorFaces(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Door:
			return false;
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::Edge:
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

	constexpr bool IsElevatedPlatform(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Edge:
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Door:
			return false;
		case ArenaTypes::VoxelType::Raised:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Mesh writing functions. All of these are in unscaled model space.
	void WriteWallUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteWallRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteWallRendererIndexBuffers(BufferView<int32_t> outOpaqueSideIndices, BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices);
	void WriteFloorUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteFloorRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteCeilingUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteCeilingRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteCeilingRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteRaisedUniqueGeometryBuffers(double yOffset, double ySize, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteRaisedRendererIndexBuffers(BufferView<int32_t> outSideIndices, BufferView<int32_t> outBottomIndices, BufferView<int32_t> outTopIndices);
	void WriteDiagonalUniqueGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteDiagonalRendererGeometryBuffers(bool type1, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteDiagonalRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteTransparentWallUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteTransparentWallRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteTransparentWallRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteEdgeUniqueGeometryBuffers(VoxelFacing2D facing, double yOffset, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteEdgeRendererGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteEdgeRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteChasmUniqueGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteChasmRendererGeometryBuffers(ArenaTypes::ChasmType chasmType, BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteChasmFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices); // Chasm walls are separate because they're conditionally enabled.
	void WriteChasmWallRendererIndexBuffers(ChasmWallIndexBuffer *outNorthIndices, ChasmWallIndexBuffer *outEastIndices, ChasmWallIndexBuffer *outSouthIndices, ChasmWallIndexBuffer *outWestIndices);
	void WriteDoorUniqueGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals);
	void WriteDoorRendererGeometryBuffers(BufferView<double> outVertices, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteDoorRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
}

#endif
