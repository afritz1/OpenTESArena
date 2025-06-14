#ifndef ARENA_MESH_UTILS_H
#define ARENA_MESH_UTILS_H

#include <array>
#include <cstdint>

#include "MeshUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Collision/Physics.h"
#include "../Voxels/ArenaChasmUtils.h"
#include "../Voxels/VoxelUtils.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"

enum class VoxelFacing2D;
enum class VoxelFacing3D;

using ArenaChasmWallIndexBuffer = std::array<int32_t, MeshUtils::INDICES_PER_QUAD>; // Two triangles per buffer.

// Provides init values for physics shape and render mesh, comes from map generation.
struct ArenaShapeInitCache
{
	static constexpr int MAX_RENDERER_VERTICES = 24;
	static constexpr int MAX_RENDERER_INDICES = 36;

	// Simplified box shape values.
	double boxWidth;
	double boxHeight;
	double boxDepth;
	double boxYOffset;
	Radians boxYRotation;

	// Per-vertex data (duplicated to some extent, compared to the minimum-required).
	std::array<double, MAX_RENDERER_VERTICES * MeshUtils::POSITION_COMPONENTS_PER_VERTEX> positions;
	std::array<double, MAX_RENDERER_VERTICES * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX> normals;
	std::array<double, MAX_RENDERER_VERTICES * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX> texCoords;
	std::array<int32_t, MAX_RENDERER_INDICES> indices0, indices1, indices2;
	std::array<const decltype(indices0)*, 3> indicesPtrs;
	std::array<VoxelFacing3D, VoxelUtils::FACE_COUNT> facings0, facings1, facings2;
	std::array<const decltype(facings0)*, 3> facingsPtrs;

	BufferView<double> positionsView, normalsView, texCoordsView;
	BufferView<int32_t> indices0View, indices1View, indices2View;
	BufferView<VoxelFacing3D> facings0View, facings1View, facings2View;

	ArenaShapeInitCache();

	void initDefaultBoxValues();
	void initRaisedBoxValues(double height, double yOffset);
	void initChasmBoxValues(bool isDryChasm);
	void initDiagonalBoxValues(bool isRightDiag);
};

// The original game doesn't actually have meshes - this is just a convenient way to define things.
namespace ArenaMeshUtils
{
	static constexpr int CHASM_WALL_TOTAL_COUNT = 4;
	static constexpr int CHASM_WALL_NORTH = 0x1;
	static constexpr int CHASM_WALL_EAST = 0x2;
	static constexpr int CHASM_WALL_SOUTH = 0x4;
	static constexpr int CHASM_WALL_WEST = 0x8;
	static constexpr int CHASM_WALL_COMBINATION_COUNT = CHASM_WALL_NORTH | CHASM_WALL_EAST | CHASM_WALL_SOUTH | CHASM_WALL_WEST;

	// The "ideal" vertices per voxel (no duplication).
	constexpr int GetUniqueVertexCount(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
			return 0;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Chasm:
		case ArenaVoxelType::Door:
			return 8;
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Edge:
			return 4;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetUniqueFaceCount(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
			return 0;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Raised:
			return 6;
		case ArenaVoxelType::Chasm:
			return 5;
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Door:
			return 4;
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
			return 1;
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Edge:
			return 2;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// The actual vertices per voxel used by the renderer due to how vertex attributes work.
	constexpr int GetRendererVertexCount(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
			return 0;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Raised:
			return 24;
		case ArenaVoxelType::TransparentWall:
			return 16;
		case ArenaVoxelType::Chasm:
			return 20;
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Door:
			return 4;
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Edge:
			return 8;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetUniqueVertexPositionComponentCount(ArenaVoxelType voxelType)
	{
		return GetUniqueVertexCount(voxelType) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetUniqueFaceNormalComponentCount(ArenaVoxelType voxelType)
	{
		return GetUniqueFaceCount(voxelType) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexPositionComponentCount(ArenaVoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexNormalComponentCount(ArenaVoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetRendererVertexTexCoordComponentCount(ArenaVoxelType voxelType)
	{
		return GetRendererVertexCount(voxelType) * MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;
	}

	constexpr int GetOpaqueIndexBufferCount(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::Wall:
			return 3;
		case ArenaVoxelType::Raised:
			return 2;
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Chasm:
			return 1;
		case ArenaVoxelType::None:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Door:
			return 0;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetOpaqueIndexCount(ArenaVoxelType voxelType, int bufferIndex)
	{
		int triangleCount = -1;

		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Door:
		case ArenaVoxelType::Edge:
			// @todo: should this static_assert false instead?
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
		case ArenaVoxelType::Wall:
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
		case ArenaVoxelType::Raised:
			if ((bufferIndex == 0) || (bufferIndex == 1))
			{
				triangleCount = 4;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaVoxelType::Chasm:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
			if (bufferIndex == 0)
			{
				triangleCount = 2;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaVoxelType::Diagonal:
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

	constexpr int GetAlphaTestedIndexBufferCount(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Chasm:
			return 0;
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Door:
			return 1;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetAlphaTestedIndexCount(ArenaVoxelType voxelType, int bufferIndex)
	{
		int triangleCount = -1;

		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Chasm:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
		case ArenaVoxelType::Raised:
			if (bufferIndex == 0)
			{
				triangleCount = 12;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaVoxelType::TransparentWall:
			if (bufferIndex == 0)
			{
				triangleCount = 8;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaVoxelType::Edge:
			if (bufferIndex == 0)
			{
				triangleCount = 4;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaVoxelType::Door:
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

	constexpr int GetIndexBufferCount(ArenaVoxelType voxelType)
	{
		return GetOpaqueIndexBufferCount(voxelType) + GetAlphaTestedIndexBufferCount(voxelType);
	}

	constexpr int GetIndexBufferIndexCount(ArenaVoxelType voxelType, int indexBufferIndex)
	{
		constexpr std::pair<ArenaVoxelType, int> IndexBuffer0FaceCounts[] =
		{
			{ ArenaVoxelType::None, 0 },
			{ ArenaVoxelType::Wall, 4 },
			{ ArenaVoxelType::Floor, 1 },
			{ ArenaVoxelType::Ceiling, 1 },
			{ ArenaVoxelType::Raised, 4 },
			{ ArenaVoxelType::Diagonal, 2 },
			{ ArenaVoxelType::TransparentWall, 4 },
			{ ArenaVoxelType::Edge, 2 },
			{ ArenaVoxelType::Chasm, 1 },
			{ ArenaVoxelType::Door, 4 }
		};

		constexpr std::pair<ArenaVoxelType, int> IndexBuffer1FaceCounts[] =
		{
			{ ArenaVoxelType::None, 0 },
			{ ArenaVoxelType::Wall, 1 },
			{ ArenaVoxelType::Floor, 0 },
			{ ArenaVoxelType::Ceiling, 0 },
			{ ArenaVoxelType::Raised, 1 },
			{ ArenaVoxelType::Diagonal, 0 },
			{ ArenaVoxelType::TransparentWall, 0 },
			{ ArenaVoxelType::Edge, 0 },
			{ ArenaVoxelType::Chasm, 0 },
			{ ArenaVoxelType::Door, 0 }
		};

		constexpr std::pair<ArenaVoxelType, int> IndexBuffer2FaceCounts[] =
		{
			{ ArenaVoxelType::None, 0 },
			{ ArenaVoxelType::Wall, 1 },
			{ ArenaVoxelType::Floor, 0 },
			{ ArenaVoxelType::Ceiling, 0 },
			{ ArenaVoxelType::Raised, 1 },
			{ ArenaVoxelType::Diagonal, 0 },
			{ ArenaVoxelType::TransparentWall, 0 },
			{ ArenaVoxelType::Edge, 0 },
			{ ArenaVoxelType::Chasm, 0 },
			{ ArenaVoxelType::Door, 0 }
		};

		const std::pair<ArenaVoxelType, int> *indexBufferFaceCounts = nullptr;
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
			const std::pair<ArenaVoxelType, int> &pair = indexBufferFaceCounts[i];
			if (pair.first == voxelType)
			{
				faceCount = pair.second;
				break;
			}
		}

		return faceCount * MeshUtils::INDICES_PER_QUAD;
	}

	// Similar to index buffer count but only for voxels whose mesh can cover one or more entire voxel faces.
	constexpr int GetFacingBufferCount(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::Wall:
			return 3;
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
			return 1;
		case ArenaVoxelType::None:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Chasm:
		case ArenaVoxelType::Door:
			return 0;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetFacingBufferFaceCount(ArenaVoxelType voxelType, int facingBufferIndex)
	{
		constexpr std::pair<ArenaVoxelType, int> FacingBuffer0FaceCounts[] =
		{
			{ ArenaVoxelType::None, 0 },
			{ ArenaVoxelType::Wall, 4 },
			{ ArenaVoxelType::Floor, 1 },
			{ ArenaVoxelType::Ceiling, 1 },
			{ ArenaVoxelType::Raised, 0 },
			{ ArenaVoxelType::Diagonal, 0 },
			{ ArenaVoxelType::TransparentWall, 0 },
			{ ArenaVoxelType::Edge, 0 },
			{ ArenaVoxelType::Chasm, 0 },
			{ ArenaVoxelType::Door, 0 }
		};

		constexpr std::pair<ArenaVoxelType, int> FacingBuffer1FaceCounts[] =
		{
			{ ArenaVoxelType::None, 0 },
			{ ArenaVoxelType::Wall, 1 },
			{ ArenaVoxelType::Floor, 0 },
			{ ArenaVoxelType::Ceiling, 0 },
			{ ArenaVoxelType::Raised, 0 },
			{ ArenaVoxelType::Diagonal, 0 },
			{ ArenaVoxelType::TransparentWall, 0 },
			{ ArenaVoxelType::Edge, 0 },
			{ ArenaVoxelType::Chasm, 0 },
			{ ArenaVoxelType::Door, 0 }
		};

		constexpr std::pair<ArenaVoxelType, int> FacingBuffer2FaceCounts[] =
		{
			{ ArenaVoxelType::None, 0 },
			{ ArenaVoxelType::Wall, 1 },
			{ ArenaVoxelType::Floor, 0 },
			{ ArenaVoxelType::Ceiling, 0 },
			{ ArenaVoxelType::Raised, 0 },
			{ ArenaVoxelType::Diagonal, 0 },
			{ ArenaVoxelType::TransparentWall, 0 },
			{ ArenaVoxelType::Edge, 0 },
			{ ArenaVoxelType::Chasm, 0 },
			{ ArenaVoxelType::Door, 0 }
		};

		const std::pair<ArenaVoxelType, int> *facingBufferFaceCounts = nullptr;
		switch (facingBufferIndex)
		{
		case 0:
			facingBufferFaceCounts = FacingBuffer0FaceCounts;
			break;
		case 1:
			facingBufferFaceCounts = FacingBuffer1FaceCounts;
			break;
		case 2:
			facingBufferFaceCounts = FacingBuffer2FaceCounts;
			break;
		default:
			DebugNotImplemented();
			break;
		}

		constexpr int voxelTypeCount = static_cast<int>(std::size(FacingBuffer0FaceCounts));

		int faceCount = 0;
		for (int i = 0; i < voxelTypeCount; i++)
		{
			const std::pair<ArenaVoxelType, int> &pair = facingBufferFaceCounts[i];
			if (pair.first == voxelType)
			{
				faceCount = pair.second;
				break;
			}
		}

		return faceCount;
	}

	constexpr int GetChasmWallIndex(bool north, bool east, bool south, bool west)
	{
		const int index = (north ? CHASM_WALL_NORTH : 0) | (east ? CHASM_WALL_EAST : 0) | (south ? CHASM_WALL_SOUTH : 0) | (west ? CHASM_WALL_WEST : 0);
		return index - 1;
	}

	constexpr bool AllowsBackFacingGeometry(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Door:
			return false;
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Chasm:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr bool AllowsAdjacentDoorFaces(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Chasm:
		case ArenaVoxelType::Door:
			return false;
		case ArenaVoxelType::None:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::Edge:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr bool AllowsInternalFaceRemoval(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Door:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Chasm:
			return false;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr bool AllowsAdjacentFaceCombining(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::TransparentWall: // @todo eventually allow hedges to combine
		case ArenaVoxelType::Door:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Chasm: // @todo eventually allow chasm floors to combine (it covers O(n^2) of the game world, not just chunk edges)
			return false;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr bool EnablesNeighborVoxelGeometry(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Chasm:
			return false;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Door:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr bool HasContextSensitiveGeometry(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Door:
			return false;
		case ArenaVoxelType::Chasm:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr bool IsElevatedPlatform(ArenaVoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaVoxelType::None:
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Diagonal:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Chasm:
		case ArenaVoxelType::Door:
			return false;
		case ArenaVoxelType::Raised:
			return true;
		default:
			DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
		}
	}

	// Mesh writing functions. All of these are in unscaled model space.
	// Unique geometry positions are lexicographically ordered for separation of responsibility from how they're used,
	// and renderer positions are ordered in the way they're consumed when being converted to triangles.
	void writeWallUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void writeWallRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeWallRendererIndexBuffers(BufferView<int32_t> outOpaqueSideIndices, BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices);
	void writeWallFacingBuffers(BufferView<VoxelFacing3D> outSideFacings, BufferView<VoxelFacing3D> outBottomFacings, BufferView<VoxelFacing3D> outTopFacings);

	void writeFloorUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void writeFloorRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void writeFloorFacingBuffers(BufferView<VoxelFacing3D> outFacings);
	
	void writeCeilingUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void writeCeilingRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeCeilingRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void writeCeilingFacingBuffers(BufferView<VoxelFacing3D> outFacings);
	
	void writeRaisedUniqueGeometryBuffers(double yOffset, double ySize, BufferView<double> outPositions, BufferView<double> outNormals);
	void writeRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeRaisedRendererIndexBuffers(BufferView<int32_t> outSideIndices, BufferView<int32_t> outBottomIndices, BufferView<int32_t> outTopIndices);
	
	void writeDiagonalUniqueGeometryBuffers(bool type1, BufferView<double> outPositions, BufferView<double> outNormals);
	void writeDiagonalRendererGeometryBuffers(bool type1, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeDiagonalRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	
	void writeTransparentWallUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void writeTransparentWallRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeTransparentWallRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	
	void writeEdgeUniqueGeometryBuffers(VoxelFacing2D facing, double yOffset, BufferView<double> outPositions, BufferView<double> outNormals);
	void writeEdgeRendererGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeEdgeRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	
	void writeChasmUniqueGeometryBuffers(ArenaChasmType chasmType, BufferView<double> outPositions, BufferView<double> outNormals);
	void writeChasmRendererGeometryBuffers(ArenaChasmType chasmType, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeChasmFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices); // Chasm walls are separate because they're conditionally enabled.
	void writeChasmWallRendererIndexBuffers(ArenaChasmWallIndexBuffer *outNorthIndices, ArenaChasmWallIndexBuffer *outEastIndices, ArenaChasmWallIndexBuffer *outSouthIndices, ArenaChasmWallIndexBuffer *outWestIndices);
	
	void writeDoorUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void writeDoorRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void writeDoorRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
}

#endif
