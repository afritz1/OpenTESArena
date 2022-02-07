#include <numeric>
#include <optional>

#include "SceneGraph.h"
#include "../ArenaRenderUtils.h"
#include "../RenderCamera.h"
#include "../../Assets/MIFUtils.h"
#include "../../Entities/EntityManager.h"
#include "../../Entities/EntityVisibilityState.h"
#include "../../Math/Constants.h"
#include "../../Math/Matrix4.h"
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
		const Double2 &uv2, ObjectMaterialID materialID, double param0, const Double3 &voxelPosition, double ceilingScale,
		int index, BufferView<RenderTriangle> &outTriangles)
	{
		Double3 worldV0, worldV1, worldV2;
		MakeWorldSpaceVertices(voxelPosition, v0, v1, v2, ceilingScale, &worldV0, &worldV1, &worldV2);

		RenderTriangle &triangle = outTriangles.get(index);
		triangle.init(worldV0, worldV1, worldV2, uv0, uv1, uv2, materialID, param0);
	}

	// Geometry generation functions (currently in world space, might be chunk space or something else later).
	// @todo: these functions should not assume opaque/alpha-tested per face, they should query the texture associated
	// with each face for that, and then write to the appropriate out buffer.
	void WriteWall(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID sideMaterialID,
		ObjectMaterialID floorMaterialID, ObjectMaterialID ceilingMaterialID, double fadePercent,
		BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount)
	{
		constexpr int triangleCount = 12;
		DebugAssert(outOpaqueTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// X=0
		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 1, outOpaqueTriangles);
		// X=1
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 2, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 3, outOpaqueTriangles);
		// Y=0
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, floorMaterialID, fadePercent, voxelPosition, ceilingScale, 4, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(1.0, 0.0, 1.0), UV_BR, UV_TR, UV_TL, floorMaterialID, fadePercent, voxelPosition, ceilingScale, 5, outOpaqueTriangles);
		// Y=1
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), UV_TL, UV_BL, UV_BR, ceilingMaterialID, fadePercent, voxelPosition, ceilingScale, 6, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, ceilingMaterialID, fadePercent, voxelPosition, ceilingScale, 7, outOpaqueTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 8, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 9, outOpaqueTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 10, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 11, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteFloor(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID materialID,
		double fadePercent, BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount)
	{
		constexpr int triangleCount = 2;
		DebugAssert(outOpaqueTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, fadePercent, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, fadePercent, voxelPosition, ceilingScale, 1, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteCeiling(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID materialID,
		double fadePercent, BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount)
	{
		constexpr int triangleCount = 2;
		DebugAssert(outOpaqueTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, fadePercent, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(1.0, 0.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, fadePercent, voxelPosition, ceilingScale, 1, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteRaised(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, double yOffset, double ySize,
		double vTop, double vBottom, ObjectMaterialID sideMaterialID, ObjectMaterialID floorMaterialID, ObjectMaterialID ceilingMaterialID,
		double fadePercent, BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount,
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
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(0.0, yBottom, 1.0), Double3(0.0, yBottom, 0.0), UV_TL, UV_BL, UV_BR, floorMaterialID, fadePercent, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(1.0, yBottom, 0.0), Double3(1.0, yBottom, 1.0), UV_BR, UV_TR, UV_TL, floorMaterialID, fadePercent, voxelPosition, ceilingScale, 1, outOpaqueTriangles);
		// Y=top
		WriteTriangle(Double3(1.0, yTop, 0.0), Double3(0.0, yTop, 0.0), Double3(0.0, yTop, 1.0), UV_TL, UV_BL, UV_BR, ceilingMaterialID, fadePercent, voxelPosition, ceilingScale, 2, outOpaqueTriangles);
		WriteTriangle(Double3(0.0, yTop, 1.0), Double3(1.0, yTop, 1.0), Double3(1.0, yTop, 0.0), UV_BR, UV_TR, UV_TL, ceilingMaterialID, fadePercent, voxelPosition, ceilingScale, 3, outOpaqueTriangles);

		// Alpha-tested
		// X=0
		WriteTriangle(Double3(0.0, yTop, 0.0), Double3(0.0, yBottom, 0.0), Double3(0.0, yBottom, 1.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, yBottom, 1.0), Double3(0.0, yTop, 1.0), Double3(0.0, yTop, 0.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
		// X=1
		WriteTriangle(Double3(1.0, yTop, 1.0), Double3(1.0, yBottom, 1.0), Double3(1.0, yBottom, 0.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, yBottom, 0.0), Double3(1.0, yTop, 0.0), Double3(1.0, yTop, 1.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, yTop, 0.0), Double3(1.0, yBottom, 0.0), Double3(0.0, yBottom, 0.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(0.0, yTop, 0.0), Double3(1.0, yTop, 0.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, yTop, 1.0), Double3(0.0, yBottom, 1.0), Double3(1.0, yBottom, 1.0), sideUvTL, sideUvBL, sideUvBR, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(1.0, yTop, 1.0), Double3(0.0, yTop, 1.0), sideUvBR, sideUvTR, sideUvTL, sideMaterialID, fadePercent, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);

		*outOpaqueTriangleCount = opaqueTriangleCount;
		*outAlphaTestedTriangleCount = alphaTestedTriangleCount;
	}

	void WriteDiagonal(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, bool flipped, ObjectMaterialID materialID,
		double fadePercent, BufferView<RenderTriangle> &outOpaqueTriangles, int *outOpaqueTriangleCount)
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
		WriteTriangle(Double3(startX, 1.0, startZ), Double3(startX, 0.0, startZ), Double3(endX, 0.0, endZ), UV_TL, UV_BL, UV_BR, materialID, fadePercent, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(endX, 0.0, endZ), Double3(endX, 1.0, endZ), Double3(startX, 1.0, startZ), UV_BR, UV_TR, UV_TL, materialID, fadePercent, voxelPosition, ceilingScale, 1, outOpaqueTriangles);
		// Back side
		WriteTriangle(Double3(endX, 1.0, endZ), Double3(endX, 0.0, endZ), Double3(startX, 0.0, startZ), UV_TL, UV_BL, UV_BR, materialID, fadePercent, voxelPosition, ceilingScale, 2, outOpaqueTriangles);
		WriteTriangle(Double3(startX, 0.0, startZ), Double3(startX, 1.0, startZ), Double3(endX, 1.0, endZ), UV_BR, UV_TR, UV_TL, materialID, fadePercent, voxelPosition, ceilingScale, 3, outOpaqueTriangles);

		*outOpaqueTriangleCount = triangleCount;
	}

	void WriteTransparentWall(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ObjectMaterialID materialID,
		BufferView<RenderTriangle> &outAlphaTestedTriangles, int *outAlphaTestedTriangleCount)
	{
		constexpr int triangleCount = 8;
		DebugAssert(outAlphaTestedTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		constexpr double param0 = 0.0; // Unused.

		// X=0
		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
		// X=1
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);

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

		constexpr double param0 = 0.0; // Unused.

		// Front side
		WriteTriangle(Double3(startX, yTop, startZ), Double3(startX, yBottom, startZ), Double3(endX, yBottom, endZ), frontUvTL, frontUvBL, frontUvBR, materialID, param0, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
		WriteTriangle(Double3(endX, yBottom, endZ), Double3(endX, yTop, endZ), Double3(startX, yTop, startZ), frontUvBR, frontUvTR, frontUvTL, materialID, param0, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
		// Back side
		WriteTriangle(Double3(endX, yTop, endZ), Double3(endX, yBottom, endZ), Double3(startX, yBottom, startZ), backUvTL, backUvBL, backUvBR, materialID, param0, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
		WriteTriangle(Double3(startX, yBottom, startZ), Double3(startX, yTop, startZ), Double3(endX, yTop, endZ), backUvBR, backUvTR, backUvTL, materialID, param0, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);

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

		constexpr double param0 = 0.0; // Unused.

		// Y=bottom (always present)
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(0.0, yBottom, 1.0), Double3(1.0, yBottom, 1.0), UV_TL, UV_BL, UV_BR, floorMaterialID, param0, voxelPosition, ceilingScale, 0, outOpaqueTriangles);
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(1.0, yBottom, 0.0), Double3(0.0, yBottom, 0.0), UV_BR, UV_TR, UV_TL, floorMaterialID, param0, voxelPosition, ceilingScale, 1, outOpaqueTriangles);

		int triangleIndex = 2;
		if (north)
		{
			// X=0
			WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, yBottom, 1.0), Double3(0.0, yBottom, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		if (south)
		{
			// X=1
			WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, yBottom, 0.0), Double3(1.0, yBottom, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		if (east)
		{
			// Z=0
			WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, yBottom, 0.0), Double3(1.0, yBottom, 0.0), UV_TL, UV_BL, UV_BR, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(1.0, yBottom, 0.0), Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		if (west)
		{
			// Z=1
			WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, yBottom, 1.0), Double3(0.0, yBottom, 1.0), UV_TL, UV_BL, UV_BR, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex, outOpaqueTriangles);
			WriteTriangle(Double3(0.0, yBottom, 1.0), Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideMaterialID, param0, voxelPosition, ceilingScale, triangleIndex + 1, outOpaqueTriangles);
			triangleIndex += 2;
		}

		*outOpaqueTriangleCount = triangleIndex;
	}

	void WriteDoor(const ChunkInt2 &chunk, const VoxelInt3 &voxel, double ceilingScale, ArenaTypes::DoorType doorType,
		double animPercent, ObjectMaterialID materialID, BufferView<RenderTriangle> &outAlphaTestedTriangles,
		int *outAlphaTestedTriangleCount)
	{
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// Base vertices and face offsets (distance from origin for rotation transform) used by all doors.
		const Double3 v0(0.0, 1.0, 0.0);
		const Double3 v1(0.0, 0.0, 0.0);
		const Double3 v2(0.0, 0.0, 1.0);
		const Double3 v3(0.0, 1.0, 1.0);
		const Double3 v4(1.0, 1.0, 1.0);
		const Double3 v5(1.0, 0.0, 1.0);
		const Double3 v6(1.0, 0.0, 0.0);
		const Double3 v7(1.0, 1.0, 0.0);
		const Double3 face0Offset = Double3::Zero;
		const Double3 face1Offset(1.0, 0.0, 1.0);
		const Double3 face2Offset(1.0, 0.0, 0.0);
		const Double3 face3Offset(0.0, 0.0, 1.0);

		// Percent of voxel side length a sliding/raising door can go.
		constexpr double maxSlidePercent = std::clamp(1.0 - ArenaRenderUtils::DOOR_MIN_VISIBLE, 0.0, 1.0);
		const double slideAmount = std::clamp(animPercent * maxSlidePercent, 0.0, 1.0);

		constexpr double param0 = 0.0; // Unused.

		// Generate and transform vertices by anim percent; each door type has its own transform behavior.
		int triangleCount = 0;
		if (doorType == ArenaTypes::DoorType::Swinging)
		{
			triangleCount = 8;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);

			// Swings counter-clockwise. The "hinge flip" occurs when crossing the south face or west face.
			// By default, the hinge is at (nearX, farZ). When flipped, it's at (farX, nearZ).
			const Radians angle = -Constants::HalfPi * animPercent;
			const Matrix4d rotationMatrix = Matrix4d::yRotation(angle);

			auto transformSwingingPoint = [&rotationMatrix](const Double3 &point, const Double3 &offset)
			{
				// Need to translate vertex to the origin before rotating.
				const Double3 basePoint = point - offset;
				const Double4 rotatedPoint = rotationMatrix * Double4(basePoint.x, basePoint.y, basePoint.z, 1.0);
				const Double3 rotatedPoint3D(rotatedPoint.x, rotatedPoint.y, rotatedPoint.z);
				return rotatedPoint3D + offset;
			};

			// @todo: update hinge position depending on camera eye position
			// @todo: make face offsets depend on camera eye position being left of the left edge, or south of the south edge.

			// X=0
			const Double3 face0_v0 = transformSwingingPoint(v0, face0Offset);
			const Double3 face0_v1 = transformSwingingPoint(v1, face0Offset);
			const Double3 face0_v2 = transformSwingingPoint(v2, face0Offset);
			const Double3 face0_v3 = transformSwingingPoint(v3, face0Offset);
			WriteTriangle(face0_v0, face0_v1, face0_v2, UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
			WriteTriangle(face0_v2, face0_v3, face0_v0, UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);

			// X=1
			const Double3 face1_v0 = transformSwingingPoint(v4, face1Offset);
			const Double3 face1_v1 = transformSwingingPoint(v5, face1Offset);
			const Double3 face1_v2 = transformSwingingPoint(v6, face1Offset);
			const Double3 face1_v3 = transformSwingingPoint(v7, face1Offset);
			WriteTriangle(face1_v0, face1_v1, face1_v2, UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
			WriteTriangle(face1_v2, face1_v3, face1_v0, UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);

			// Z=0
			const Double3 face2_v0 = transformSwingingPoint(v7, face2Offset);
			const Double3 face2_v1 = transformSwingingPoint(v6, face2Offset);
			const Double3 face2_v2 = transformSwingingPoint(v1, face2Offset);
			const Double3 face2_v3 = transformSwingingPoint(v0, face2Offset);
			WriteTriangle(face2_v0, face2_v1, face2_v2, UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
			WriteTriangle(face2_v2, face2_v3, face2_v0, UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);

			// Z=1
			const Double3 face3_v0 = transformSwingingPoint(v3, face3Offset);
			const Double3 face3_v1 = transformSwingingPoint(v2, face3Offset);
			const Double3 face3_v2 = transformSwingingPoint(v5, face3Offset);
			const Double3 face3_v3 = transformSwingingPoint(v4, face3Offset);
			WriteTriangle(face3_v0, face3_v1, face3_v2, UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
			WriteTriangle(face3_v2, face3_v3, face3_v0, UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);
		}
		else if (doorType == ArenaTypes::DoorType::Sliding)
		{
			triangleCount = 8;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);
			
			const Double2 uvTL(slideAmount, 0.0);
			const Double2 uvTR(1.0, 0.0);
			const Double2 uvBL(uvTL.x, 1.0);
			const Double2 uvBR(1.0, 1.0);

			// Slides to the left.
			// X=0
			const Double3 face0_v0 = v0;
			const Double3 face0_v1 = v1;
			const Double3 face0_v2 = v2 + ((v1 - v2) * slideAmount);
			const Double3 face0_v3 = v3 + ((v0 - v3) * slideAmount);
			WriteTriangle(face0_v0, face0_v1, face0_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
			WriteTriangle(face0_v2, face0_v3, face0_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);

			// X=1
			const Double3 face1_v0 = v4;
			const Double3 face1_v1 = v5;
			const Double3 face1_v2 = v6 + ((v5 - v6) * slideAmount);
			const Double3 face1_v3 = v7 + ((v4 - v7) * slideAmount);
			WriteTriangle(face1_v0, face1_v1, face1_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
			WriteTriangle(face1_v2, face1_v3, face1_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);

			// Z=0
			const Double3 face2_v0 = v7;
			const Double3 face2_v1 = v6;
			const Double3 face2_v2 = v1 + ((v6 - v1) * slideAmount);
			const Double3 face2_v3 = v0 + ((v7 - v0) * slideAmount);
			WriteTriangle(face2_v0, face2_v1, face2_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
			WriteTriangle(face2_v2, face2_v3, face2_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);

			// Z=1
			const Double3 face3_v0 = v3;
			const Double3 face3_v1 = v2;
			const Double3 face3_v2 = v5 + ((v2 - v5) * slideAmount);
			const Double3 face3_v3 = v4 + ((v3 - v4) * slideAmount);
			WriteTriangle(face3_v0, face3_v1, face3_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
			WriteTriangle(face3_v2, face3_v3, face3_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);
		}
		else if (doorType == ArenaTypes::DoorType::Raising)
		{
			triangleCount = 8;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);

			const Double2 uvTL(0.0, slideAmount);
			const Double2 uvTR(1.0, slideAmount);
			const Double2 uvBL(0.0, 1.0);
			const Double2 uvBR(1.0, 1.0);

			// Raises up.
			// X=0
			const Double3 face0_v0 = v0;
			const Double3 face0_v1 = v1 + ((v0 - v1) * slideAmount);
			const Double3 face0_v2 = v2 + ((v3 - v2) * slideAmount);
			const Double3 face0_v3 = v3;
			WriteTriangle(face0_v0, face0_v1, face0_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
			WriteTriangle(face0_v2, face0_v3, face0_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);

			// X=1
			const Double3 face1_v0 = v4;
			const Double3 face1_v1 = v5 + ((v4 - v5) * slideAmount);
			const Double3 face1_v2 = v6 + ((v7 - v6) * slideAmount);
			const Double3 face1_v3 = v7;
			WriteTriangle(face1_v0, face1_v1, face1_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
			WriteTriangle(face1_v2, face1_v3, face1_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);

			// Z=0
			const Double3 face2_v0 = v7;
			const Double3 face2_v1 = v6 + ((v7 - v6) * slideAmount);
			const Double3 face2_v2 = v1 + ((v0 - v1) * slideAmount);
			const Double3 face2_v3 = v0;
			WriteTriangle(face2_v0, face2_v1, face2_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
			WriteTriangle(face2_v2, face2_v3, face2_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);

			// Z=1
			const Double3 face3_v0 = v3;
			const Double3 face3_v1 = v2 + ((v3 - v2) * slideAmount);
			const Double3 face3_v2 = v5 + ((v4 - v5) * slideAmount);
			const Double3 face3_v3 = v4;
			WriteTriangle(face3_v0, face3_v1, face3_v2, uvTL, uvBL, uvBR, materialID, param0, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
			WriteTriangle(face3_v2, face3_v3, face3_v0, uvBR, uvTR, uvTL, materialID, param0, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);
		}
		else if (doorType == ArenaTypes::DoorType::Splitting)
		{
			triangleCount = 16;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);

			// Similar to Sliding but half goes left, half goes right.
			// X=0
			WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
			WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
			// X=1
			WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
			WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);
			// Z=0
			WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
			WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);
			// Z=1
			WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, materialID, param0, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
			WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, materialID, param0, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(doorType)));
		}

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

		constexpr double param0 = 0.0; // Unused.

		constexpr double ceilingScale = 1.0; // Unused/already baked into position.
		WriteTriangle(entityHalfWidth + entityHeight, entityHalfWidth, -entityHalfWidth, UV_TL, UV_BL, UV_BR, materialID, param0, entityPosition, ceilingScale, 0, outEntityTriangles);
		WriteTriangle(-entityHalfWidth, -entityHalfWidth + entityHeight, entityHalfWidth + entityHeight, UV_BR, UV_TR, UV_TL, materialID, param0, entityPosition, ceilingScale, 1, outEntityTriangles);
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
				const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(wall.sideTextureAssetRef);
				const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(wall.floorTextureAssetRef);
				const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(wall.ceilingTextureAssetRef);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteWall(chunkPos, voxelPos, ceilingScale, sideMaterialID, floorMaterialID, ceilingMaterialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 12), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
			{
				const VoxelDefinition::FloorData &floor = voxelDef.floor;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(floor.textureAssetRef);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteFloor(chunkPos, voxelPos, ceilingScale, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
			{
				const VoxelDefinition::CeilingData &ceiling = voxelDef.ceiling;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(ceiling.textureAssetRef);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteCeiling(chunkPos, voxelPos, ceilingScale, materialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 2), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
			{
				const VoxelDefinition::RaisedData &raised = voxelDef.raised;
				const ObjectMaterialID sideMaterialID = levelInst.getVoxelMaterialID(raised.sideTextureAssetRef);
				const ObjectMaterialID floorMaterialID = levelInst.getVoxelMaterialID(raised.floorTextureAssetRef);
				const ObjectMaterialID ceilingMaterialID = levelInst.getVoxelMaterialID(raised.ceilingTextureAssetRef);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteRaised(chunkPos, voxelPos, ceilingScale, raised.yOffset, raised.ySize,
					raised.vTop, raised.vBottom, sideMaterialID, floorMaterialID, ceilingMaterialID, fadePercent,
					BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 4), &opaqueTriangleCount,
					BufferView<RenderTriangle>(alphaTestedTrianglesBuffer.data(), 8), &alphaTestedTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
			{
				const VoxelDefinition::DiagonalData &diagonal = voxelDef.diagonal;
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(diagonal.textureAssetRef);
				const double fadePercent = getVoxelFadePercentOrDefault(voxelPos);
				sgGeometry::WriteDiagonal(chunkPos, voxelPos, ceilingScale, diagonal.type1, materialID, fadePercent,
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
				const ObjectMaterialID sideMaterialID = levelInst.getChasmWallMaterialID(chasm.type, chasmAnimPercent, chasm.textureAssetRef);
				sgGeometry::WriteChasm(chunkPos, voxelPos, ceilingScale, hasNorthFace, hasSouthFace, hasEastFace, hasWestFace,
					isDry, floorMaterialID, sideMaterialID, BufferView<RenderTriangle>(opaqueTrianglesBuffer.data(), 10), &opaqueTriangleCount);
			}
			else if (voxelDef.type == ArenaTypes::VoxelType::Door)
			{
				const VoxelDefinition::DoorData &door = voxelDef.door;
				const double animPercent = getVoxelOpenDoorPercentOrDefault(voxelPos);
				const ObjectMaterialID materialID = levelInst.getVoxelMaterialID(door.textureAssetRef);
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

	// @todo: only call this on a scene change? the dirty voxels above should be keeping everything good now.
	this->clearVoxels();

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
