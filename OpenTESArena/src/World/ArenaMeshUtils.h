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
#include "components/utilities/Span.h"

enum class VoxelFacing2D;
enum class VoxelFacing3D;

using ArenaChasmWallIndexBuffer = std::array<int32_t, MeshUtils::INDICES_PER_QUAD>; // Two triangles per buffer.

struct ArenaShapeRaisedInitInfo
{
	double vTop;
	double vBottom;

	ArenaShapeRaisedInitInfo();
};

struct ArenaShapeEdgeInitInfo
{
	VoxelFacing2D facing;
	bool flippedTexCoords;

	ArenaShapeEdgeInitInfo();
};

struct ArenaShapeDiagonalInitInfo
{
	bool isRightDiagonal;

	ArenaShapeDiagonalInitInfo();
};

// Provides init values for physics shape and for transforming the render mesh, comes from map generation.
struct ArenaShapeInitCache
{
	// Simplified box shape values.
	double boxWidth;
	double boxHeight;
	double boxDepth;
	double boxYOffset;
	Radians boxYRotation;

	ArenaVoxelType voxelType;
	union
	{
		ArenaShapeRaisedInitInfo raised;
		ArenaShapeEdgeInitInfo edge;
		ArenaShapeDiagonalInitInfo diagonal;
	};

	ArenaShapeInitCache();

	void initDefaultBoxValues(ArenaVoxelType voxelType);
	void initRaisedBoxValues(double yOffset, double ySize, double vTop, double vBottom);
	void initEdgeBoxValues(double yOffset, VoxelFacing2D facing, bool flippedTexCoords);
	void initChasmBoxValues(bool isDryChasm);
	void initDiagonalBoxValues(bool isRightDiagonal);
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
			return false;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Chasm:
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
		case ArenaVoxelType::Door:
		case ArenaVoxelType::Diagonal:
			return false;
		case ArenaVoxelType::Wall:
		case ArenaVoxelType::Floor:
		case ArenaVoxelType::Ceiling:
		case ArenaVoxelType::Raised:
		case ArenaVoxelType::TransparentWall:
		case ArenaVoxelType::Edge:
		case ArenaVoxelType::Chasm:
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

	bool isFullyCoveringFacing(const ArenaShapeInitCache &shapeInitCache, VoxelFacing3D facing);

	void writeChasmWallRendererIndexBuffers(ArenaChasmWallIndexBuffer *outNorthIndices, ArenaChasmWallIndexBuffer *outEastIndices, ArenaChasmWallIndexBuffer *outSouthIndices, ArenaChasmWallIndexBuffer *outWestIndices);

}

#endif
