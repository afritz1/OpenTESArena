#include <numeric>
#include <optional>

#include "SceneGraph.h"
#include "../RenderCamera.h"
#include "../../Assets/MIFUtils.h"
#include "../../Entities/EntityManager.h"
#include "../../Entities/EntityVisibilityState.h"
#include "../../World/ChunkManager.h"
#include "../../World/LevelInstance.h"
#include "../../World/VoxelFacing2D.h"
#include "../../World/VoxelGeometry.h"

#include "components/debug/Debug.h"

namespace sgGeometry
{
	constexpr int MAX_TRIANGLES_PER_VOXEL = 16;

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

	// Translates a model space triangle into world space and writes it into the output buffer at the given index.
	void WriteTriangle(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double2 &uv0, const Double2 &uv1,
		const Double2 &uv2, ObjectMaterialID materialID, const Double3 &voxelPosition, double ceilingScale, int index,
		BufferView<RenderTriangle> &outTriangles)
	{
		Double3 worldV0, worldV1, worldV2;
		MakeWorldSpaceVertices(voxelPosition, v0, v1, v2, ceilingScale, &worldV0, &worldV1, &worldV2);

		RenderTriangle &triangle = outTriangles.get(index);
		triangle.init(worldV0, worldV1, worldV2, uv0, uv1, uv2, materialID);
	}

	// Geometry generation functions (currently in world space, might be chunk space or something else later).
	// @todo: these functions should not assume opaque/alpha-tested per face, they should query the texture associated
	// with each face for that, and then write to the appropriate out buffer.
	void WriteWall(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID sideMaterialID,
		ObjectMaterialID floorMaterialID, ObjectMaterialID ceilingMaterialID, BufferView<RenderTriangle> &outOpaqueTriangles,
		int *outOpaqueTriangleCount)
	{
		constexpr int triangleCount = 12;
		DebugAssert(outOpaqueTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// X=0
		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, 1, outOpaqueTriangles);
		// X=1
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, 2, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, 3, outOpaqueTriangles);
		// Y=0
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, floorMaterialID, voxelPosition, ceilingScale, 4, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(1.0, 0.0, 1.0), UV_BR, UV_TR, UV_TL, floorMaterialID, voxelPosition, ceilingScale, 5, outOpaqueTriangles);
		// Y=1
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), UV_TL, UV_BL, UV_BR, ceilingMaterialID, voxelPosition, ceilingScale, 6, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, ceilingMaterialID, voxelPosition, ceilingScale, 7, outOpaqueTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, 8, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, 9, outOpaqueTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, 10, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, 11, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteFloor(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID materialID,
		BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount)
	{
		constexpr int triangleCount = 2;
		DebugAssert(outOpaqueTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 1, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteCeiling(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID materialID,
		BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount)
	{
		constexpr int triangleCount = 2;
		DebugAssert(outOpaqueTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(1.0, 0.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 1, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteRaised(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, double yOffset, double ySize,
		double vTop, double vBottom, ObjectMaterialID sideMaterialID, ObjectMaterialID floorMaterialID, ObjectMaterialID ceilingMaterialID,
		BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount,
		BufferView<RenderTriangle> &outAlphaTestedTriangles, int *outAlphaTestedTriangleCount)
	{
		constexpr int opaqueTriangleCount = 4;
		constexpr int alphaTestedTriangleCount = 8;
		DebugAssert(outOpaqueTriangles.getCount() == opaqueTriangleCount);
		DebugAssert(outAlphaTestedTriangles.getCount() == alphaTestedTriangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		const double yBottom = yOffset;
		const double yTop = yOffset + ySize;

		const Double2 sideUvTL(0.0, vTop);
		const Double2 sideUvTR(1.0, vTop);
		const Double2 sideUvBL(0.0, vBottom);
		const Double2 sideUvBR(1.0, vBottom);

		// Opaque
		// Y=bottom
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(0.0, yBottom, 1.0), Double3(0.0, yBottom, 0.0), UV_TL, UV_BL, UV_BR, floorMaterialID, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(1.0, yBottom, 0.0), Double3(1.0, yBottom, 1.0), UV_BR, UV_TR, UV_TL, floorMaterialID, voxelPosition, ceilingScale, 1, outOpaqueTriangles);
		// Y=top
		WriteTriangle(Double3(1.0, yTop, 0.0), Double3(0.0, yTop, 0.0), Double3(0.0, yTop, 1.0), UV_TL, UV_BL, UV_BR, ceilingMaterialID, voxelPosition, ceilingScale, 2, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, yTop, 1.0), Double3(1.0, yTop, 1.0), Double3(1.0, yTop, 0.0), UV_BR, UV_TR, UV_TL, ceilingMaterialID, voxelPosition, ceilingScale, 3, outOpaqueTriangles);

		// Alpha-tested
		// X=0
		WriteTriangle(Double3(0.0, yTop, 0.0), Double3(0.0, yBottom, 0.0), Double3(0.0, yBottom, 1.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, yBottom, 1.0), Double3(0.0, yTop, 1.0), Double3(0.0, yTop, 0.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
		// X=1
		WriteTriangle(Double3(1.0, yTop, 1.0), Double3(1.0, yBottom, 1.0), Double3(1.0, yBottom, 0.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, yBottom, 0.0), Double3(1.0, yTop, 0.0), Double3(1.0, yTop, 1.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, yTop, 0.0), Double3(1.0, yBottom, 0.0), Double3(0.0, yBottom, 0.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(0.0, yTop, 0.0), Double3(1.0, yTop, 0.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, yTop, 1.0), Double3(0.0, yBottom, 1.0), Double3(1.0, yBottom, 1.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(1.0, yTop, 1.0), Double3(0.0, yTop, 1.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);

		*outOpaqueTriangleCount = opaqueTriangleCount;
		*outAlphaTestedTriangleCount = alphaTestedTriangleCount;
	}

	void WriteDiagonal(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, bool flipped, ObjectMaterialID materialID,
		BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount)
	{
		constexpr int triangleCount = 4; // @todo: might change to 2 in the future if a back-face culling bool is added.
		DebugAssert(outOpaqueTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		double startX, endX;
		double startZ, endZ;
		if (flipped)
		{
			startX = 0.0;
			endX = 1.0;
			startZ = 0.0;
			endZ = 1.0;
		}
		else
		{
			startX = 0.0;
			endX = 1.0;
			startZ = 1.0;
			endZ = 0.0;
		}

		// Front side
		WriteTriangle(Double3(startX, 1.0, startZ), Double3(startX, 0.0, startZ), Double3(endX, 0.0, endZ), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(endX, 0.0, endZ), Double3(endX, 1.0, endZ), Double3(startX, 1.0, startZ), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 1, outOpaqueTriangles);
		// Back side
		WriteTriangle(Double3(endX, 1.0, endZ), Double3(endX, 0.0, endZ), Double3(startX, 0.0, startZ), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 2, outOpaqueTriangles);
		WriteTriangle(Double3(startX, 0.0, startZ), Double3(startX, 1.0, startZ), Double3(endX, 1.0, endZ), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 3, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteTransparentWall(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID materialID,
		BufferView<RenderTriangle> &outAlphaTestedTriangles, int *outAlphaTestedTriangleCount)
	{
		constexpr int triangleCount = 8;
		DebugAssert(outAlphaTestedTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// X=0
		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
		// X=1
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);

		*outAlphaTestedTriangleCount = triangleCount;
	}

	void WriteEdge(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, VoxelFacing2D facing, double yOffset,
		bool flipped, ObjectMaterialID materialID, BufferView<RenderTriangle> &outAlphaTestedTriangles, int *outAlphaTestedTriangleCount)
	{
		constexpr int triangleCount = 4; // @todo: might change to 2 in the future if a back-face culling bool is added.
		DebugAssert(outAlphaTestedTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		double startX, endX;
		double startZ, endZ;
		switch (facing)
		{
		case VoxelFacing2D::PositiveX:
			startX = 1.0;
			endX = 1.0;
			startZ = 1.0;
			endZ = 0.0;
			break;
		case VoxelFacing2D::NegativeX:
			startX = 0.0;
			endX = 0.0;
			startZ = 0.0;
			endZ = 1.0;
			break;
		case VoxelFacing2D::PositiveZ:
			startX = 0.0;
			endX = 1.0;
			startZ = 1.0;
			endZ = 1.0;
			break;
		case VoxelFacing2D::NegativeZ:
			startX = 1.0;
			endX = 0.0;
			startZ = 0.0;
			endZ = 0.0;
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(facing)));
		}

		Double2 frontUvTL, frontUvTR, frontUvBL, frontUvBR;
		if (!flipped)
		{
			frontUvTL = UV_TL;
			frontUvTR = UV_TR;
			frontUvBL = UV_BL;
			frontUvBR = UV_BR;
		}
		else
		{
			frontUvTL = UV_TR;
			frontUvTR = UV_TL;
			frontUvBL = UV_BR;
			frontUvBR = UV_BL;
		}

		const Double2 backUvTL = frontUvTR;
		const Double2 backUvTR = frontUvTL;
		const Double2 backUvBL = frontUvBR;
		const Double2 backUvBR = frontUvBL;

		const double yBottom = yOffset;
		const double yTop = yBottom + 1.0;

		// Front side
		WriteTriangle(Double3(startX, yTop, startZ), Double3(startX, yBottom, startZ), Double3(endX, yBottom, endZ), frontUvTL, frontUvBL, frontUvBR, materialID, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
		WriteTriangle(Double3(endX, yBottom, endZ), Double3(endX, yTop, endZ), Double3(startX, yTop, startZ), frontUvBR, frontUvTR, frontUvTL, materialID, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
		// Back side
		WriteTriangle(Double3(endX, yTop, endZ), Double3(endX, yBottom, endZ), Double3(startX, yBottom, startZ), backUvTL, backUvBL, backUvBR, materialID, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
		WriteTriangle(Double3(startX, yBottom, startZ), Double3(startX, yTop, startZ), Double3(endX, yTop, endZ), backUvBR, backUvTR, backUvTL, materialID, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);

		*outAlphaTestedTriangleCount = triangleCount;
	}

	// @todo: chasm walls effectively have two textures; will need to work with that and have a dedicated pixel shader that takes two textures.
	void WriteChasm(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, bool north, bool south, bool east, bool west,
		bool isDry, ObjectMaterialID floorMaterialID, ObjectMaterialID sideMaterialID, BufferView<RenderTriangle> &outOpaqueTriangles,
		int *outOpaqueTriangleCount)
	{
		auto countFace = [](bool faceExists)
		{
			return faceExists ? 2 : 0;
		};

		// Variable number of walls based on the chasm instance.
		const int triangleCount = 2 + countFace(north) + countFace(south) + countFace(east) + countFace(west);
		DebugAssert(outOpaqueTriangles.getCount() >= triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// @todo: Water and lava chasms should not be scaled by ceilingScale.
		const double yBottom = 0.0; //isDry ? 0.0 : (1.0 - (1.0 / ceilingScale)); // @todo: verify math

		// Y=bottom (always present)
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(0.0, yBottom, 1.0), Double3(1.0, yBottom, 1.0), UV_TL, UV_BL, UV_BR, floorMaterialID, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(1.0, yBottom, 0.0), Double3(0.0, yBottom, 0.0), UV_BR, UV_TR, UV_TL, floorMaterialID, voxelPosition, ceilingScale, 1, outOpaqueTriangles);

		int triangleIndex = 2;
		if (north)
		{
			// X=0
			WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, yBottom, 1.0), Double3(0.0, yBottom, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		if (south)
		{
			// X=1
			WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, yBottom, 0.0), Double3(1.0, yBottom, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		if (east)
		{
			// Z=0
			WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, yBottom, 0.0), Double3(1.0, yBottom, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(1.0, yBottom, 0.0), Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		if (west)
		{
			// Z=1
			WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, yBottom, 1.0), Double3(0.0, yBottom, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(0.0, yBottom, 1.0), Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		*outOpaqueTriangleCount = triangleIndex;
	}

	void WriteDoor(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ArenaTypes::DoorType doorType,
		double animPercent, ObjectMaterialID materialID, BufferView<RenderTriangle> &outAlphaTestedTriangles,
		int *outAlphaTestedTriangleCount)
	{
		const int triangleCount = [doorType]()
		{
			switch (doorType)
			{
			case ArenaTypes::DoorType::Swinging:
				return 8;
			case ArenaTypes::DoorType::Sliding:
				return 8;
			case ArenaTypes::DoorType::Raising:
				return 8;
			case ArenaTypes::DoorType::Splitting:
				return 16;
			default:
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(doorType)));
			}
		}();

		DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// @todo: transform vertices by anim percent; each door type has its own transform behavior
		// X=0
		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
		// X=1
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);

		*outAlphaTestedTriangleCount = triangleCount;
	}

	void WriteEntity(const ChunkInt2 &chunk, const VoxelDouble3 &point, ObjectMaterialID materialID,
		double width, double height, const Double3 &entityForward, BufferView<RenderTriangle> &outEntityTriangles)
	{
		DebugAssert(outEntityTriangles.getCount() == 2);

		const Double3 entityPosition = MakeEntityPosition(chunk, point);
		const Double3 &entityUp = Double3::UnitY;
		const Double3 entityRight = entityForward.cross(entityUp).normalized();
		const Double3 entityHalfWidth = entityRight * (width * 0.50);
		const Double3 entityHeight = entityUp * height;

		constexpr double ceilingScale = 1.0; // Unused/already baked into position.
		WriteTriangle(entityHalfWidth + entityHeight, entityHalfWidth, -entityHalfWidth, UV_TL, UV_BL, UV_BR, materialID, entityPosition, ceilingScale, 0, outEntityTriangles);
		WriteTriangle(-entityHalfWidth, -entityHalfWidth + entityHeight, entityHalfWidth + entityHeight, UV_BR, UV_TR, UV_TL, materialID, entityPosition, ceilingScale, 1, outEntityTriangles);
	}
}

BufferView<const RenderTriangle> SceneGraph::getVisibleOpaqueVoxelGeometry() const
{
	return BufferView<const RenderTriangle>(this->opaqueVoxelTriangles.data(), static_cast<int>(this->opaqueVoxelTriangles.size()));
}

BufferView<const RenderTriangle> SceneGraph::getVisibleAlphaTestedVoxelGeometry() const
{
	return BufferView<const RenderTriangle>(this->alphaTestedVoxelTriangles.data(), static_cast<int>(this->alphaTestedVoxelTriangles.size()));
}

BufferView<const RenderTriangle> SceneGraph::getVisibleEntityGeometry() const
{
	return BufferView<const RenderTriangle>(this->entityTriangles.data(), static_cast<int>(this->entityTriangles.size()));
}

void SceneGraph::updateVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double ceilingScale,
	double chasmAnimPercent, bool nightLightsAreActive)
{
	this->clearVoxels();

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

	const ChunkManager &chunkManager = levelInst.getChunkManager();

	// Compare chunk manager chunk count w/ our grid size and resize if needed.
	const int chunkCount = chunkManager.getChunkCount();
	/*if (this->chunkRenderInsts.size() != chunkCount)
	{
		this->chunkRenderInsts.resize(chunkCount);
	}*/

	// Populate render defs and insts for each chunk.
	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const SNInt chunkWidth = Chunk::WIDTH;
		const int chunkHeight = chunk.getHeight();
		const WEInt chunkDepth = Chunk::DEPTH;

		/*ChunkRenderDefinition chunkRenderDef;
		chunkRenderDef.init(chunkWidth, chunkHeight, chunkDepth);*/

		const ChunkInt2 chunkPos = chunk.getCoord();
		const ChunkInt2 relativeChunkPos = chunkPos - camera.chunk; // Relative to camera chunk.
		constexpr double chunkDimReal = static_cast<double>(ChunkUtils::CHUNK_DIM);

		// Top right and bottom left world space corners of this chunk.
		const NewDouble2 chunkTR2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, VoxelDouble2::Zero);
		const NewDouble2 chunkBL2D = VoxelUtils::chunkPointToNewPoint(relativeChunkPos, VoxelDouble2(chunkDimReal, chunkDimReal));

		const bool isChunkVisible = MathUtils::triangleRectangleIntersection(
			cameraEye2D, cameraFrustumRightPoint2D, cameraFrustumLeftPoint2D, chunkTR2D, chunkBL2D);
		if (!isChunkVisible)
		{
			continue;
		}

		for (WEInt z = 0; z < chunkDepth; z++)
		{
			for (int y = 0; y < chunkHeight; y++)
			{
				for (SNInt x = 0; x < chunkWidth; x++)
				{
					const Chunk::VoxelID voxelID = chunk.getVoxel(x, y, z);
					const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
					//const VoxelInstance *voxelInst = nullptr; // @todo: need to get the voxel inst for this voxel (if any).

					// @todo: looks like VoxelGeometry::getQuads() isn't aware that we want ALL geometry all the time;
					// so like chasm walls, diagonals, and edge voxels need to be handled differently than this. I don't think
					// passing a VoxelInstance would help. We need the "total possible geometry" for the voxel so certain faces
					// can be enabled/disabled by the voxel render def logic type.
					/*std::array<Quad, VoxelRenderDefinition::MAX_RECTS> quads;
					const int quadCount = VoxelGeometry::getQuads(voxelDef, VoxelInt3::Zero, ceilingScale,
						voxelInst, quads.data(), static_cast<int>(quads.size()));

					VoxelRenderDefinition voxelRenderDef;
					for (int i = 0; i < quadCount; i++)
					{
						const ObjectTextureID textureID = -1; // @todo: get from ChunkManager or Chunk.

						RectangleRenderDefinition &rectRenderDef = voxelRenderDef.rects[i];
						rectRenderDef.init(quads[i], textureID);
					}

					auto &faceIndicesDefs = voxelRenderDef.faceIndices;
					for (int i = 0; i < static_cast<int>(faceIndicesDefs.size()); i++)
					{
						VoxelRenderDefinition::FaceIndicesDef &faceIndicesDef = faceIndicesDefs[i];
						faceIndicesDef.count = quadCount;
						std::iota(faceIndicesDef.indices.begin(), faceIndicesDef.indices.end(), 0); // (naive) All faces visible all the time. Optimize if needed
						// @todo: to calculate properly, would we do a dot product check from the rect's center to each corner of a face to make sure it's front-facing?
						// Would have to do differently for chasms.
					}

					// @todo: once we are properly sharing voxel render defs instead of naively making a new one for each voxel,
					// probably need some get-or-add pattern for VoxelRenderDefIDs. Given some VoxelDefinition + VoxelInstance pair,
					// is there a VoxelRenderDefID for them or should we make a new render def?

					// @todo: map the Chunk::VoxelID to VoxelRenderDefID (I think?)

					this->voxelRenderDefs.emplace_back(std::move(voxelRenderDef));
					chunkRenderDef.voxelRenderDefIDs.set(x, y, z, static_cast<VoxelRenderDefID>(this->voxelRenderDefs.size()) - 1);*/

					const VoxelInt3 voxelPos(x, y, z);
					std::array<RenderTriangle, sgGeometry::MAX_TRIANGLES_PER_VOXEL> opaqueTrianglesBuffer, alphaTestedTrianglesBuffer;
					int opaqueTriangleCount = 0;
					int alphaTestedTriangleCount = 0;
					if (voxelDef.type == ArenaTypes::VoxelType::Wall)
					{
						const VoxelDefinition::WallData &wall = voxelDef.wall;
						const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(wall.sideTextureAssetRef);
						const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(wall.floorTextureAssetRef);
						const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(wall.ceilingTextureAssetRef);
						sgGeometry::WriteWall(chunkPos, voxelPos, ceilingScale, sideMaterialID, floorMaterialID, ceilingMaterialID,
							BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 12), &opaqueTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
					{
						const VoxelDefinition::FloorData &floor = voxelDef.floor;
						const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(floor.textureAssetRef);
						sgGeometry::WriteFloor(chunkPos, voxelPos, ceilingScale, materialID,
							BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
					{
						const VoxelDefinition::CeilingData &ceiling = voxelDef.ceiling;
						const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(ceiling.textureAssetRef);
						sgGeometry::WriteCeiling(chunkPos, voxelPos, ceilingScale, materialID,
							BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
					{
						const VoxelDefinition::RaisedData &raised = voxelDef.raised;
						const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(raised.sideTextureAssetRef);
						const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(raised.floorTextureAssetRef);
						const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(raised.ceilingTextureAssetRef);
						sgGeometry::WriteRaised(chunkPos, voxelPos, ceilingScale, raised.yOffset, raised.ySize,
							raised.vTop, raised.vBottom, sideMaterialID, floorMaterialID, ceilingMaterialID,
							BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 4), &opaqueTriangleCount,
							BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 8), &alphaTestedTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
					{
						const VoxelDefinition::DiagonalData &diagonal = voxelDef.diagonal;
						const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(diagonal.textureAssetRef);
						sgGeometry::WriteDiagonal(chunkPos, voxelPos, ceilingScale, diagonal.type1, materialID,
							BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 4), &opaqueTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
					{
						const VoxelDefinition::TransparentWallData &transparentWall = voxelDef.transparentWall;
						const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(transparentWall.textureAssetRef);
						sgGeometry::WriteTransparentWall(chunkPos, voxelPos, ceilingScale, materialID,
							BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 8), &alphaTestedTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
					{
						const VoxelDefinition::EdgeData &edge = voxelDef.edge;
						const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(edge.textureAssetRef);
						sgGeometry::WriteEdge(chunkPos, voxelPos, ceilingScale, edge.facing, edge.yOffset, edge.flipped, materialID,
							BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 4), &alphaTestedTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
					{
						const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
						const bool north = true; // @todo: need voxel instance
						const bool south = true;
						const bool east = true;
						const bool west = true;
						const bool isDry = chasm.type == ArenaTypes::ChasmType::Dry;
						const ObjectMaterialID floorMaterialID = levelInst.getChasmFloorMaterialID(chasm.type, chasmAnimPercent);
						const ObjectMaterialID sideMaterialID = levelInst.getChasmWallMaterialID(chasm.type, chasmAnimPercent, chasm.textureAssetRef);
						sgGeometry::WriteChasm(chunkPos, voxelPos, ceilingScale, north, south, east, west, isDry, floorMaterialID, sideMaterialID,
							BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 10), &opaqueTriangleCount);
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Door)
					{
						const VoxelDefinition::DoorData &door = voxelDef.door;
						const double animPercent = 0.0; // @todo: need voxel instance
						const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(door.textureAssetRef);
						sgGeometry::WriteDoor(chunkPos, voxelPos, ceilingScale, door.type, animPercent,
							materialID, BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 16), &alphaTestedTriangleCount);
					}

					if (opaqueTriangleCount > 0)
					{
						const auto srcStart = opaqueTrianglesBuffer.cbegin();
						const auto srcEnd = srcStart + opaqueTriangleCount;
						this->opaqueVoxelTriangles.insert(this->opaqueVoxelTriangles.end(), srcStart, srcEnd);
					}

					if (alphaTestedTriangleCount > 0)
					{
						const auto srcStart = alphaTestedTrianglesBuffer.cbegin();
						const auto srcEnd = srcStart + alphaTestedTriangleCount;
						this->alphaTestedVoxelTriangles.insert(this->alphaTestedVoxelTriangles.end(), srcStart, srcEnd);
					}
				}
			}
		}

		/*this->chunkRenderDefs.emplace_back(std::move(chunkRenderDef));

		ChunkRenderInstance chunkRenderInst;
		for (int i = 0; i < chunk.getVoxelInstCount(); i++)
		{
			const VoxelInstance &voxelInst = chunk.getVoxelInst(i);
			VoxelRenderInstance voxelRenderInst;
			// @todo

			chunkRenderInst.addVoxelRenderInstance(std::move(voxelRenderInst));
		}

		this->chunkRenderInsts.emplace_back(std::move(chunkRenderInst));*/
	}
}

void SceneGraph::updateEntities(const LevelInstance &levelInst, const CoordDouble3 &cameraPos, const VoxelDouble3 &cameraDir,
	const EntityDefinitionLibrary &entityDefLibrary, double ceilingScale, bool nightLightsAreActive, bool playerHasLight)
{
	this->clearEntities();

	const ChunkManager &chunkManager = levelInst.getChunkManager();
	const int chunkCount = chunkManager.getChunkCount();

	const EntityManager &entityManager = levelInst.getEntityManager();
	std::vector<const Entity*> entityPtrs;

	const CoordDouble2 cameraPos2D(cameraPos.chunk, VoxelDouble2(cameraPos.point.x, cameraPos.point.z));
	const VoxelDouble3 entityDir = -cameraDir;

	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const ChunkInt2 &chunkCoord = chunk.getCoord();
		const int entityCountInChunk = entityManager.getCountInChunk(chunkCoord);
		entityPtrs.resize(entityCountInChunk);
		const int writtenEntityCount = entityManager.getEntitiesInChunk(
			chunkCoord, entityPtrs.data(), static_cast<int>(entityPtrs.size()));
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
				const TextureAssetReference &textureAssetRef = animKeyframe.getTextureAssetRef();
				const bool flipped = animKeyframeList.isFlipped();
				const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && (entityDef.getDoodad().puddle);
				const ObjectMaterialID materialID = levelInst.getEntityMaterialID(textureAssetRef, flipped, reflective);

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
	this->clearSky();
	//DebugNotImplemented();
}

/*void SceneGraph::updateVisibleGeometry(const RenderCamera &camera)
{
	// @todo: clear current geometry/light/etc. buffers

	DebugNotImplemented();
}*/

void SceneGraph::clearVoxels()
{
	this->voxelRenderDefs.clear();
	this->chunkRenderDefs.clear();
	this->chunkRenderInsts.clear();
	this->opaqueVoxelTriangles.clear();
	this->alphaTestedVoxelTriangles.clear();
}

void SceneGraph::clearEntities()
{
	this->entityRenderDefs.clear();
	this->entityRenderInsts.clear();
	this->entityTriangles.clear();
}

void SceneGraph::clearSky()
{
	this->skyObjectRenderDefs.clear();
	this->skyObjectRenderInsts.clear();
	this->skyTriangles.clear();
}

void SceneGraph::clear()
{
	this->clearVoxels();
	this->clearEntities();
	this->clearSky();
}
