#include <algorithm>

#include "ArenaVoxelUtils.h"
#include "VoxelDefinition.h"
#include "VoxelFacing2D.h"
#include "VoxelGeometry.h"
#include "VoxelInstance.h"
#include "../Assets/ArenaTypes.h"
#include "../Math/Quad.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"

namespace
{
	// Side length vectors used by voxel geometry functions.
	Double3 GetXVec()
	{
		return Double3::UnitX;
	}

	Double3 GetYVec(double scale)
	{
		return Double3::UnitY * scale;
	}

	Double3 GetZVec()
	{
		return Double3::UnitZ;
	}

	// Voxel geometry generation functions. Generally low X face first, then high X, etc..
	void GenerateWall(const VoxelDefinition::WallData &wall, const Double3 &origin, double ceilingHeight,
		BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 6);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(ceilingHeight);
		const Double3 zVec = GetZVec();

		// Near X
		const Quad face0(
			origin,
			origin + zVec,
			origin + yVec + zVec);

		// Far X
		const Quad face1(
			origin + xVec + zVec,
			origin + xVec,
			origin + xVec + yVec);

		// Near Y
		const Quad face2(
			origin + xVec,
			origin + xVec + zVec,
			origin + zVec);

		// Far Y
		const Quad face3(
			origin + yVec,
			origin + yVec + zVec,
			origin + xVec + yVec + zVec);

		// Near Z
		const Quad face4(
			origin + xVec,
			origin,
			origin + yVec);

		// Far Z
		const Quad face5(
			origin + zVec,
			origin + xVec + zVec,
			origin + xVec + yVec + zVec);

		outView.set(0, face0);
		outView.set(1, face1);
		outView.set(2, face2);
		outView.set(3, face3);
		outView.set(4, face4);
		outView.set(5, face5);
	}

	void GenerateFloor(const VoxelDefinition::FloorData &floor, const Double3 &origin,
		double ceilingHeight, BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 1);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(ceilingHeight);
		const Double3 zVec = GetZVec();

		// Far Y
		const Quad face(
			origin + yVec,
			origin + yVec + zVec,
			origin + xVec + yVec + zVec);

		outView.set(0, face);
	}

	void GenerateCeiling(const VoxelDefinition::CeilingData &ceiling, const Double3 &origin,
		double ceilingHeight, BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 1);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(ceilingHeight);
		const Double3 zVec = GetZVec();

		// Near Y
		const Quad face(
			origin + xVec,
			origin + xVec + zVec,
			origin + zVec);

		outView.set(0, face);
	}

	void GenerateRaised(const VoxelDefinition::RaisedData &raised, const Double3 &origin,
		double ceilingHeight, BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 6);

		const Double3 raisedOrigin = origin + Double3(0.0, raised.yOffset * ceilingHeight, 0.0);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(raised.ySize * ceilingHeight);
		const Double3 zVec = GetZVec();

		// Near X
		const Quad face0(
			raisedOrigin,
			raisedOrigin + zVec,
			raisedOrigin + yVec + zVec);

		// Far X
		const Quad face1(
			raisedOrigin + xVec + zVec,
			raisedOrigin + xVec,
			raisedOrigin + xVec + yVec);

		// Near Y
		const Quad face2(
			raisedOrigin + xVec,
			raisedOrigin + xVec + zVec,
			raisedOrigin + zVec);

		// Far Y
		const Quad face3(
			raisedOrigin + yVec,
			raisedOrigin + yVec + zVec,
			raisedOrigin + xVec + yVec + zVec);

		// Near Z
		const Quad face4(
			raisedOrigin + xVec,
			raisedOrigin,
			raisedOrigin + yVec);

		// Far Z
		const Quad face5(
			raisedOrigin + zVec,
			raisedOrigin + xVec + zVec,
			raisedOrigin + xVec + yVec + zVec);

		outView.set(0, face0);
		outView.set(1, face1);
		outView.set(2, face2);
		outView.set(3, face3);
		outView.set(4, face4);
		outView.set(5, face5);
	}

	void GenerateDiagonal(const VoxelDefinition::DiagonalData &diagonal, const Double3 &origin,
		double ceilingHeight, BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 1);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(ceilingHeight);
		const Double3 zVec = GetZVec();

		// Diagonal orientation depends on type.
		const Quad face = [&diagonal, &origin, ceilingHeight, &xVec, &yVec, &zVec]()
		{
			if (diagonal.type1)
			{
				// (NearX, NearZ) to (FarX, FarZ)
				return Quad(
					origin,
					origin + xVec + zVec,
					origin + xVec + yVec + zVec);
			}
			else
			{
				// (FarX, NearZ) to (NearX, FarZ)
				return Quad(
					origin + xVec,
					origin + zVec,
					origin + yVec + zVec);
			}
		}();

		outView.set(0, face);
	}

	void GenerateTransparentWall(const VoxelDefinition::TransparentWallData &transparent,
		const Double3 &origin, double ceilingHeight, BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 4);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(ceilingHeight);
		const Double3 zVec = GetZVec();

		// Near X
		const Quad face0(
			origin,
			origin + zVec,
			origin + yVec + zVec);

		// Far X
		const Quad face1(
			origin + xVec + zVec,
			origin + xVec,
			origin + xVec + yVec);

		// Near Z
		const Quad face2(
			origin + xVec,
			origin,
			origin + yVec);

		// Far Z
		const Quad face3(
			origin + zVec,
			origin + xVec + zVec,
			origin + xVec + yVec + zVec);

		outView.set(0, face0);
		outView.set(1, face1);
		outView.set(2, face2);
		outView.set(3, face3);
	}

	void GenerateEdge(const VoxelDefinition::EdgeData &edge, const Double3 &origin,
		double ceilingHeight, BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 1);

		const Double3 edgeOrigin = origin + Double3(0.0, edge.yOffset * ceilingHeight, 0.0);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(ceilingHeight);
		const Double3 zVec = GetZVec();

		const Quad face = [&edge, &edgeOrigin, &xVec, &yVec, &zVec]()
		{
			// Geometry depends on orientation.
			if (edge.facing == VoxelFacing2D::PositiveX)
			{
				return Quad(
					edgeOrigin + xVec + zVec,
					edgeOrigin + xVec,
					edgeOrigin + xVec + yVec);
			}
			else if (edge.facing == VoxelFacing2D::NegativeX)
			{
				return Quad(
					edgeOrigin,
					edgeOrigin + zVec,
					edgeOrigin + yVec + zVec);
			}
			else if (edge.facing == VoxelFacing2D::PositiveZ)
			{
				return Quad(
					edgeOrigin + zVec,
					edgeOrigin + xVec + zVec,
					edgeOrigin + xVec + yVec + zVec);
			}
			else
			{
				DebugAssert(edge.facing == VoxelFacing2D::NegativeZ);
				return Quad(
					edgeOrigin + xVec,
					edgeOrigin,
					edgeOrigin + yVec);
			}
		}();

		outView.set(0, face);
	}

	void GenerateChasm(const VoxelDefinition::ChasmData &chasm, const Double3 &origin,
		double ceilingHeight, const VoxelInstance::ChasmState *chasmState, BufferView<Quad> &outView)
	{
		// Depends on number of faces and chasm type.
		const int faceCount = outView.getCount();
		DebugAssert(faceCount > 0);

		const double chasmDepth = (chasm.type == ArenaTypes::ChasmType::Dry) ?
			ceilingHeight : ArenaVoxelUtils::WET_CHASM_DEPTH;
		const Double3 chasmOrigin = origin + Double3(0.0, ceilingHeight - chasmDepth, 0.0);

		const bool hasNorthFace = (chasmState != nullptr) && chasmState->getNorth();
		const bool hasEastFace = (chasmState != nullptr) && chasmState->getEast();
		const bool hasSouthFace = (chasmState != nullptr) && chasmState->getSouth();
		const bool hasWestFace = (chasmState != nullptr) && chasmState->getWest();

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(chasmDepth);
		const Double3 zVec = GetZVec();

		// Floor
		const Quad face0(
			chasmOrigin + xVec,
			chasmOrigin + xVec + zVec,
			chasmOrigin + zVec);

		outView.set(0, face0);

		// Near X
		int faceIndex = 1;
		if (hasSouthFace)
		{
			const Quad face(
				chasmOrigin,
				chasmOrigin + zVec,
				chasmOrigin + yVec + zVec);

			outView.set(faceIndex, face);
			faceIndex++;
		}

		// Far X
		if (hasNorthFace)
		{
			const Quad face(
				chasmOrigin + xVec + zVec,
				chasmOrigin + xVec,
				chasmOrigin + xVec + yVec);

			outView.set(faceIndex, face);
			faceIndex++;
		}

		// Near Z
		if (hasWestFace)
		{
			const Quad face(
				chasmOrigin + xVec,
				chasmOrigin,
				chasmOrigin + yVec);

			outView.set(faceIndex, face);
			faceIndex++;
		}

		// Far Z
		if (hasEastFace)
		{
			const Quad face(
				chasmOrigin + zVec,
				chasmOrigin + xVec + zVec,
				chasmOrigin + xVec + yVec + zVec);

			outView.set(faceIndex, face);
			faceIndex++;
		}

		DebugAssert(faceIndex == outView.getCount());
	}

	void GenerateDoor(const VoxelDefinition::DoorData &door, const Double3 &origin,
		double ceilingHeight, BufferView<Quad> &outView)
	{
		DebugAssert(outView.getCount() == 4);

		const Double3 xVec = GetXVec();
		const Double3 yVec = GetYVec(ceilingHeight);
		const Double3 zVec = GetZVec();

		// Near X
		const Quad face0(
			origin,
			origin + zVec,
			origin + yVec + zVec);

		// Far X
		const Quad face1(
			origin + xVec + zVec,
			origin + xVec,
			origin + xVec + yVec);

		// Near Z
		const Quad face2(
			origin + xVec,
			origin,
			origin + yVec);

		// Far Z
		const Quad face3(
			origin + zVec,
			origin + xVec + zVec,
			origin + xVec + yVec + zVec);

		outView.set(0, face0);
		outView.set(1, face1);
		outView.set(2, face2);
		outView.set(3, face3);
	}
}

void VoxelGeometry::getInfo(const VoxelDefinition &voxelDef, const VoxelInstance *voxelInst, int *outQuadCount)
{
	auto maybeWrite = [outQuadCount](int quadCount)
	{
		if (outQuadCount != nullptr)
		{
			*outQuadCount = quadCount;
		}
	};

	switch (voxelDef.type)
	{
	case ArenaTypes::VoxelType::None:
		maybeWrite(0);
		break;
	case ArenaTypes::VoxelType::Wall:
		maybeWrite(6);
		break;
	case ArenaTypes::VoxelType::Floor:
		maybeWrite(1);
		break;
	case ArenaTypes::VoxelType::Ceiling:
		maybeWrite(1);
		break;
	case ArenaTypes::VoxelType::Raised:
		maybeWrite(6);
		break;
	case ArenaTypes::VoxelType::Diagonal:
		maybeWrite(1);
		break;
	case ArenaTypes::VoxelType::TransparentWall:
		maybeWrite(4);
		break;
	case ArenaTypes::VoxelType::Edge:
		maybeWrite(1);
		break;
	case ArenaTypes::VoxelType::Chasm:
	{
		// Chasm geometry depends on visible face count.
		const int faceCount = [voxelInst]()
		{
			if (voxelInst != nullptr)
			{
				DebugAssert(voxelInst->getType() == VoxelInstance::Type::Chasm);
				const VoxelInstance::ChasmState &chasmState = voxelInst->getChasmState();
				return chasmState.getFaceCount();
			}
			else
			{
				// One for floor.
				return 1;
			}
		}();

		maybeWrite(faceCount);
		break;
	}
	case ArenaTypes::VoxelType::Door:
		// Doors are an unusual case. Just pretend they're closed here.
		maybeWrite(4);
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelDef.type)));
		break;
	}
}

int VoxelGeometry::getQuads(const VoxelDefinition &voxelDef, const NewInt3 &voxel, double ceilingHeight,
	const VoxelInstance *voxelInst, Quad *outQuads, int bufferSize)
{
	if ((outQuads == nullptr) || (bufferSize <= 0))
	{
		return 0;
	}

	int quadCount;
	VoxelGeometry::getInfo(voxelDef, voxelInst, &quadCount);

	// If there's nothing to write, or all the geometry data can't fit in the output buffer,
	// then return failure.
	const bool hasEnoughBufferSize = bufferSize >= quadCount;
	if ((quadCount == 0) || !hasEnoughBufferSize)
	{
		return 0;
	}

	BufferView<Quad> quadView(outQuads, quadCount);
	const Double3 origin(
		static_cast<double>(voxel.x),
		static_cast<double>(voxel.y) * ceilingHeight,
		static_cast<double>(voxel.z));

	switch (voxelDef.type)
	{
	case ArenaTypes::VoxelType::None:
		break;
	case ArenaTypes::VoxelType::Wall:
		GenerateWall(voxelDef.wall, origin, ceilingHeight, quadView);
		break;
	case ArenaTypes::VoxelType::Floor:
		GenerateFloor(voxelDef.floor, origin, ceilingHeight, quadView);
		break;
	case ArenaTypes::VoxelType::Ceiling:
		GenerateCeiling(voxelDef.ceiling, origin, ceilingHeight, quadView);
		break;
	case ArenaTypes::VoxelType::Raised:
		GenerateRaised(voxelDef.raised, origin, ceilingHeight, quadView);
		break;
	case ArenaTypes::VoxelType::Diagonal:
		GenerateDiagonal(voxelDef.diagonal, origin, ceilingHeight, quadView);
		break;
	case ArenaTypes::VoxelType::TransparentWall:
		GenerateTransparentWall(voxelDef.transparentWall, origin, ceilingHeight, quadView);
		break;
	case ArenaTypes::VoxelType::Edge:
		GenerateEdge(voxelDef.edge, origin, ceilingHeight, quadView);
		break;
	case ArenaTypes::VoxelType::Chasm:
	{
		const VoxelInstance::ChasmState *chasmState = [voxelInst]() -> const VoxelInstance::ChasmState*
		{
			if ((voxelInst != nullptr) && (voxelInst->getType() == VoxelInstance::Type::Chasm))
			{
				return &voxelInst->getChasmState();
			}
			
			return nullptr;
		}();

		GenerateChasm(voxelDef.chasm, origin, ceilingHeight, chasmState, quadView);
		break;
	}
	case ArenaTypes::VoxelType::Door:
		GenerateDoor(voxelDef.door, origin, ceilingHeight, quadView);
		break;
	default:
		break;
	}

	return quadCount;
}
