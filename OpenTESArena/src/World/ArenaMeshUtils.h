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

		ShapeInitCache()
		{
			this->boxWidth = 0.0;
			this->boxHeight = 0.0;
			this->boxDepth = 0.0;
			this->boxYOffset = 0.0;
			this->boxYRotation = 0.0;

			this->positions.fill(0.0);
			this->normals.fill(0.0);
			this->texCoords.fill(0.0);

			this->indices0.fill(-1);
			this->indices1.fill(-1);
			this->indices2.fill(-1);
			this->indicesPtrs = { &this->indices0, &this->indices1, &this->indices2 };

			this->facings0.fill(static_cast<VoxelFacing3D>(-1));
			this->facings1.fill(static_cast<VoxelFacing3D>(-1));
			this->facings2.fill(static_cast<VoxelFacing3D>(-1));
			this->facingsPtrs = { &this->facings0, &this->facings1, &this->facings2 };

			this->positionsView.init(this->positions);
			this->normalsView.init(this->normals);
			this->texCoordsView.init(this->texCoords);

			this->indices0View.init(this->indices0);
			this->indices1View.init(this->indices1);
			this->indices2View.init(this->indices2);

			this->facings0View.init(this->facings0);
			this->facings1View.init(this->facings1);
			this->facings2View.init(this->facings2);
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
	void WriteWallUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteWallRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteWallRendererIndexBuffers(BufferView<int32_t> outOpaqueSideIndices, BufferView<int32_t> outOpaqueBottomIndices, BufferView<int32_t> outOpaqueTopIndices);
	void WriteFloorUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteFloorRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteCeilingUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteCeilingRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteCeilingRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteRaisedUniqueGeometryBuffers(double yOffset, double ySize, BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteRaisedRendererGeometryBuffers(double yOffset, double ySize, double vBottom, double vTop, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteRaisedRendererIndexBuffers(BufferView<int32_t> outSideIndices, BufferView<int32_t> outBottomIndices, BufferView<int32_t> outTopIndices);
	void WriteDiagonalUniqueGeometryBuffers(bool type1, BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteDiagonalRendererGeometryBuffers(bool type1, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteDiagonalRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices);
	void WriteTransparentWallUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteTransparentWallRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteTransparentWallRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteEdgeUniqueGeometryBuffers(VoxelFacing2D facing, double yOffset, BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteEdgeRendererGeometryBuffers(VoxelFacing2D facing, double yOffset, bool flipped, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteEdgeRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
	void WriteChasmUniqueGeometryBuffers(ArenaChasmType chasmType, BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteChasmRendererGeometryBuffers(ArenaChasmType chasmType, BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteChasmFloorRendererIndexBuffers(BufferView<int32_t> outOpaqueIndices); // Chasm walls are separate because they're conditionally enabled.
	void WriteChasmWallRendererIndexBuffers(ChasmWallIndexBuffer *outNorthIndices, ChasmWallIndexBuffer *outEastIndices, ChasmWallIndexBuffer *outSouthIndices, ChasmWallIndexBuffer *outWestIndices);
	void WriteDoorUniqueGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals);
	void WriteDoorRendererGeometryBuffers(BufferView<double> outPositions, BufferView<double> outNormals, BufferView<double> outTexCoords);
	void WriteDoorRendererIndexBuffers(BufferView<int32_t> outAlphaTestedIndices);
}

#endif
