#include <array>
#include <numeric>
#include <optional>

#include "SceneGraph.h"
#include "../ArenaRenderUtils.h"
#include "../RenderCamera.h"
#include "../Renderer.h"
#include "../RendererSystem3D.h"
#include "../RenderTriangle.h"
#include "../../Assets/MIFUtils.h"
#include "../../Entities/EntityManager.h"
#include "../../Entities/EntityVisibilityState.h"
#include "../../Math/Constants.h"
#include "../../Math/Matrix4.h"
#include "../../Media/TextureManager.h"
#include "../../World/ChunkManager.h"
#include "../../World/LevelInstance.h"
#include "../../World/MapDefinition.h"
#include "../../World/MapType.h"
#include "../../World/VoxelFacing2D.h"
#include "../../World/VoxelGeometry.h"

#include "components/debug/Debug.h"

namespace sgGeometry
{
	// Quad texture coordinates (top left, top right, etc.).
	const Double2 UV_TL(0.0, 0.0);
	const Double2 UV_TR(1.0, 0.0);
	const Double2 UV_BL(0.0, 1.0);
	const Double2 UV_BR(1.0, 1.0);

	// Makes the world space position of where a voxel should be.
	Double3 MakeVoxelPosition(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale)
	{
		const Int3 absoluteVoxel = VoxelUtils::chunkVoxelToNewVoxel(chunk, voxel);
		return Double3(
			static_cast<double>(absoluteVoxel.x),
			static_cast<double>(absoluteVoxel.y) * ceilingScale,
			static_cast<double>(absoluteVoxel.z));
	}

	// Makes the world space position of where an entity's bottom center should be. The ceiling scale is already
	// in the 3D point.
	Double3 MakeEntityPosition(const ChunkInt2 &chunk, const VoxelDouble3 &point)
	{
		return VoxelUtils::chunkPointToNewPoint(chunk, point);
	}

	// Makes a world space triangle. The given vertices are in model space and contain the 0->1 values where 1 is
	// a voxel corner.
	void MakeWorldSpaceVertices(const Double3 &voxelPosition, const Double3 &v0, const Double3 &v1, const Double3 &v2,
		double ceilingScale, Double3 *outV0, Double3 *outV1, Double3 *outV2)
	{
		outV0->x = voxelPosition.x + v0.x;
		outV0->y = voxelPosition.y + (v0.y * ceilingScale);
		outV0->z = voxelPosition.z + v0.z;

		outV1->x = voxelPosition.x + v1.x;
		outV1->y = voxelPosition.y + (v1.y * ceilingScale);
		outV1->z = voxelPosition.z + v1.z;

		outV2->x = voxelPosition.x + v2.x;
		outV2->y = voxelPosition.y + (v2.y * ceilingScale);
		outV2->z = voxelPosition.z + v2.z;
	}
}

namespace sgMesh
{
	constexpr int MAX_VERTICES_PER_VOXEL = 24;
	constexpr int MAX_INDICES_PER_VOXEL = 36;
	constexpr int INDICES_PER_TRIANGLE = 3;
	constexpr int COMPONENTS_PER_VERTEX = 3; // XYZ
	constexpr int ATTRIBUTES_PER_VERTEX = 2; // UV texture coordinates

	// The "ideal" vertices per voxel (no duplication).
	constexpr int GetVoxelUniqueVertexCount(ArenaTypes::VoxelType voxelType)
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
	constexpr int GetVoxelActualVertexCount(ArenaTypes::VoxelType voxelType)
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

	static constexpr int VOXEL_WALL_INDEX_BUFFER_INDEX_SIDE = 0;
	static constexpr int VOXEL_WALL_INDEX_BUFFER_INDEX_BOTTOM = 1;
	static constexpr int VOXEL_WALL_INDEX_BUFFER_INDEX_TOP = 2;

	static constexpr int VOXEL_RAISED_INDEX_BUFFER_INDEX_SIDE = 0;
	static constexpr int VOXEL_RAISED_INDEX_BUFFER_INDEX_BOTTOM = 0;
	static constexpr int VOXEL_RAISED_INDEX_BUFFER_INDEX_TOP = 1;

	constexpr int GetVoxelOpaqueIndexBufferCount(ArenaTypes::VoxelType voxelType)
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

	constexpr int GetVoxelOpaqueIndexCount(ArenaTypes::VoxelType voxelType, int bufferIndex)
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
			if (bufferIndex == VOXEL_WALL_INDEX_BUFFER_INDEX_SIDE)
			{
				triangleCount = 8;
			}
			else if ((bufferIndex == VOXEL_WALL_INDEX_BUFFER_INDEX_BOTTOM) || (bufferIndex == VOXEL_WALL_INDEX_BUFFER_INDEX_TOP))
			{
				triangleCount = 2;
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)) + " " + std::to_string(bufferIndex));
			}
			break;
		case ArenaTypes::VoxelType::Raised:
			if ((bufferIndex == VOXEL_RAISED_INDEX_BUFFER_INDEX_BOTTOM) ||
				(bufferIndex == VOXEL_RAISED_INDEX_BUFFER_INDEX_TOP))
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

		return triangleCount * INDICES_PER_TRIANGLE;
	}

	constexpr int GetVoxelAlphaTestedIndexBufferCount(ArenaTypes::VoxelType voxelType)
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

	constexpr int GetVoxelAlphaTestedIndexCount(ArenaTypes::VoxelType voxelType, int bufferIndex)
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
			if (bufferIndex == VOXEL_RAISED_INDEX_BUFFER_INDEX_SIDE)
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
		
		return triangleCount * INDICES_PER_TRIANGLE;
	}

	// Mesh writing functions. All of these are in model space, scaled by the ceiling scale.
	void WriteWallMeshGeometryBuffers(const VoxelDefinition::WallData &wall, double ceilingScale,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Wall);

		// One quad per face (results in duplication; necessary for correct texture mapping).
		const std::array<double, vertexCount * COMPONENTS_PER_VERTEX> vertices =
		{
			// X=0
			0.0, ceilingScale, 0.0,
			0.0, 0.0, 0.0,
			0.0, 0.0, 1.0,
			0.0, ceilingScale, 1.0,
			// X=1
			1.0, ceilingScale, 1.0,
			1.0, 0.0, 1.0,
			1.0, 0.0, 0.0,
			1.0, ceilingScale, 0.0,
			// Y=0
			0.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			// Y=1
			0.0, ceilingScale, 1.0,
			1.0, ceilingScale, 1.0,
			1.0, ceilingScale, 0.0,
			0.0, ceilingScale, 0.0,
			// Z=0
			1.0, ceilingScale, 0.0,
			1.0, 0.0, 0.0,
			0.0, 0.0, 0.0,
			0.0, ceilingScale, 0.0,
			// Z=1
			0.0, ceilingScale, 1.0,
			0.0, 0.0, 1.0,
			1.0, 0.0, 1.0,
			1.0, ceilingScale, 1.0
		};

		constexpr std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
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
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 3);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 0);

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, VOXEL_WALL_INDEX_BUFFER_INDEX_SIDE)> sideIndices =
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

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, VOXEL_WALL_INDEX_BUFFER_INDEX_BOTTOM)> bottomIndices =
		{
			// Y=0
			8, 9, 10,
			10, 11, 8
		};

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, VOXEL_WALL_INDEX_BUFFER_INDEX_TOP)> topIndices =
		{
			// Y=1
			12, 13, 14,
			14, 15, 12
		};

		std::copy(sideIndices.begin(), sideIndices.end(), outOpaqueSideIndices.get());
		std::copy(bottomIndices.begin(), bottomIndices.end(), outOpaqueBottomIndices.get());
		std::copy(topIndices.begin(), topIndices.end(), outOpaqueTopIndices.get());
	}

	void WriteFloorMeshGeometryBuffers(const VoxelDefinition::FloorData &floor, double ceilingScale,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Floor);

		const std::array<double, vertexCount * COMPONENTS_PER_VERTEX> vertices =
		{
			// Y=1
			0.0, ceilingScale, 1.0,
			1.0, ceilingScale, 1.0,
			1.0, ceilingScale, 0.0,
			0.0, ceilingScale, 0.0
		};

		constexpr std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
		{
			// Y=1
			0.0, 0.0,
			0.0, 1.0,
			1.0, 1.0,
			1.0, 0.0
		};

		std::copy(vertices.begin(), vertices.end(), outVertices.get());
		std::copy(attributes.begin(), attributes.end(), outAttributes.get());
	}

	void WriteFloorMeshIndexBuffers(BufferView<int32_t> outOpaqueIndices)
	{
		constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Floor;
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 1);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 0);

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, 0)> indices =
		{
			// Y=1
			0, 1, 2,
			2, 3, 0
		};

		std::copy(indices.begin(), indices.end(), outOpaqueIndices.get());
	}

	void WriteCeilingMeshGeometryBuffers(const VoxelDefinition::CeilingData &ceiling,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Ceiling);

		constexpr std::array<double, vertexCount * COMPONENTS_PER_VERTEX> vertices =
		{
			// Y=0
			0.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
		};

		constexpr std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
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
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 1);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 0);

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, 0)> indices =
		{
			// Y=0
			0, 1, 2,
			2, 3, 0
		};

		std::copy(indices.begin(), indices.end(), outOpaqueIndices.get());
	}

	void WriteRaisedMeshGeometryBuffers(const VoxelDefinition::RaisedData &raised, double ceilingScale,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Raised);
		const double yBottom = raised.yOffset * ceilingScale;
		const double yTop = (raised.yOffset + raised.ySize) * ceilingScale;
		const double vBottom = raised.vBottom;
		const double vTop = raised.vTop;

		// One quad per face (results in duplication; necessary for correct texture mapping).
		const std::array<double, vertexCount * COMPONENTS_PER_VERTEX> vertices =
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

		const std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
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
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 2);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 1);

		constexpr std::array<int32_t, GetVoxelAlphaTestedIndexCount(voxelType, VOXEL_RAISED_INDEX_BUFFER_INDEX_SIDE)> sideIndices =
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

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, VOXEL_RAISED_INDEX_BUFFER_INDEX_BOTTOM)> bottomIndices =
		{
			// Y=0
			8, 9, 10,
			10, 11, 8
		};

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, VOXEL_RAISED_INDEX_BUFFER_INDEX_TOP)> topIndices =
		{
			// Y=1
			12, 13, 14,
			14, 15, 12
		};

		std::copy(sideIndices.begin(), sideIndices.end(), outAlphaTestedSideIndices.get());
		std::copy(bottomIndices.begin(), bottomIndices.end(), outOpaqueBottomIndices.get());
		std::copy(topIndices.begin(), topIndices.end(), outOpaqueTopIndices.get());
	}

	void WriteDiagonalMeshGeometryBuffers(const VoxelDefinition::DiagonalData &diagonal,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Diagonal);

		constexpr std::array<double, vertexCount * COMPONENTS_PER_VERTEX> type1Vertices =
		{
			0.0, 1.0, 0.0,
			0.0, 0.0, 0.0,
			1.0, 0.0, 1.0,
			1.0, 1.0, 1.0,
		};

		constexpr std::array<double, vertexCount * COMPONENTS_PER_VERTEX> type2Vertices =
		{
			1.0, 1.0, 0.0,
			1.0, 0.0, 0.0,
			0.0, 0.0, 1.0,
			0.0, 1.0, 1.0,
		};

		const std::array<double, vertexCount * COMPONENTS_PER_VERTEX> &vertices = diagonal.type1 ? type1Vertices : type2Vertices;

		constexpr std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
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
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 1);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 0);

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, 0)> indices =
		{
			0, 1, 2,
			2, 3, 0
		};

		std::copy(indices.begin(), indices.end(), outOpaqueIndices.get());
	}

	void WriteTransparentWallMeshGeometryBuffers(const VoxelDefinition::TransparentWallData &transparentWall,
		double ceilingScale, BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::TransparentWall);

		// One quad per face (results in duplication; necessary for correct texture mapping).
		const std::array<double, vertexCount * COMPONENTS_PER_VERTEX> vertices =
		{
			// X=0
			0.0, ceilingScale, 0.0,
			0.0, 0.0, 0.0,
			0.0, 0.0, 1.0,
			0.0, ceilingScale, 1.0,
			// X=1
			1.0, ceilingScale, 1.0,
			1.0, 0.0, 1.0,
			1.0, 0.0, 0.0,
			1.0, ceilingScale, 0.0,
			// Z=0
			1.0, ceilingScale, 0.0,
			1.0, 0.0, 0.0,
			0.0, 0.0, 0.0,
			0.0, ceilingScale, 0.0,
			// Z=1
			0.0, ceilingScale, 1.0,
			0.0, 0.0, 1.0,
			1.0, 0.0, 1.0,
			1.0, ceilingScale, 1.0
		};

		constexpr std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
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
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 0);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 1);

		constexpr std::array<int32_t, GetVoxelAlphaTestedIndexCount(voxelType, 0)> indices =
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

	void WriteEdgeMeshGeometryBuffers(const VoxelDefinition::EdgeData &edge, double ceilingScale,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Edge);
		const double yBottom = edge.yOffset * ceilingScale;
		const double yTop = (edge.yOffset + 1.0) * ceilingScale;

		constexpr int componentCount = vertexCount * COMPONENTS_PER_VERTEX;

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
		switch (edge.facing)
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
			DebugNotImplementedMsg(std::to_string(static_cast<int>(edge.facing)));
		}

		constexpr int attributeCount = vertexCount * ATTRIBUTES_PER_VERTEX;
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

		const std::array<double, attributeCount> &attributes = edge.flipped ? flippedAttributes : unflippedAttributes;

		std::copy(vertices->begin(), vertices->end(), outVertices.get());
		std::copy(attributes.begin(), attributes.end(), outAttributes.get());
	}

	void WriteEdgeMeshIndexBuffers(BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Edge;
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 0);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 1);

		constexpr std::array<int32_t, GetVoxelAlphaTestedIndexCount(voxelType, 0)> indices =
		{
			0, 1, 2,
			2, 3, 0
		};

		std::copy(indices.begin(), indices.end(), outAlphaTestedIndices.get());
	}

	void WriteChasmMeshGeometryBuffers(const VoxelDefinition::ChasmData &chasm, double ceilingScale,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Chasm);
		const double yBottom = (chasm.type != ArenaTypes::ChasmType::Dry) ? (ceilingScale - 1.0) : 0.0;
		const double yTop = ceilingScale;

		const std::array<double, vertexCount * COMPONENTS_PER_VERTEX> vertices =
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

		constexpr std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
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
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 1);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 0); // @temp

		constexpr std::array<int32_t, GetVoxelOpaqueIndexCount(voxelType, 0)> opaqueIndices =
		{
			// Y=0
			0, 1, 2,
			2, 3, 0
		};

		// @temp: not writing chasm walls until later
		/*constexpr std::array<int32_t, GetVoxelAlphaTestedIndexCount(ArenaTypes::VoxelType::Chasm)> alphaTestedIndices =
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

	void WriteDoorMeshGeometryBuffers(const VoxelDefinition::DoorData &door, double ceilingScale,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		constexpr int vertexCount = GetVoxelActualVertexCount(ArenaTypes::VoxelType::Door);

		// @todo: does this need to care about the door type or can we do all that in the vertex shader?

		// One quad per face (results in duplication; necessary for correct texture mapping).
		const std::array<double, vertexCount * COMPONENTS_PER_VERTEX> vertices =
		{
			// X=0
			0.0, ceilingScale, 0.0,
			0.0, 0.0, 0.0,
			0.0, 0.0, 1.0,
			0.0, ceilingScale, 1.0,
			// X=1
			1.0, ceilingScale, 1.0,
			1.0, 0.0, 1.0,
			1.0, 0.0, 0.0,
			1.0, ceilingScale, 0.0,
			// Z=0
			1.0, ceilingScale, 0.0,
			1.0, 0.0, 0.0,
			0.0, 0.0, 0.0,
			0.0, ceilingScale, 0.0,
			// Z=1
			0.0, ceilingScale, 1.0,
			0.0, 0.0, 1.0,
			1.0, 0.0, 1.0,
			1.0, ceilingScale, 1.0
		};

		constexpr std::array<double, vertexCount * ATTRIBUTES_PER_VERTEX> attributes =
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
		static_assert(GetVoxelOpaqueIndexBufferCount(voxelType) == 0);
		static_assert(GetVoxelAlphaTestedIndexBufferCount(voxelType) == 1);

		constexpr std::array<int32_t, GetVoxelAlphaTestedIndexCount(voxelType, 0)> indices =
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

	void WriteVoxelMeshGeometryBuffers(const VoxelDefinition &voxelDef, double ceilingScale,
		BufferView<double> outVertices, BufferView<double> outAttributes)
	{
		const ArenaTypes::VoxelType voxelType = voxelDef.type;
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
			WriteWallMeshGeometryBuffers(voxelDef.wall, ceilingScale, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Floor:
			WriteFloorMeshGeometryBuffers(voxelDef.floor, ceilingScale, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Ceiling:
			WriteCeilingMeshGeometryBuffers(voxelDef.ceiling, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Raised:
			WriteRaisedMeshGeometryBuffers(voxelDef.raised, ceilingScale, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Diagonal:
			WriteDiagonalMeshGeometryBuffers(voxelDef.diagonal, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::TransparentWall:
			WriteTransparentWallMeshGeometryBuffers(voxelDef.transparentWall, ceilingScale, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Edge:
			WriteEdgeMeshGeometryBuffers(voxelDef.edge, ceilingScale, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Chasm:
			WriteChasmMeshGeometryBuffers(voxelDef.chasm, ceilingScale, outVertices, outAttributes);
			break;
		case ArenaTypes::VoxelType::Door:
			WriteDoorMeshGeometryBuffers(voxelDef.door, ceilingScale, outVertices, outAttributes);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}

	void WriteVoxelMeshIndexBuffers(const VoxelDefinition &voxelDef, BufferView<int32_t> outOpaqueIndices0, 
		BufferView<int32_t> outOpaqueIndices1, BufferView<int32_t> outOpaqueIndices2, 
		BufferView<int32_t> outAlphaTestedIndices)
	{
		const ArenaTypes::VoxelType voxelType = voxelDef.type;
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

namespace sgTexture
{
	// Loads the given voxel definition's textures into the voxel textures list if they haven't been loaded yet.
	void LoadVoxelDefTextures(const VoxelDefinition &voxelDef, std::vector<SceneGraph::LoadedVoxelTexture> &voxelTextures,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelDef.getTextureAssetCount(); i++)
		{
			const TextureAsset &textureAsset = voxelDef.getTextureAsset(i);
			const auto cacheIter = std::find_if(voxelTextures.begin(), voxelTextures.end(),
				[&textureAsset](const SceneGraph::LoadedVoxelTexture &loadedTexture)
			{
				return loadedTexture.textureAsset == textureAsset;
			});

			if (cacheIter == voxelTextures.end())
			{
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load voxel texture \"" + textureAsset.filename + "\".");
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				ObjectTextureID voxelTextureID;
				if (!renderer.tryCreateObjectTexture(textureBuilder, &voxelTextureID))
				{
					DebugLogWarning("Couldn't create voxel texture \"" + textureAsset.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef voxelTextureRef(voxelTextureID, renderer);
				SceneGraph::LoadedVoxelTexture newTexture;
				newTexture.init(textureAsset, std::move(voxelTextureRef));
				voxelTextures.emplace_back(std::move(newTexture));
			}
		}
	}

	// Loads the given entity definition's textures into the entity textures list if they haven't been loaded yet.
	void LoadEntityDefTextures(const EntityDefinition &entityDef, std::vector<SceneGraph::LoadedEntityTexture> &entityTextures,
		TextureManager &textureManager, Renderer &renderer)
	{
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && entityDef.getDoodad().puddle;

		auto processKeyframe = [&entityTextures, &textureManager, &renderer, reflective](
			const EntityAnimationDefinition::Keyframe &keyframe, bool flipped)
		{
			const TextureAsset &textureAsset = keyframe.getTextureAsset();
			const auto cacheIter = std::find_if(entityTextures.begin(), entityTextures.end(),
				[&textureAsset, flipped, reflective](const SceneGraph::LoadedEntityTexture &loadedTexture)
			{
				return (loadedTexture.textureAsset == textureAsset) && (loadedTexture.flipped == flipped) &&
					(loadedTexture.reflective == reflective);
			});

			if (cacheIter == entityTextures.end())
			{
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load entity texture \"" + textureAsset.filename + "\".");
					return;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				const int textureWidth = textureBuilder.getWidth();
				const int textureHeight = textureBuilder.getHeight();

				ObjectTextureID entityTextureID;
				if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, false, &entityTextureID))
				{
					DebugLogWarning("Couldn't create entity texture \"" + textureAsset.filename + "\".");
					return;
				}

				ScopedObjectTextureRef entityTextureRef(entityTextureID, renderer);
				DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
				const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
				const uint8_t *srcTexels = srcTexture.texels.get();

				LockedTexture lockedEntityTexture = renderer.lockObjectTexture(entityTextureID);
				if (!lockedEntityTexture.isValid())
				{
					DebugLogWarning("Couldn't lock entity texture \"" + textureAsset.filename + "\".");
					return;
				}

				DebugAssert(!lockedEntityTexture.isTrueColor);
				uint8_t *dstTexels = static_cast<uint8_t*>(lockedEntityTexture.texels);

				for (int y = 0; y < textureHeight; y++)
				{
					for (int x = 0; x < textureWidth; x++)
					{
						// Mirror texture if this texture is for an angle that gets mirrored.
						const int srcIndex = x + (y * textureWidth);
						const int dstIndex = (!flipped ? x : (textureWidth - 1 - x)) + (y * textureWidth);
						dstTexels[dstIndex] = srcTexels[srcIndex];
					}
				}

				renderer.unlockObjectTexture(entityTextureID);

				SceneGraph::LoadedEntityTexture newTexture;
				newTexture.init(textureAsset, flipped, reflective, std::move(entityTextureRef));
				entityTextures.emplace_back(std::move(newTexture));
			}
		};

		for (int i = 0; i < animDef.getStateCount(); i++)
		{
			const EntityAnimationDefinition::State &state = animDef.getState(i);
			for (int j = 0; j < state.getKeyframeListCount(); j++)
			{
				const EntityAnimationDefinition::KeyframeList &keyframeList = state.getKeyframeList(j);
				const bool flipped = keyframeList.isFlipped();
				for (int k = 0; k < keyframeList.getKeyframeCount(); k++)
				{
					const EntityAnimationDefinition::Keyframe &keyframe = keyframeList.getKeyframe(k);
					processKeyframe(keyframe, flipped);
				}
			}
		}
	}

	// Loads the chasm floor materials for the given chasm. This should be done before loading the chasm wall
	// as each wall material has a dependency on the floor texture.
	void LoadChasmFloorTextures(ArenaTypes::ChasmType chasmType, std::vector<SceneGraph::LoadedChasmTextureList> &chasmTextureLists,
		TextureManager &textureManager, Renderer &renderer)
	{
		const auto cacheIter = std::find_if(chasmTextureLists.begin(), chasmTextureLists.end(),
			[chasmType](const SceneGraph::LoadedChasmTextureList &loadedTextureList)
		{
			return loadedTextureList.chasmType == chasmType;
		});

		DebugAssertMsg(cacheIter == chasmTextureLists.end(), "Already loaded chasm floor textures for type \"" +
			std::to_string(static_cast<int>(chasmType)) + "\".");

		const bool hasTexturedAnim = chasmType != ArenaTypes::ChasmType::Dry;
		if (hasTexturedAnim)
		{
			const std::string chasmFilename = [chasmType]()
			{
				if (chasmType == ArenaTypes::ChasmType::Wet)
				{
					return ArenaRenderUtils::CHASM_WATER_FILENAME;
				}
				else if (chasmType == ArenaTypes::ChasmType::Lava)
				{
					return ArenaRenderUtils::CHASM_LAVA_FILENAME;
				}
				else
				{
					DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(chasmType)));
				}
			}();

			SceneGraph::LoadedChasmTextureList newTextureList;
			newTextureList.init(chasmType);

			const Buffer<TextureAsset> textureAssets = TextureUtils::makeTextureAssets(chasmFilename, textureManager);
			for (int i = 0; i < textureAssets.getCount(); i++)
			{
				const TextureAsset &textureAsset = textureAssets.get(i);
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load chasm texture \"" + textureAsset.filename + "\".");
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				ObjectTextureID chasmTextureID;
				if (!renderer.tryCreateObjectTexture(textureBuilder, &chasmTextureID))
				{
					DebugLogWarning("Couldn't create chasm texture \"" + textureAsset.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef chasmTextureRef(chasmTextureID, renderer);

				// Populate chasmTextureRefs, leave wall entries empty since they're populated next.
				newTextureList.chasmTextureRefs.emplace_back(std::move(chasmTextureRef));
			}

			chasmTextureLists.emplace_back(std::move(newTextureList));
		}
		else
		{
			// Dry chasms are just a single color, no texture asset available.
			ObjectTextureID dryChasmTextureID;
			if (!renderer.tryCreateObjectTexture(1, 1, false, &dryChasmTextureID))
			{
				DebugLogWarning("Couldn't create dry chasm texture.");
				return;
			}

			ScopedObjectTextureRef dryChasmTextureRef(dryChasmTextureID, renderer);
			LockedTexture lockedTexture = renderer.lockObjectTexture(dryChasmTextureID);
			if (!lockedTexture.isValid())
			{
				DebugLogWarning("Couldn't lock dry chasm texture for writing.");
				return;
			}

			DebugAssert(!lockedTexture.isTrueColor);
			uint8_t *texels = static_cast<uint8_t*>(lockedTexture.texels);
			*texels = ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR;
			renderer.unlockObjectTexture(dryChasmTextureID);

			SceneGraph::LoadedChasmTextureList newTextureList;
			newTextureList.init(chasmType);

			// Populate chasmTextureRefs, leave wall entries empty since they're populated next.
			newTextureList.chasmTextureRefs.emplace_back(std::move(dryChasmTextureRef));
			chasmTextureLists.emplace_back(std::move(newTextureList));
		}
	}

	// Loads the chasm wall material for the given chasm and texture asset. This expects the chasm floor material to
	// already be loaded and available for sharing.
	void LoadChasmWallTextures(ArenaTypes::ChasmType chasmType, const TextureAsset &textureAsset,
		std::vector<SceneGraph::LoadedChasmTextureList> &chasmTextureLists, TextureManager &textureManager, Renderer &renderer)
	{
		const auto listIter = std::find_if(chasmTextureLists.begin(), chasmTextureLists.end(),
			[chasmType](const SceneGraph::LoadedChasmTextureList &loadedTextureList)
		{
			return loadedTextureList.chasmType == chasmType;
		});

		DebugAssertMsg(listIter != chasmTextureLists.end(), "Expected loaded chasm floor texture list for type \"" +
			std::to_string(static_cast<int>(chasmType)) + "\".");

		std::vector<SceneGraph::LoadedChasmTextureList::WallEntry> &wallEntries = listIter->wallEntries;
		const auto entryIter = std::find_if(wallEntries.begin(), wallEntries.end(),
			[&textureAsset](const SceneGraph::LoadedChasmTextureList::WallEntry &wallEntry)
		{
			return wallEntry.wallTextureAsset == textureAsset;
		});

		if (entryIter == wallEntries.end())
		{
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
			if (!textureBuilderID.has_value())
			{
				DebugLogWarning("Couldn't load chasm wall texture \"" + textureAsset.filename + "\".");
				return;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID wallTextureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &wallTextureID))
			{
				DebugLogWarning("Couldn't create chasm wall texture \"" + textureAsset.filename + "\".");
				return;
			}

			SceneGraph::LoadedChasmTextureList::WallEntry newWallEntry;
			newWallEntry.wallTextureAsset = textureAsset;
			newWallEntry.wallTextureRef.init(wallTextureID, renderer);
			wallEntries.emplace_back(std::move(newWallEntry));
		}
	}
}

void SceneGraph::LoadedVoxelTexture::init(const TextureAsset &textureAsset,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

void SceneGraph::LoadedEntityTexture::init(const TextureAsset &textureAsset, bool flipped,
	bool reflective, ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->flipped = flipped;
	this->reflective = reflective;
	this->objectTextureRef = std::move(objectTextureRef);
}

void SceneGraph::LoadedChasmTextureList::init(ArenaTypes::ChasmType chasmType)
{
	this->chasmType = chasmType;
}

ObjectTextureID SceneGraph::getVoxelTextureID(const TextureAsset &textureAsset) const
{
	const auto iter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
		[&textureAsset](const LoadedVoxelTexture &loadedTexture)
	{
		return loadedTexture.textureAsset == textureAsset;
	});

	DebugAssertMsg(iter != this->voxelTextures.end(), "No loaded voxel texture for \"" + textureAsset.filename + "\".");
	const ScopedObjectTextureRef &objectTextureRef = iter->objectTextureRef;
	return objectTextureRef.get();
}

ObjectTextureID SceneGraph::getEntityTextureID(const TextureAsset &textureAsset, bool flipped, bool reflective) const
{
	const auto iter = std::find_if(this->entityTextures.begin(), this->entityTextures.end(),
		[&textureAsset, flipped, reflective](const LoadedEntityTexture &loadedTexture)
	{
		return (loadedTexture.textureAsset == textureAsset) && (loadedTexture.flipped == flipped) &&
			(loadedTexture.reflective == reflective);
	});

	DebugAssertMsg(iter != this->entityTextures.end(), "No loaded entity texture for \"" + textureAsset.filename + "\".");
	const ScopedObjectTextureRef &objectTextureRef = iter->objectTextureRef;
	return objectTextureRef.get();
}

ObjectTextureID SceneGraph::getChasmFloorTextureID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent) const
{
	const auto iter = std::find_if(this->chasmTextureLists.begin(), this->chasmTextureLists.end(),
		[chasmType](const LoadedChasmTextureList &loadedTextureList)
	{
		return loadedTextureList.chasmType == chasmType;
	});

	DebugAssertMsg(iter != this->chasmTextureLists.end(), "No loaded chasm floor texture for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");
	const std::vector<ScopedObjectTextureRef> &floorTextureRefs = iter->chasmTextureRefs;
	const int textureCount = static_cast<int>(floorTextureRefs.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(floorTextureRefs, index);
	const ScopedObjectTextureRef &floorTextureRef = floorTextureRefs[index];
	return floorTextureRef.get();
}

ObjectTextureID SceneGraph::getChasmWallTextureID(ArenaTypes::ChasmType chasmType, const TextureAsset &textureAsset) const
{
	const auto listIter = std::find_if(this->chasmTextureLists.begin(), this->chasmTextureLists.end(),
		[chasmType, &textureAsset](const LoadedChasmTextureList &loadedTextureList)
	{
		return loadedTextureList.chasmType == chasmType;
	});

	DebugAssertMsg(listIter != this->chasmTextureLists.end(), "No loaded chasm floor texture for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");

	const std::vector<LoadedChasmTextureList::WallEntry> &entries = listIter->wallEntries;
	const auto entryIter = std::find_if(entries.begin(), entries.end(),
		[&textureAsset](const LoadedChasmTextureList::WallEntry &entry)
	{
		return entry.wallTextureAsset == textureAsset;
	});

	DebugAssertMsg(entryIter != entries.end(), "No loaded chasm wall texture for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\" and texture \"" + textureAsset.filename + "\".");

	const ScopedObjectTextureRef &wallTextureRef = entryIter->wallTextureRef;
	return wallTextureRef.get();
}

BufferView<const RenderDrawCall> SceneGraph::getDrawCalls() const
{
	return BufferView<const RenderDrawCall>(this->drawCalls.data(), static_cast<int>(this->drawCalls.size()));
}

void SceneGraph::loadTextures(const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, TextureManager &textureManager,
	Renderer &renderer)
{
	// Load chasm floor textures, independent of voxels in the level. Do this before chasm wall texture loading
	// because walls are multi-textured and depend on the chasm animation textures.
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Dry, this->chasmTextureLists, textureManager, renderer);
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Wet, this->chasmTextureLists, textureManager, renderer);
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Lava, this->chasmTextureLists, textureManager, renderer);

	// Load textures known at level load time. Note that none of the object texture IDs allocated here are
	// matched with voxel/entity instances until the chunks containing them are created.
	auto loadLevelDefTextures = [this, &mapDefinition, &textureManager, &renderer](int levelIndex)
	{
		const LevelInfoDefinition &levelInfoDef = mapDefinition.getLevelInfoForLevel(levelIndex);

		for (int i = 0; i < levelInfoDef.getVoxelDefCount(); i++)
		{
			const VoxelDefinition &voxelDef = levelInfoDef.getVoxelDef(i);
			sgTexture::LoadVoxelDefTextures(voxelDef, this->voxelTextures, textureManager, renderer);

			if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
			{
				const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
				sgTexture::LoadChasmWallTextures(chasm.type, chasm.textureAsset, this->chasmTextureLists, textureManager, renderer);
			}
		}

		for (int i = 0; i < levelInfoDef.getEntityDefCount(); i++)
		{
			const EntityDefinition &entityDef = levelInfoDef.getEntityDef(i);
			sgTexture::LoadEntityDefTextures(entityDef, this->entityTextures, textureManager, renderer);
		}
	};

	const MapType mapType = mapDefinition.getMapType();
	if ((mapType == MapType::Interior) || (mapType == MapType::City))
	{
		// Load textures for the active level.
		DebugAssert(activeLevelIndex.has_value());
		loadLevelDefTextures(*activeLevelIndex);
	}
	else if (mapType == MapType::Wilderness)
	{
		// Load textures for all wilderness chunks.
		for (int i = 0; i < mapDefinition.getLevelCount(); i++)
		{
			loadLevelDefTextures(i);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
	}

	// Load citizen textures if citizens can exist in the level.
	if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		DebugAssert(citizenGenInfo.has_value());
		const EntityDefinition &maleEntityDef = *citizenGenInfo->maleEntityDef;
		const EntityDefinition &femaleEntityDef = *citizenGenInfo->femaleEntityDef;
		sgTexture::LoadEntityDefTextures(maleEntityDef, this->entityTextures, textureManager, renderer);
		sgTexture::LoadEntityDefTextures(femaleEntityDef, this->entityTextures, textureManager, renderer);
	}
}

// @todo: the problem with this design is it only cares about the active chunks, not ones that haven't loaded yet,
// which probably have different textures in them. Should probably try a LevelDefinition way instead?
void SceneGraph::loadVoxels(const LevelInstance &levelInst, const RenderCamera &camera, bool nightLightsAreActive,
	RendererSystem3D &renderer)
{
	// Expect empty chunks to have been created just now (it's done before this in the edge case
	// there are no voxels at all since entities rely on chunks existing).
	DebugAssert(!this->graphChunks.empty());

	const double ceilingScale = levelInst.getCeilingScale();
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	for (int chunkIndex = 0; chunkIndex < chunkManager.getChunkCount(); chunkIndex++)
	{
		const Chunk &chunk = chunkManager.getChunk(chunkIndex);
		SceneGraphChunk &graphChunk = this->graphChunks[chunkIndex];

		// Add voxel definitions to the scene graph and create mappings to them.
		for (int voxelDefIndex = 0; voxelDefIndex < chunk.getVoxelDefCount(); voxelDefIndex++)
		{
			const Chunk::VoxelID voxelID = static_cast<Chunk::VoxelID>(voxelDefIndex);
			const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
			const ArenaTypes::VoxelType voxelType = voxelDef.type;

			SceneGraphVoxelDefinition graphVoxelDef;
			if (voxelType != ArenaTypes::VoxelType::None) // Only attempt to create buffers for non-air voxels.
			{
				const int vertexCount = sgMesh::GetVoxelActualVertexCount(voxelType);
				if (!renderer.tryCreateVertexBuffer(vertexCount, sgMesh::COMPONENTS_PER_VERTEX, &graphVoxelDef.vertexBufferID))
				{
					DebugLogError("Couldn't create vertex buffer for voxel ID " + std::to_string(voxelID) +
						" in chunk (" + chunk.getPosition().toString() + ").");
					continue;
				}

				if (!renderer.tryCreateAttributeBuffer(vertexCount, sgMesh::ATTRIBUTES_PER_VERTEX, &graphVoxelDef.attributeBufferID))
				{
					DebugLogError("Couldn't create attribute buffer for voxel ID " + std::to_string(voxelID) +
						" in chunk (" + chunk.getPosition().toString() + ").");
					graphVoxelDef.freeBuffers(renderer);
					continue;
				}

				std::array<double, sgMesh::MAX_VERTICES_PER_VOXEL * sgMesh::COMPONENTS_PER_VERTEX> vertices;
				std::array<double, sgMesh::MAX_VERTICES_PER_VOXEL * sgMesh::ATTRIBUTES_PER_VERTEX> attributes;
				std::array<int32_t, sgMesh::MAX_INDICES_PER_VOXEL> opaqueIndices0, opaqueIndices1, opaqueIndices2;
				std::array<int32_t, sgMesh::MAX_INDICES_PER_VOXEL> alphaTestedIndices0;
				vertices.fill(0.0);
				attributes.fill(0.0);
				opaqueIndices0.fill(-1);
				opaqueIndices1.fill(-1);
				opaqueIndices2.fill(-1);
				alphaTestedIndices0.fill(-1);

				const std::array<const decltype(opaqueIndices0)*, 3> opaqueIndicesPtrs =
				{
					&opaqueIndices0, &opaqueIndices1, &opaqueIndices2
				};

				// Generate mesh geometry and indices for this voxel definition.
				sgMesh::WriteVoxelMeshGeometryBuffers(voxelDef, ceilingScale,
					BufferView<double>(vertices.data(), static_cast<int>(vertices.size())),
					BufferView<double>(attributes.data(), static_cast<int>(attributes.size())));
				sgMesh::WriteVoxelMeshIndexBuffers(voxelDef,
					BufferView<int32_t>(opaqueIndices0.data(), static_cast<int>(opaqueIndices0.size())),
					BufferView<int32_t>(opaqueIndices1.data(), static_cast<int>(opaqueIndices1.size())),
					BufferView<int32_t>(opaqueIndices2.data(), static_cast<int>(opaqueIndices2.size())),
					BufferView<int32_t>(alphaTestedIndices0.data(), static_cast<int>(alphaTestedIndices0.size())));

				renderer.populateVertexBuffer(graphVoxelDef.vertexBufferID,
					BufferView<const double>(vertices.data(), vertexCount * sgMesh::COMPONENTS_PER_VERTEX));
				renderer.populateAttributeBuffer(graphVoxelDef.attributeBufferID,
					BufferView<const double>(attributes.data(), vertexCount * sgMesh::ATTRIBUTES_PER_VERTEX));

				const int opaqueIndexBufferCount = sgMesh::GetVoxelOpaqueIndexBufferCount(voxelType);				
				for (int bufferIndex = 0; bufferIndex < opaqueIndexBufferCount; bufferIndex++)
				{
					const int opaqueIndexCount = sgMesh::GetVoxelOpaqueIndexCount(voxelType, bufferIndex);
					IndexBufferID &opaqueIndexBufferID = graphVoxelDef.opaqueIndexBufferIDs[bufferIndex];
					if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &opaqueIndexBufferID))
					{
						DebugLogError("Couldn't create opaque index buffer for voxel ID " + std::to_string(voxelID) +
							" in chunk (" + chunk.getPosition().toString() + ").");
						graphVoxelDef.freeBuffers(renderer);
						continue;
					}

					graphVoxelDef.opaqueIndexBufferIdCount++;

					const auto &indices = *opaqueIndicesPtrs[bufferIndex];
					renderer.populateIndexBuffer(opaqueIndexBufferID,
						BufferView<const int32_t>(indices.data(), opaqueIndexCount));
				}

				const bool hasAlphaTestedIndexBuffer = sgMesh::GetVoxelAlphaTestedIndexBufferCount(voxelType) > 0;
				if (hasAlphaTestedIndexBuffer)
				{
					const int alphaTestedIndexCount = sgMesh::GetVoxelAlphaTestedIndexCount(voxelType, 0);
					if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &graphVoxelDef.alphaTestedIndexBufferID))
					{
						DebugLogError("Couldn't create alpha-tested index buffer for voxel ID " + std::to_string(voxelID) +
							" in chunk (" + chunk.getPosition().toString() + ").");
						graphVoxelDef.freeBuffers(renderer);
						continue;
					}

					renderer.populateIndexBuffer(graphVoxelDef.alphaTestedIndexBufferID,
						BufferView<const int32_t>(alphaTestedIndices0.data(), alphaTestedIndexCount));
				}
			}

			const SceneGraphVoxelID graphVoxelID = graphChunk.addVoxelDef(std::move(graphVoxelDef));
			graphChunk.voxelDefMappings.emplace(voxelID, graphVoxelID);
		}

		auto addDrawCall = [this, ceilingScale](SNInt x, int y, WEInt z, VertexBufferID vertexBufferID,
			AttributeBufferID attributeBufferID, IndexBufferID indexBufferID, ObjectTextureID textureID,
			PixelShaderType pixelShaderType)
		{
			RenderDrawCall drawCall;
			drawCall.vertexBufferID = vertexBufferID;
			drawCall.attributeBufferID = attributeBufferID;
			drawCall.indexBufferID = indexBufferID;
			drawCall.textureIDs[0] = textureID; // @todo: add another parameter for multi-texturing for chasm walls
			drawCall.vertexShaderType = VertexShaderType::Default;
			drawCall.pixelShaderType = pixelShaderType;
			drawCall.worldSpaceOffset = Double3(
				static_cast<SNDouble>(x),
				static_cast<double>(y) * ceilingScale,
				static_cast<WEDouble>(z));
			
			this->drawCalls.emplace_back(std::move(drawCall));
		};

		// Generate draw calls for each non-air voxel.
		// @todo: draw calls should be stored per chunk; that way it's easier to sort by distance,
		// add an entire chunk, cull an entire chunk, etc.
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (int y = 0; y < chunk.getHeight(); y++)
			{
				for (SNInt x = 0; x < Chunk::WIDTH; x++)
				{
					// Get the voxel def mapping's index and use it for this voxel.
					const Chunk::VoxelID voxelID = chunk.getVoxel(x, y, z);
					const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
					if (voxelDef.type == ArenaTypes::VoxelType::None)
					{
						continue;
					}

					const auto defIter = graphChunk.voxelDefMappings.find(voxelID);
					DebugAssert(defIter != graphChunk.voxelDefMappings.end());
					const SceneGraphVoxelID graphVoxelID = defIter->second;
					graphChunk.voxels.set(x, y, z, graphVoxelID);

					// Convert voxel XYZ to world space.
					const NewInt2 worldXZ = VoxelUtils::chunkVoxelToNewVoxel(chunk.getPosition(), VoxelInt2(x, z));
					const int worldY = y;

					const bool usesVoxelTextures = voxelDef.type != ArenaTypes::VoxelType::Chasm;

					const SceneGraphVoxelDefinition &graphVoxelDef = graphChunk.voxelDefs[graphVoxelID];
					for (int bufferIndex = 0; bufferIndex < graphVoxelDef.opaqueIndexBufferIdCount; bufferIndex++)
					{
						ObjectTextureID textureID = -1;

						if (usesVoxelTextures)
						{
							const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
								[&voxelDef](const SceneGraph::LoadedVoxelTexture &loadedTexture)
							{
								return loadedTexture.textureAsset == voxelDef.getTextureAsset(0); // @todo: this index should be provided by the voxel def
							});

							if (voxelTextureIter != this->voxelTextures.end())
							{
								textureID = voxelTextureIter->objectTextureRef.get();
							}
						}
						else
						{
							const ArenaTypes::ChasmType chasmType = voxelDef.chasm.type;
							const auto chasmIter = std::find_if(this->chasmTextureLists.begin(), this->chasmTextureLists.end(),
								[chasmType](const SceneGraph::LoadedChasmTextureList &loadedTextureList)
							{
								return loadedTextureList.chasmType == chasmType;
							});

							if (chasmIter != this->chasmTextureLists.end())
							{
								const auto &chasmTextureRefs = chasmIter->chasmTextureRefs;
								DebugAssert(!chasmTextureRefs.empty());
								const ScopedObjectTextureRef &chasmTextureRef = chasmTextureRefs[0]; // @todo: daytimePercent? chasmAnimPercent?
								textureID = chasmTextureRef.get();
							}
						}

						if (textureID < 0)
						{
							DebugLogError("Couldn't find texture ID for opaque voxel texture.");
							continue;
						}

						const IndexBufferID opaqueIndexBufferID = graphVoxelDef.opaqueIndexBufferIDs[bufferIndex];
						addDrawCall(worldXZ.x, worldY, worldXZ.y, graphVoxelDef.vertexBufferID, graphVoxelDef.attributeBufferID,
							opaqueIndexBufferID, textureID, PixelShaderType::Opaque);
					}

					if (graphVoxelDef.alphaTestedIndexBufferID >= 0)
					{
						ObjectTextureID textureID = -1;

						if (usesVoxelTextures)
						{
							const auto voxelTextureIter = std::find_if(this->voxelTextures.begin(), this->voxelTextures.end(),
								[&voxelDef](const SceneGraph::LoadedVoxelTexture &loadedTexture)
							{
								return loadedTexture.textureAsset == voxelDef.getTextureAsset(0); // @todo: this index should be provided by the voxel def
							});

							if (voxelTextureIter != this->voxelTextures.end())
							{
								textureID = voxelTextureIter->objectTextureRef.get();
							}
						}
						else
						{
							const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
							const ArenaTypes::ChasmType chasmType = chasm.type;
							const auto chasmIter = std::find_if(this->chasmTextureLists.begin(), this->chasmTextureLists.end(),
								[chasmType](const SceneGraph::LoadedChasmTextureList &loadedTextureList)
							{
								return loadedTextureList.chasmType == chasmType;
							});

							if (chasmIter != this->chasmTextureLists.end())
							{
								const auto &wallEntries = chasmIter->wallEntries;
								const auto wallTextureIter = std::find_if(wallEntries.begin(), wallEntries.end(),
									[&chasm](const SceneGraph::LoadedChasmTextureList::WallEntry &wallEntry)
								{
									return wallEntry.wallTextureAsset == chasm.textureAsset;
								});

								DebugAssert(wallTextureIter != wallEntries.end());
								textureID = wallTextureIter->wallTextureRef.get();
							}
						}

						if (textureID < 0)
						{
							DebugLogError("Couldn't find texture ID for opaque voxel texture.");
							continue;
						}

						addDrawCall(worldXZ.x, worldY, worldXZ.y, graphVoxelDef.vertexBufferID, graphVoxelDef.attributeBufferID,
							graphVoxelDef.alphaTestedIndexBufferID, textureID, PixelShaderType::AlphaTested);
					}
				}
			}
		}
	}
}

/*void SceneGraph::loadEntities(const LevelInstance &levelInst, const RenderCamera &camera,
	const EntityDefinitionLibrary &entityDefLibrary, bool nightLightsAreActive, bool playerHasLight,
	RendererSystem3D &renderer)
{
	DebugAssert(!this->graphChunks.empty());

	// @todo
	DebugNotImplemented();
}

void SceneGraph::loadSky(const SkyInstance &skyInst, double daytimePercent, double latitude, RendererSystem3D &renderer)
{
	// @todo
	DebugNotImplemented();
}

void SceneGraph::loadWeather(const SkyInstance &skyInst, double daytimePercent, RendererSystem3D &renderer)
{
	// @todo
	DebugNotImplemented();
}*/

void SceneGraph::loadScene(const LevelInstance &levelInst, const SkyInstance &skyInst,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
	double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight, double daytimePercent,
	double latitude, const EntityDefinitionLibrary &entityDefLibrary, TextureManager &textureManager,
	Renderer &renderer, RendererSystem3D &renderer3D)
{
	DebugAssert(this->graphChunks.empty());
	DebugAssert(this->drawCalls.empty());

	// Create empty graph chunks using the chunk manager's chunks as a reference.
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	for (int i = 0; i < chunkManager.getChunkCount(); i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		SceneGraphChunk graphChunk;
		graphChunk.init(chunk.getPosition(), chunk.getHeight());
		this->graphChunks.emplace_back(std::move(graphChunk));
	}

	// @todo: load textures somewhere in here and in a way that their draw calls can be generated; maybe want to store
	// TextureAssets with SceneGraphVoxelDefinition? Might want textures to be ref-counted if reused between chunks.

	const double ceilingScale = levelInst.getCeilingScale();
	this->loadTextures(activeLevelIndex, mapDefinition, citizenGenInfo, textureManager, renderer);
	this->loadVoxels(levelInst, camera, nightLightsAreActive, renderer3D);

	/*this->loadEntities(levelInst, camera, entityDefLibrary, nightLightsAreActive, playerHasLight, renderer3D);
	this->loadSky(skyInst, daytimePercent, latitude, renderer3D);
	this->loadWeather(skyInst, daytimePercent, renderer3D);*/

	// @todo: populate draw calls since update() only operates on dirty stuff from chunk manager/entity manager/etc.
}

void SceneGraph::unloadScene(RendererSystem3D &renderer)
{
	// Free vertex/attribute/index buffer IDs from renderer.
	for (SceneGraphChunk &chunk : this->graphChunks)
	{
		chunk.freeBuffers(renderer);
	}

	this->graphChunks.clear();
	this->drawCalls.clear();
}

/*void SceneGraph::updateVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double chasmAnimPercent,
	bool nightLightsAreActive, RendererSystem3D &renderer)
{
	const ChunkManager &chunkManager = levelInst.getChunkManager();
	const int chunkCount = chunkManager.getChunkCount();

	// Remove stale graph chunks.
	for (int i = static_cast<int>(this->graphChunks.size()) - 1; i >= 0; i--)
	{
		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		const ChunkInt2 &graphChunkPos = graphChunk.position;

		bool isStale = true;
		for (int j = 0; j < chunkCount; j++)
		{
			const Chunk &chunk = chunkManager.getChunk(j);
			const ChunkInt2 &chunkPos = chunk.getPosition();
			if (chunkPos == graphChunkPos)
			{
				isStale = false;
				break;
			}
		}

		if (isStale)
		{
			this->graphChunks.erase(this->graphChunks.begin() + i);
		}
	}

	// Insert new empty graph chunks (to have their voxels updated by the associated chunk's dirty voxels).
	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const ChunkInt2 &chunkPos = chunk.getPosition();

		bool shouldInsert = true;
		for (int j = 0; j < static_cast<int>(this->graphChunks.size()); j++)
		{
			const SceneGraphChunk &graphChunk = this->graphChunks[j];
			const ChunkInt2 &graphChunkPos = graphChunk.position;
			if (graphChunkPos == chunkPos)
			{
				shouldInsert = false;
				break;
			}
		}

		if (shouldInsert)
		{
			SceneGraphChunk graphChunk;
			graphChunk.init(chunkPos, chunk.getHeight());
			this->graphChunks.emplace_back(std::move(graphChunk));
		}
	}

	// @todo: decide how to load voxels into these new graph chunks - maybe want to do the chunk adding/removing
	// before updateVoxels(), same as how loadVoxels() expects the chunks to already be there (albeit empty).

	// Arbitrary value, just needs to be long enough to touch the farthest chunks in practice.
	// - @todo: maybe use far clipping plane value?
	constexpr double frustumLength = 1000.0;

	const Double2 cameraEye2D(camera.point.x, camera.point.z);
	const Double2 cameraFrustumLeftPoint2D(
		camera.point.x + ((camera.forwardScaled.x - camera.rightScaled.x) * frustumLength),
		camera.point.z + ((camera.forwardScaled.z - camera.rightScaled.z) * frustumLength));
	const Double2 cameraFrustumRightPoint2D(
		camera.point.x + ((camera.forwardScaled.x + camera.rightScaled.x) * frustumLength),
		camera.point.z + ((camera.forwardScaled.z + camera.rightScaled.z) * frustumLength));

	// Update dirty voxels in each scene graph chunk.
	// @todo: animating voxel instances should be set dirty every frame in Chunk::update() or whatever
	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const SNInt chunkWidth = Chunk::WIDTH;
		const int chunkHeight = chunk.getHeight();
		const WEInt chunkDepth = Chunk::DEPTH;

		const ChunkInt2 chunkPos = chunk.getPosition();

		auto getVoxelFadePercentOrDefault = [&chunk](const VoxelInt3 &voxelPos)
		{
			const VoxelInstance *fadingVoxelInst = chunk.tryGetVoxelInst(voxelPos, VoxelInstance::Type::Fading);
			return (fadingVoxelInst != nullptr) ? fadingVoxelInst->getFadeState().getPercentFaded() : 0.0;
		};

		auto getVoxelOpenDoorPercentOrDefault = [&chunk](const VoxelInt3 &voxelPos)
		{
			const VoxelInstance *openDoorVoxelInst = chunk.tryGetVoxelInst(voxelPos, VoxelInstance::Type::OpenDoor);
			return (openDoorVoxelInst != nullptr) ? openDoorVoxelInst->getDoorState().getPercentOpen() : 0.0;
		};

		// Get the scene graph chunk associated with the world space chunk.
		const auto graphChunkIter = std::find_if(this->graphChunks.begin(), this->graphChunks.end(),
			[&chunkPos](const SceneGraphChunk &graphChunk)
		{
			return graphChunk.position == chunkPos;
		});

		DebugAssertMsg(graphChunkIter != this->graphChunks.end(), "Expected scene graph chunk (" + chunkPos.toString() + ") to have been added.");
		SceneGraphChunk &graphChunk = *graphChunkIter;

		// @todo: these two buffers could probably be removed if SceneGraphVoxel is going to store them instead.
		std::array<RenderTriangle, sgGeometry::MAX_TRIANGLES_PER_VOXEL> opaqueTrianglesBuffer, alphaTestedTrianglesBuffer;
		int opaqueTriangleCount = 0;
		int alphaTestedTriangleCount = 0;

		for (int dirtyVoxelIndex = 0; dirtyVoxelIndex < chunk.getDirtyVoxelCount(); dirtyVoxelIndex++)
		{
			const VoxelInt3 &voxelPos = chunk.getDirtyVoxel(dirtyVoxelIndex);
			const Chunk::VoxelID voxelID = chunk.getVoxel(voxelPos.x, voxelPos.y, voxelPos.z);
			const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);

			opaqueTriangleCount = 0;
			alphaTestedTriangleCount = 0;
			if (voxelDef.type == ArenaTypes::VoxelType::Wall)
			{
				const VoxelDefinition::WallData &wall = voxelDef.wall;
				const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(wall.sideTextureAsset);
				const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(wall.floorTextureAsset);
				const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(wall.ceilingTextureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteWall(chunkPos, voxelPos, ceilingScale, sideMaterialID, floorMaterialID, ceilingMaterialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 12), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
			{
				const VoxelDefinition::FloorData &floor = voxelDef.floor;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(floor.textureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteFloor(chunkPos, voxelPos, ceilingScale, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
			{
				const VoxelDefinition::CeilingData &ceiling = voxelDef.ceiling;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(ceiling.textureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteCeiling(chunkPos, voxelPos, ceilingScale, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
			{
				const VoxelDefinition::RaisedData &raised = voxelDef.raised;
				const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(raised.sideTextureAsset);
				const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(raised.floorTextureAsset);
				const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(raised.ceilingTextureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteRaised(chunkPos, voxelPos, ceilingScale, raised.yOffset, raised.ySize,
					raised.vTop, raised.vBottom, sideMaterialID, floorMaterialID, ceilingMaterialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 4), &opaqueTriangleCount,
					BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 8), &alphaTestedTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
			{
				const VoxelDefinition::DiagonalData &diagonal = voxelDef.diagonal;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(diagonal.textureAsset);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteDiagonal(chunkPos, voxelPos, ceilingScale, diagonal.type1, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 4), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
			{
				const VoxelDefinition::TransparentWallData &transparentWall = voxelDef.transparentWall;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(transparentWall.textureAsset);
				sgGeometry::WriteTransparentWall(chunkPos, voxelPos, ceilingScale, materialID,
					BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 8), &alphaTestedTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
			{
				const VoxelDefinition::EdgeData &edge = voxelDef.edge;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(edge.textureAsset);
				sgGeometry::WriteEdge(chunkPos, voxelPos, ceilingScale, edge.facing, edge.yOffset, edge.flipped, materialID,
					BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 4), &alphaTestedTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
			{
				const VoxelInstance *chasmVoxelInst = chunk.tryGetVoxelInst(voxelPos, VoxelInstance::Type::Chasm);
				bool hasNorthFace = false;
				bool hasSouthFace = false;
				bool hasEastFace = false;
				bool hasWestFace = false;
				if (chasmVoxelInst != nullptr)
				{
					const VoxelInstance::ChasmState &chasmState = chasmVoxelInst->getChasmState();
					hasNorthFace = chasmState.getNorth();
					hasSouthFace = chasmState.getSouth();
					hasEastFace = chasmState.getEast();
					hasWestFace = chasmState.getWest();
				}

				const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
				const bool isDry = chasm.type == ArenaTypes::ChasmType::Dry;
				const ObjectMaterialID floorMaterialID = levelInst.getChasmFloorMaterialID(chasm.type, chasmAnimPercent);
				const ObjectMaterialID sideMaterialID = levelInst.getChasmWallMaterialID(chasm.type, chasmAnimPercent, chasm.textureAsset);
				sgGeometry::WriteChasm(chunkPos, voxelPos, ceilingScale, hasNorthFace, hasSouthFace, hasEastFace, hasWestFace,
					isDry, floorMaterialID, sideMaterialID, BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 10), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Door)
			{
				const VoxelDefinition::DoorData &door = voxelDef.door;
				const double animPercent = getVoxelOpenDoorPercentOrDefault(voxelPos);
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(door.textureAsset);
				sgGeometry::WriteDoor(chunkPos, voxelPos, ceilingScale, door.type, animPercent,
					materialID, BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 16), &alphaTestedTriangleCount);
			}

			SceneGraphVoxel &graphVoxel = graphChunk.voxels.get(voxelPos.x, voxelPos.y, voxelPos.z);
			Buffer<RenderTriangle> &dstOpaqueTriangles = graphVoxel.opaqueTriangles;
			Buffer<RenderTriangle> &dstAlphaTestedTriangles = graphVoxel.alphaTestedTriangles;
			if (opaqueTriangleCount > 0)
			{
				const auto srcStart = opaqueTrianglesBuffer.cbegin();
				const auto srcEnd = srcStart + opaqueTriangleCount;
				dstOpaqueTriangles.init(opaqueTriangleCount);
				std::copy(srcStart, srcEnd, dstOpaqueTriangles.get());
			}
			else if ((opaqueTriangleCount == 0) && (dstOpaqueTriangles.getCount() > 0))
			{
				dstOpaqueTriangles.clear();
			}

			if (alphaTestedTriangleCount > 0)
			{
				const auto srcStart = alphaTestedTrianglesBuffer.cbegin();
				const auto srcEnd = srcStart + alphaTestedTriangleCount;
				dstAlphaTestedTriangles.init(alphaTestedTriangleCount);
				std::copy(srcStart, srcEnd, dstAlphaTestedTriangles.get());
			}
			else if ((alphaTestedTriangleCount == 0) && (dstAlphaTestedTriangles.getCount() > 0))
			{
				dstAlphaTestedTriangles.clear();
			}
		}
	}

	// Regenerate draw lists.
	// @todo: maybe this is where we need to call the voxel animation logic functions so we know what material ID
	// to use for chasms, etc.? Might be good to finally bring in the VoxelRenderDefinition, etc..
	for (int i = 0; i < static_cast<int>(this->graphChunks.size()); i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const ChunkInt2 &chunkPos = chunk.getPosition();
		const ChunkInt2 relativeChunkPos = chunkPos - camera.chunk; // Relative to camera chunk.
		constexpr double chunkDimReal = static_cast<double>(ChunkUtils::CHUNK_DIM);

		// Top right and bottom left world space corners of this chunk.
		const NewDouble2 chunkTR2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, VoxelDouble2::Zero);
		const NewDouble2 chunkBL2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, VoxelDouble2(chunkDimReal, chunkDimReal));

		// See if this chunk's geometry should reach the draw list.
		const bool isChunkVisible = MathUtils::triangleRectangleIntersection(
			cameraEye2D, cameraFrustumRightPoint2D, cameraFrustumLeftPoint2D, chunkTR2D, chunkBL2D);

		if (!isChunkVisible)
		{
			continue;
		}

		const SceneGraphChunk &graphChunk = this->graphChunks[i];
		const Buffer3D<SceneGraphVoxel> &graphChunkVoxels = graphChunk.voxels;

		for (WEInt z = 0; z < graphChunkVoxels.getDepth(); z++)
		{
			for (SNInt x = 0; x < graphChunkVoxels.getWidth(); x++)
			{
				const VoxelInt2 voxelColumnPos(x, z);
				const VoxelDouble2 voxelColumnPoint(
					static_cast<SNDouble>(voxelColumnPos.x),
					static_cast<WEDouble>(voxelColumnPos.y));
				const NewDouble2 voxelTR2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, voxelColumnPoint);
				const NewDouble2 voxelBL2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, voxelColumnPoint + VoxelDouble2(1.0, 1.0));

				// See if this voxel's geometry should reach the draw list.
				// @todo: the 2D camera triangle here is not correct when looking up or down, currently results in missing triangles on-screen; need a larger triangle based on the angle to compensate.
				// @todo: replace this per-voxel-column operation with a quadtree look-up that can do large groups of voxel columns at once
				const bool isVoxelColumnVisible = MathUtils::triangleRectangleIntersection(
					cameraEye2D, cameraFrustumRightPoint2D, cameraFrustumLeftPoint2D, voxelTR2D, voxelBL2D);

				if (!isVoxelColumnVisible)
				{
					continue;
				}

				for (int y = 0; y < graphChunkVoxels.getHeight(); y++)
				{
					const SceneGraphVoxel &graphVoxel = graphChunkVoxels.get(x, y, z);
					const Buffer<RenderTriangle> &srcOpaqueTriangles = graphVoxel.opaqueTriangles;
					const Buffer<RenderTriangle> &srcAlphaTestedTriangles = graphVoxel.alphaTestedTriangles;
					const int srcOpaqueTriangleCount = srcOpaqueTriangles.getCount();
					const int srcAlphaTestedTriangleCount = srcAlphaTestedTriangles.getCount();
					if (srcOpaqueTriangleCount > 0)
					{
						const RenderTriangle *srcStart = srcOpaqueTriangles.get();
						const RenderTriangle *srcEnd = srcOpaqueTriangles.end();
						this->opaqueVoxelTriangles.insert(this->opaqueVoxelTriangles.end(), srcStart, srcEnd);
					}

					if (srcAlphaTestedTriangleCount > 0)
					{
						const RenderTriangle *srcStart = srcAlphaTestedTriangles.get();
						const RenderTriangle *srcEnd = srcAlphaTestedTriangles.end();
						this->alphaTestedVoxelTriangles.insert(this->alphaTestedVoxelTriangles.end(), srcStart, srcEnd);
					}
				}
			}
		}
	}

	// @todo: sort opaque chunk geometry near to far
	// @todo: sort alpha-tested chunk geometry far to near
	// ^ for both of these, the goal is so we can essentially just memcpy each chunk's geometry into the scene graph's draw lists.
}

void SceneGraph::updateEntities(const LevelInstance &levelInst, const RenderCamera &camera,
	const EntityDefinitionLibrary &entityDefLibrary, bool nightLightsAreActive, bool playerHasLight,
	RendererSystem3D &renderer)
{
	DebugNotImplemented();
	/*const ChunkManager &chunkManager = levelInst.getChunkManager();
	const int chunkCount = chunkManager.getChunkCount();

	const EntityManager &entityManager = levelInst.getEntityManager();
	std::vector<const Entity*> entityPtrs;

	const CoordDouble2 cameraPos2D(camera.chunk, VoxelDouble2(camera.point.x, camera.point.z));
	const VoxelDouble3 entityDir = -camera.forward;

	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const ChunkInt2 &chunkPosition = chunk.getPosition();
		const int entityCountInChunk = entityManager.getCountInChunk(chunkPosition);
		entityPtrs.resize(entityCountInChunk);
		const int writtenEntityCount = entityManager.getEntitiesInChunk(
			chunkPosition, entityPtrs.data(), static_cast<int>(entityPtrs.size()));
		DebugAssert(writtenEntityCount == entityCountInChunk);

		for (const Entity *entityPtr : entityPtrs)
		{
			if (entityPtr != nullptr)
			{
				const Entity &entity = *entityPtr;
				const CoordDouble2 &entityCoord = entity.getPosition();
				const EntityDefID entityDefID = entity.getDefinitionID();
				const EntityDefinition &entityDef = entityManager.getEntityDef(entityDefID, entityDefLibrary);
				const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
				//const EntityAnimationInstance &animInst = entity.getAnimInstance();

				EntityVisibilityState3D visState;
				entityManager.getEntityVisibilityState3D(entity, cameraPos2D, ceilingScale, chunkManager, entityDefLibrary, visState);
				const EntityAnimationDefinition::State &animState = animDef.getState(visState.stateIndex);
				const EntityAnimationDefinition::KeyframeList &animKeyframeList = animState.getKeyframeList(visState.angleIndex);
				const EntityAnimationDefinition::Keyframe &animKeyframe = animKeyframeList.getKeyframe(visState.keyframeIndex);
				const TextureAsset &textureAsset = animKeyframe.getTextureAsset();
				const bool flipped = animKeyframeList.isFlipped();
				const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && (entityDef.getDoodad().puddle);
				const ObjectMaterialID materialID = levelInst.getEntityMaterialID(textureAsset, flipped, reflective);

				std::array<RenderTriangle, 2> entityTrianglesBuffer;
				sgGeometry::WriteEntity(visState.flatPosition.chunk, visState.flatPosition.point, materialID,
					animKeyframe.getWidth(), animKeyframe.getHeight(), entityDir,
					BufferView<RenderTriangle>(entityTrianglesBuffer.data(), static_cast<int>(entityTrianglesBuffer.size())));

				const auto srcStart = entityTrianglesBuffer.cbegin();
				const auto srcEnd = srcStart + 2;
				this->entityTriangles.insert(this->entityTriangles.end(), srcStart, srcEnd);
			}
		}
	}
}

void SceneGraph::updateSky(const SkyInstance &skyInst, double daytimePercent, double latitude)
{
	//this->clearSky();
	DebugNotImplemented();
}

void SceneGraph::updateWeather(const SkyInstance &skyInst)
{
	// @todo
	DebugNotImplemented();
}

/*void SceneGraph::updateScene(const LevelInstance &levelInst, const SkyInstance &skyInst,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
	double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
	double daytimePercent, double latitude, const EntityDefinitionLibrary &entityDefLibrary,
	TextureManager &textureManager, RendererSystem3D &renderer)
{
	// @todo: update chunks first so we know which chunks need to be fully loaded in with loadVoxels(), etc..
	DebugNotImplemented();

	this->updateVoxels(levelInst, camera, chasmAnimPercent, nightLightsAreActive, renderer);
	this->updateEntities(levelInst, camera, entityDefLibrary, nightLightsAreActive, playerHasLight, renderer);
	this->updateSky(skyInst, daytimePercent, latitude);
	this->updateWeather(skyInst);
}*/
