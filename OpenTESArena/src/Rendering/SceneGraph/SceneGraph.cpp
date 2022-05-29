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
			// Swings counter-clockwise. The "hinge flip" occurs when crossing the south face or west face.
			// By default, the hinge is at (nearX, farZ). When flipped, it's at (farX, nearZ).
			triangleCount = 8;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);

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
			// Slides to the left.
			triangleCount = 8;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);

			const Double2 uvTL(slideAmount, 0.0);
			const Double2 uvTR(1.0, 0.0);
			const Double2 uvBL(uvTL.x, 1.0);
			const Double2 uvBR(1.0, 1.0);

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
			// Raises up.
			triangleCount = 8;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);

			const Double2 uvTL(0.0, slideAmount);
			const Double2 uvTR(1.0, slideAmount);
			const Double2 uvBL(0.0, 1.0);
			const Double2 uvBR(1.0, 1.0);

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
			// Similar to Sliding but half goes left, half goes right.
			triangleCount = 16;
			DebugAssert(outAlphaTestedTriangles.getCount() >= triangleCount);

			const double splitAmount = slideAmount * 0.50;
			const double leftSplitAmount = 0.50 - splitAmount;
			const double rightSplitAmount = 0.50 + splitAmount;

			const Double2 leftUvTL(splitAmount, 0.0);
			const Double2 leftUvTR(0.5, 0.0);
			const Double2 leftUvBL(leftUvTL.x, 1.0);
			const Double2 leftUvBR(0.5, 1.0);
			const Double2 rightUvTL(0.5, 0.0);
			const Double2 rightUvTR(std::clamp(1.0 - splitAmount, 0.0, 1.0), 0.0);
			const Double2 rightUvBL(0.5, 1.0);
			const Double2 rightUvBR(rightUvTR.x, 1.0);

			// X=0
			const Double3 face0Left_v0 = v0;
			const Double3 face0Left_v1 = v1;
			const Double3 face0Left_v2 = v1 + ((v2 - v1) * leftSplitAmount);
			const Double3 face0Left_v3 = v0 + ((v3 - v0) * leftSplitAmount);
			const Double3 face0Right_v0 = v0 + ((v3 - v0) * rightSplitAmount);
			const Double3 face0Right_v1 = v1 + ((v2 - v1) * rightSplitAmount);
			const Double3 face0Right_v2 = v2;
			const Double3 face0Right_v3 = v3;
			WriteTriangle(face0Left_v0, face0Left_v1, face0Left_v2, leftUvTL, leftUvBL, leftUvBR, materialID, param0, voxelPosition, ceilingScale, 0, outAlphaTestedTriangles);
			WriteTriangle(face0Left_v2, face0Left_v3, face0Left_v0, leftUvBR, leftUvTR, leftUvTL, materialID, param0, voxelPosition, ceilingScale, 1, outAlphaTestedTriangles);
			WriteTriangle(face0Right_v0, face0Right_v1, face0Right_v2, rightUvTL, rightUvBL, rightUvBR, materialID, param0, voxelPosition, ceilingScale, 2, outAlphaTestedTriangles);
			WriteTriangle(face0Right_v2, face0Right_v3, face0Right_v0, rightUvBR, rightUvTR, rightUvTL, materialID, param0, voxelPosition, ceilingScale, 3, outAlphaTestedTriangles);

			// X=1
			const Double3 face1Left_v0 = v4;
			const Double3 face1Left_v1 = v5;
			const Double3 face1Left_v2 = v5 + ((v6 - v5) * leftSplitAmount);
			const Double3 face1Left_v3 = v4 + ((v7 - v4) * leftSplitAmount);
			const Double3 face1Right_v0 = v4 + ((v7 - v4) * rightSplitAmount);
			const Double3 face1Right_v1 = v5 + ((v6 - v5) * rightSplitAmount);
			const Double3 face1Right_v2 = v6;
			const Double3 face1Right_v3 = v7;
			WriteTriangle(face1Left_v0, face1Left_v1, face1Left_v2, leftUvTL, leftUvBL, leftUvBR, materialID, param0, voxelPosition, ceilingScale, 4, outAlphaTestedTriangles);
			WriteTriangle(face1Left_v2, face1Left_v3, face1Left_v0, leftUvBR, leftUvTR, leftUvTL, materialID, param0, voxelPosition, ceilingScale, 5, outAlphaTestedTriangles);
			WriteTriangle(face1Right_v0, face1Right_v1, face1Right_v2, rightUvTL, rightUvBL, rightUvBR, materialID, param0, voxelPosition, ceilingScale, 6, outAlphaTestedTriangles);
			WriteTriangle(face1Right_v2, face1Right_v3, face1Right_v0, rightUvBR, rightUvTR, rightUvTL, materialID, param0, voxelPosition, ceilingScale, 7, outAlphaTestedTriangles);

			// Z=0
			const Double3 face2Left_v0 = v7;
			const Double3 face2Left_v1 = v6;
			const Double3 face2Left_v2 = v6 + ((v1 - v6) * leftSplitAmount);
			const Double3 face2Left_v3 = v7 + ((v0 - v7) * leftSplitAmount);
			const Double3 face2Right_v0 = v7 + ((v0 - v7) * rightSplitAmount);
			const Double3 face2Right_v1 = v6 + ((v1 - v6) * rightSplitAmount);
			const Double3 face2Right_v2 = v1;
			const Double3 face2Right_v3 = v0;
			WriteTriangle(face2Left_v0, face2Left_v1, face2Left_v2, leftUvTL, leftUvBL, leftUvBR, materialID, param0, voxelPosition, ceilingScale, 8, outAlphaTestedTriangles);
			WriteTriangle(face2Left_v2, face2Left_v3, face2Left_v0, leftUvBR, leftUvTR, leftUvTL, materialID, param0, voxelPosition, ceilingScale, 9, outAlphaTestedTriangles);
			WriteTriangle(face2Right_v0, face2Right_v1, face2Right_v2, rightUvTL, rightUvBL, rightUvBR, materialID, param0, voxelPosition, ceilingScale, 10, outAlphaTestedTriangles);
			WriteTriangle(face2Right_v2, face2Right_v3, face2Right_v0, rightUvBR, rightUvTR, rightUvTL, materialID, param0, voxelPosition, ceilingScale, 11, outAlphaTestedTriangles);

			// Z=1
			const Double3 face3Left_v0 = v3;
			const Double3 face3Left_v1 = v2;
			const Double3 face3Left_v2 = v2 + ((v5 - v2) * leftSplitAmount);
			const Double3 face3Left_v3 = v3 + ((v4 - v3) * leftSplitAmount);
			const Double3 face3Right_v0 = v3 + ((v4 - v3) * rightSplitAmount);
			const Double3 face3Right_v1 = v2 + ((v5 - v2) * rightSplitAmount);
			const Double3 face3Right_v2 = v5;
			const Double3 face3Right_v3 = v4;
			WriteTriangle(face3Left_v0, face3Left_v1, face3Left_v2, leftUvTL, leftUvBL, leftUvBR, materialID, param0, voxelPosition, ceilingScale, 12, outAlphaTestedTriangles);
			WriteTriangle(face3Left_v2, face3Left_v3, face3Left_v0, leftUvBR, leftUvTR, leftUvTL, materialID, param0, voxelPosition, ceilingScale, 13, outAlphaTestedTriangles);
			WriteTriangle(face3Right_v0, face3Right_v1, face3Right_v2, rightUvTL, rightUvBL, rightUvBR, materialID, param0, voxelPosition, ceilingScale, 14, outAlphaTestedTriangles);
			WriteTriangle(face3Right_v2, face3Right_v3, face3Right_v0, rightUvBR, rightUvTR, rightUvTL, materialID, param0, voxelPosition, ceilingScale, 15, outAlphaTestedTriangles);
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

namespace sgMesh
{
	constexpr int MAX_VERTICES_PER_VOXEL = 8;
	constexpr int MAX_INDICES_PER_VOXEL = 36;
	constexpr int INDICES_PER_TRIANGLE = 3;
	constexpr int COMPONENTS_PER_VERTEX = 3; // XYZ
	constexpr int ATTRIBUTES_PER_VERTEX = 4; // XY texture coordinates

	constexpr int GetVoxelVertexCount(ArenaTypes::VoxelType voxelType)
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

	constexpr int GetVoxelOpaqueIndexCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Door:
		case ArenaTypes::VoxelType::Edge:
			return 0;
		case ArenaTypes::VoxelType::Wall:
			return 12 * INDICES_PER_TRIANGLE;
		case ArenaTypes::VoxelType::Raised:
			return 4 * INDICES_PER_TRIANGLE;
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
			return 2 * INDICES_PER_TRIANGLE;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	constexpr int GetVoxelAlphaTestedIndexCount(ArenaTypes::VoxelType voxelType)
	{
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::None:
		case ArenaTypes::VoxelType::Wall:
		case ArenaTypes::VoxelType::Floor:
		case ArenaTypes::VoxelType::Ceiling:
		case ArenaTypes::VoxelType::Diagonal:
			return 0;
		case ArenaTypes::VoxelType::Raised:
		case ArenaTypes::VoxelType::TransparentWall:
		case ArenaTypes::VoxelType::Chasm:
		case ArenaTypes::VoxelType::Door:
			return 12 * INDICES_PER_TRIANGLE;
		case ArenaTypes::VoxelType::Edge:
			return 2 * INDICES_PER_TRIANGLE;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(voxelType)));
		}
	}

	void WriteWallMeshBuffers(const VoxelDefinition::WallData &wall, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Wall) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteFloorMeshBuffers(const VoxelDefinition::FloorData &floor, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Floor) * 3> vertices =
		{
			// X=0

			// X=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteCeilingMeshBuffers(const VoxelDefinition::CeilingData &ceiling, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Ceiling) * 3> vertices =
		{
			// X=0

			// X=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteRaisedMeshBuffers(const VoxelDefinition::RaisedData &raised, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Raised) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteDiagonalMeshBuffers(const VoxelDefinition::DiagonalData &diagonal, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Diagonal) * 3> vertices =
		{
			// X=0

			// X=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteTransparentWallMeshBuffers(const VoxelDefinition::TransparentWallData &transparentWall,
		BufferView<double> outVertices, BufferView<double> outAttributes, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::TransparentWall) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteEdgeMeshBuffers(const VoxelDefinition::EdgeData &edge, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outAlphaTestedIndices)
	{
		// @todo: four different vertex buffers depending on the side? The vertical size is always the same.
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Edge) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteChasmMeshBuffers(const VoxelDefinition::ChasmData &chasm, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Chasm) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteDoorMeshBuffers(const VoxelDefinition::DoorData &door, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outAlphaTestedIndices)
	{
		constexpr std::array<double, GetVoxelVertexCount(ArenaTypes::VoxelType::Door) * 3> vertices =
		{
			// X=0

			// X=1

			// Y=0

			// Y=1

			// Z=0

			// Z=1

		};

		// @todo
		DebugNotImplemented();
	}

	void WriteVoxelMeshBuffers(const VoxelDefinition &voxelDef, BufferView<double> outVertices,
		BufferView<double> outAttributes, BufferView<int32_t> outOpaqueIndices,
		BufferView<int32_t> outAlphaTestedIndices)
	{
		const ArenaTypes::VoxelType voxelType = voxelDef.type;
		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Wall:
			WriteWallMeshBuffers(voxelDef.wall, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::Floor:
			WriteFloorMeshBuffers(voxelDef.floor, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::Ceiling:
			WriteCeilingMeshBuffers(voxelDef.ceiling, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::Raised:
			WriteRaisedMeshBuffers(voxelDef.raised, outVertices, outAttributes, outOpaqueIndices, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Diagonal:
			WriteDiagonalMeshBuffers(voxelDef.diagonal, outVertices, outAttributes, outOpaqueIndices);
			break;
		case ArenaTypes::VoxelType::TransparentWall:
			WriteTransparentWallMeshBuffers(voxelDef.transparentWall, outVertices, outAttributes, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Edge:
			WriteEdgeMeshBuffers(voxelDef.edge, outVertices, outAttributes, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Chasm:
			WriteChasmMeshBuffers(voxelDef.chasm, outVertices, outAttributes, outOpaqueIndices, outAlphaTestedIndices);
			break;
		case ArenaTypes::VoxelType::Door:
			WriteDoorMeshBuffers(voxelDef.door, outVertices, outAttributes, outAlphaTestedIndices);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}
}

namespace sgTexture
{
	// Loads the given voxel definition's textures into the voxel materials list if they haven't been loaded yet.
	void LoadVoxelDefTextures(const VoxelDefinition &voxelDef, std::vector<SceneGraph::LoadedVoxelMaterial> &voxelMaterials,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelDef.getTextureAssetReferenceCount(); i++)
		{
			const TextureAssetReference &textureAssetRef = voxelDef.getTextureAssetReference(i);
			const auto cacheIter = std::find_if(voxelMaterials.begin(), voxelMaterials.end(),
				[&textureAssetRef](const SceneGraph::LoadedVoxelMaterial &loadedMaterial)
			{
				return loadedMaterial.textureAssetRef == textureAssetRef;
			});

			if (cacheIter == voxelMaterials.end())
			{
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load voxel texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				ObjectTextureID voxelTextureID;
				if (!renderer.tryCreateObjectTexture(textureBuilder, &voxelTextureID))
				{
					DebugLogWarning("Couldn't create voxel texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef voxelTextureRef(voxelTextureID, renderer);
				ObjectMaterialID voxelMaterialID;
				if (!renderer.tryCreateObjectMaterial(voxelTextureID, &voxelMaterialID))
				{
					DebugLogWarning("Couldn't create voxel material \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectMaterialRef voxelMaterialRef(voxelMaterialID, renderer);
				SceneGraph::LoadedVoxelMaterial newMaterial;
				newMaterial.init(textureAssetRef, std::move(voxelTextureRef), std::move(voxelMaterialRef));
				voxelMaterials.emplace_back(std::move(newMaterial));
			}
		}
	}

	// Loads the given entity definition's textures into the entity materials list if they haven't been loaded yet.
	void LoadEntityDefTextures(const EntityDefinition &entityDef, std::vector<SceneGraph::LoadedEntityMaterial> &entityMaterials,
		TextureManager &textureManager, Renderer &renderer)
	{
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && entityDef.getDoodad().puddle;

		auto processKeyframe = [&entityMaterials, &textureManager, &renderer, reflective](
			const EntityAnimationDefinition::Keyframe &keyframe, bool flipped)
		{
			const TextureAssetReference &textureAssetRef = keyframe.getTextureAssetRef();
			const auto cacheIter = std::find_if(entityMaterials.begin(), entityMaterials.end(),
				[&textureAssetRef, flipped, reflective](const SceneGraph::LoadedEntityMaterial &loadedMaterial)
			{
				return (loadedMaterial.textureAssetRef == textureAssetRef) && (loadedMaterial.flipped == flipped) &&
					(loadedMaterial.reflective == reflective);
			});

			if (cacheIter == entityMaterials.end())
			{
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load entity texture \"" + textureAssetRef.filename + "\".");
					return;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				const int textureWidth = textureBuilder.getWidth();
				const int textureHeight = textureBuilder.getHeight();

				ObjectTextureID entityTextureID;
				if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, false, &entityTextureID))
				{
					DebugLogWarning("Couldn't create entity texture \"" + textureAssetRef.filename + "\".");
					return;
				}

				ScopedObjectTextureRef entityTextureRef(entityTextureID, renderer);
				DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
				const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
				const uint8_t *srcTexels = srcTexture.texels.get();

				LockedTexture lockedEntityTexture = renderer.lockObjectTexture(entityTextureID);
				if (!lockedEntityTexture.isValid())
				{
					DebugLogWarning("Couldn't lock entity texture \"" + textureAssetRef.filename + "\".");
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

				ObjectMaterialID entityMaterialID;
				if (!renderer.tryCreateObjectMaterial(entityTextureID, &entityMaterialID))
				{
					DebugLogWarning("Couldn't create entity material \"" + textureAssetRef.filename + "\".");
					return;
				}

				ScopedObjectMaterialRef entityMaterialRef(entityMaterialID, renderer);
				SceneGraph::LoadedEntityMaterial newMaterial;
				newMaterial.init(textureAssetRef, flipped, reflective, std::move(entityTextureRef), std::move(entityMaterialRef));
				entityMaterials.emplace_back(std::move(newMaterial));
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
	void LoadChasmFloorTextures(ArenaTypes::ChasmType chasmType, std::vector<SceneGraph::LoadedChasmMaterialList> &chasmMaterialLists,
		TextureManager &textureManager, Renderer &renderer)
	{
		const auto cacheIter = std::find_if(chasmMaterialLists.begin(), chasmMaterialLists.end(),
			[chasmType](const SceneGraph::LoadedChasmMaterialList &loadedMaterialList)
		{
			return loadedMaterialList.chasmType == chasmType;
		});

		DebugAssertMsg(cacheIter == chasmMaterialLists.end(), "Already loaded chasm floor materials for type \"" +
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

			SceneGraph::LoadedChasmMaterialList newMaterialList;
			newMaterialList.init(chasmType);

			const Buffer<TextureAssetReference> textureAssetRefs = TextureUtils::makeTextureAssetRefs(chasmFilename, textureManager);
			for (int i = 0; i < textureAssetRefs.getCount(); i++)
			{
				const TextureAssetReference &textureAssetRef = textureAssetRefs.get(i);
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load chasm texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				ObjectTextureID chasmTextureID;
				if (!renderer.tryCreateObjectTexture(textureBuilder, &chasmTextureID))
				{
					DebugLogWarning("Couldn't create chasm texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef chasmTextureRef(chasmTextureID, renderer);
				ObjectMaterialID chasmMaterialID;
				if (!renderer.tryCreateObjectMaterial(chasmTextureID, &chasmMaterialID))
				{
					DebugLogWarning("Couldn't create chasm floor material \"" + textureAssetRef.filename + "\".");
					return;
				}

				ScopedObjectMaterialRef chasmMaterialRef(chasmMaterialID, renderer);

				// Populate chasmTextureRefs and floorMaterialRefs, leave entries empty.
				newMaterialList.chasmTextureRefs.emplace_back(std::move(chasmTextureRef));
				newMaterialList.floorMaterialRefs.emplace_back(std::move(chasmMaterialRef));
			}

			chasmMaterialLists.emplace_back(std::move(newMaterialList));
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

			ObjectMaterialID chasmMaterialID;
			if (!renderer.tryCreateObjectMaterial(dryChasmTextureID, &chasmMaterialID))
			{
				DebugLogWarning("Couldn't create dry chasm floor material.");
				return;
			}

			ScopedObjectMaterialRef dryChasmMaterialRef(chasmMaterialID, renderer);
			SceneGraph::LoadedChasmMaterialList newMaterialList;
			newMaterialList.init(chasmType);

			// Populate chasmTextureRefs and floorMaterialRefs, leave entries empty.
			newMaterialList.chasmTextureRefs.emplace_back(std::move(dryChasmTextureRef));
			newMaterialList.floorMaterialRefs.emplace_back(std::move(dryChasmMaterialRef));
			chasmMaterialLists.emplace_back(std::move(newMaterialList));
		}
	}

	// Loads the chasm wall material for the given chasm and texture asset. This expects the chasm floor material to
	// already be loaded and available for sharing.
	void LoadChasmWallTextures(ArenaTypes::ChasmType chasmType, const TextureAssetReference &textureAssetRef,
		std::vector<SceneGraph::LoadedChasmMaterialList> &chasmMaterialLists, TextureManager &textureManager, Renderer &renderer)
	{
		const auto listIter = std::find_if(chasmMaterialLists.begin(), chasmMaterialLists.end(),
			[chasmType](const SceneGraph::LoadedChasmMaterialList &loadedMaterialList)
		{
			return loadedMaterialList.chasmType == chasmType;
		});

		DebugAssertMsg(listIter != chasmMaterialLists.end() && listIter->floorMaterialRefs.size() > 0,
			"Expected loaded chasm floor material list for type \"" + std::to_string(static_cast<int>(chasmType)) + "\".");

		std::vector<SceneGraph::LoadedChasmMaterialList::Entry> &entries = listIter->entries;
		const auto entryIter = std::find_if(entries.begin(), entries.end(),
			[&textureAssetRef](const SceneGraph::LoadedChasmMaterialList::Entry &entry)
		{
			return entry.wallTextureAssetRef == textureAssetRef;
		});

		if (entryIter == entries.end())
		{
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
			if (!textureBuilderID.has_value())
			{
				DebugLogWarning("Couldn't load chasm wall texture \"" + textureAssetRef.filename + "\".");
				return;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID wallTextureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &wallTextureID))
			{
				DebugLogWarning("Couldn't create chasm wall texture \"" + textureAssetRef.filename + "\".");
				return;
			}

			SceneGraph::LoadedChasmMaterialList::Entry newEntry;
			newEntry.wallTextureAssetRef = textureAssetRef;
			newEntry.wallTextureRef.init(wallTextureID, renderer);

			const std::vector<ScopedObjectTextureRef> &chasmTextureRefs = listIter->chasmTextureRefs;
			const int chasmTextureCount = static_cast<int>(chasmTextureRefs.size());
			for (int i = 0; i < chasmTextureCount; i++)
			{
				const ObjectTextureID floorTextureID = chasmTextureRefs[i].get();
				ObjectMaterialID wallMaterialID;
				if (!renderer.tryCreateObjectMaterial(floorTextureID, wallTextureID, &wallMaterialID))
				{
					DebugLogWarning("Couldn't create chasm wall material \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectMaterialRef wallMaterialRef(wallMaterialID, renderer);
				newEntry.wallMaterialRefs.emplace_back(std::move(wallMaterialRef));
			}

			entries.emplace_back(std::move(newEntry));
		}
	}
}

void SceneGraph::LoadedVoxelMaterial::init(const TextureAssetReference &textureAssetRef,
	ScopedObjectTextureRef &&objectTextureRef, ScopedObjectMaterialRef &&objectMaterialRef)
{
	this->textureAssetRef = textureAssetRef;
	this->objectTextureRef = std::move(objectTextureRef);
	this->objectMaterialRef = std::move(objectMaterialRef);
}

void SceneGraph::LoadedEntityMaterial::init(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective,
	ScopedObjectTextureRef &&objectTextureRef, ScopedObjectMaterialRef &&objectMaterialRef)
{
	this->textureAssetRef = textureAssetRef;
	this->flipped = flipped;
	this->reflective = reflective;
	this->objectTextureRef = std::move(objectTextureRef);
	this->objectMaterialRef = std::move(objectMaterialRef);
}

void SceneGraph::LoadedChasmMaterialList::init(ArenaTypes::ChasmType chasmType)
{
	this->chasmType = chasmType;
}

ObjectMaterialID SceneGraph::getVoxelMaterialID(const TextureAssetReference &textureAssetRef) const
{
	const auto iter = std::find_if(this->voxelMaterials.begin(), this->voxelMaterials.end(),
		[&textureAssetRef](const LoadedVoxelMaterial &loadedMaterial)
	{
		return loadedMaterial.textureAssetRef == textureAssetRef;
	});

	DebugAssertMsg(iter != this->voxelMaterials.end(), "No loaded voxel material for \"" + textureAssetRef.filename + "\".");
	const ScopedObjectMaterialRef &objectMaterialRef = iter->objectMaterialRef;
	return objectMaterialRef.get();
}

ObjectMaterialID SceneGraph::getEntityMaterialID(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective) const
{
	const auto iter = std::find_if(this->entityMaterials.begin(), this->entityMaterials.end(),
		[&textureAssetRef, flipped, reflective](const LoadedEntityMaterial &loadedMaterial)
	{
		return (loadedMaterial.textureAssetRef == textureAssetRef) && (loadedMaterial.flipped == flipped) &&
			(loadedMaterial.reflective == reflective);
	});

	DebugAssertMsg(iter != this->entityMaterials.end(), "No loaded entity material for \"" + textureAssetRef.filename + "\".");
	const ScopedObjectMaterialRef &objectMaterialRef = iter->objectMaterialRef;
	return objectMaterialRef.get();
}

ObjectMaterialID SceneGraph::getChasmFloorMaterialID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent) const
{
	const auto iter = std::find_if(this->chasmMaterialLists.begin(), this->chasmMaterialLists.end(),
		[chasmType](const LoadedChasmMaterialList &loadedMaterialList)
	{
		return loadedMaterialList.chasmType == chasmType;
	});

	DebugAssertMsg(iter != this->chasmMaterialLists.end(), "No loaded chasm floor material for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");
	const std::vector<ScopedObjectMaterialRef> &floorMaterialRefs = iter->floorMaterialRefs;
	const int textureCount = static_cast<int>(floorMaterialRefs.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(floorMaterialRefs, index);
	const ScopedObjectMaterialRef &floorMaterialRef = floorMaterialRefs[index];
	return floorMaterialRef.get();
}

ObjectMaterialID SceneGraph::getChasmWallMaterialID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent,
	const TextureAssetReference &textureAssetRef) const
{
	const auto listIter = std::find_if(this->chasmMaterialLists.begin(), this->chasmMaterialLists.end(),
		[chasmType, &textureAssetRef](const LoadedChasmMaterialList &loadedMaterialList)
	{
		return (loadedMaterialList.chasmType == chasmType);
	});

	DebugAssertMsg(listIter != this->chasmMaterialLists.end(), "No loaded chasm floor material for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");

	const std::vector<LoadedChasmMaterialList::Entry> &entries = listIter->entries;
	const auto entryIter = std::find_if(entries.begin(), entries.end(),
		[&textureAssetRef](const LoadedChasmMaterialList::Entry &entry)
	{
		return entry.wallTextureAssetRef == textureAssetRef;
	});

	DebugAssertMsg(entryIter != entries.end(), "No loaded chasm wall material for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\" and texture \"" + textureAssetRef.filename + "\".");

	const std::vector<ScopedObjectMaterialRef> &wallMaterialRefs = entryIter->wallMaterialRefs;
	const int textureCount = static_cast<int>(wallMaterialRefs.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(wallMaterialRefs, index);
	const ScopedObjectMaterialRef &wallMaterialRef = wallMaterialRefs[index];
	return wallMaterialRef.get();
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
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Dry, this->chasmMaterialLists, textureManager, renderer);
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Wet, this->chasmMaterialLists, textureManager, renderer);
	sgTexture::LoadChasmFloorTextures(ArenaTypes::ChasmType::Lava, this->chasmMaterialLists, textureManager, renderer);

	// Load textures known at level load time. Note that none of the object texture IDs allocated here are
	// matched with voxel/entity instances until the chunks containing them are created.
	auto loadLevelDefTextures = [this, &mapDefinition, &textureManager, &renderer](int levelIndex)
	{
		const LevelInfoDefinition &levelInfoDef = mapDefinition.getLevelInfoForLevel(levelIndex);

		for (int i = 0; i < levelInfoDef.getVoxelDefCount(); i++)
		{
			const VoxelDefinition &voxelDef = levelInfoDef.getVoxelDef(i);
			sgTexture::LoadVoxelDefTextures(voxelDef, this->voxelMaterials, textureManager, renderer);

			if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
			{
				const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
				sgTexture::LoadChasmWallTextures(chasm.type, chasm.textureAssetRef, this->chasmMaterialLists, textureManager, renderer);
			}
		}

		for (int i = 0; i < levelInfoDef.getEntityDefCount(); i++)
		{
			const EntityDefinition &entityDef = levelInfoDef.getEntityDef(i);
			sgTexture::LoadEntityDefTextures(entityDef, this->entityMaterials, textureManager, renderer);
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
		sgTexture::LoadEntityDefTextures(maleEntityDef, this->entityMaterials, textureManager, renderer);
		sgTexture::LoadEntityDefTextures(femaleEntityDef, this->entityMaterials, textureManager, renderer);
	}
}

void SceneGraph::loadVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double ceilingScale,
	double chasmAnimPercent, bool nightLightsAreActive, RendererSystem3D &renderer)
{
	// Expect empty chunks to have been created just now (it's done before this in the edge case
	// there are no voxels at all since entities rely on chunks existing).
	DebugAssert(!this->graphChunks.empty());

	const ChunkManager &chunkManager = levelInst.getChunkManager();
	for (int chunkIndex = 0; chunkIndex < chunkManager.getChunkCount(); chunkIndex++)
	{
		const Chunk &chunk = chunkManager.getChunk(chunkIndex);
		SceneGraphChunk &graphChunk = this->graphChunks[chunkIndex];

		// Add voxel definitions to the scene graph.
		for (int voxelDefIndex = 0; voxelDefIndex < chunk.getVoxelDefCount(); voxelDefIndex++)
		{
			const Chunk::VoxelID voxelID = static_cast<Chunk::VoxelID>(voxelDefIndex);
			const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
			const ArenaTypes::VoxelType voxelType = voxelDef.type;

			SceneGraphVoxelDefinition graphVoxelDef;
			if (voxelType != ArenaTypes::VoxelType::None) // Only attempt to create buffers for non-air voxels.
			{
				const int vertexCount = sgMesh::GetVoxelVertexCount(voxelType);
				if (!renderer.tryCreateVertexBuffer(vertexCount, sgMesh::COMPONENTS_PER_VERTEX, &graphVoxelDef.vertexBufferID))
				{
					DebugLogError("Couldn't create vertex buffer for voxel ID " + std::to_string(voxelID) +
						" in chunk (" + chunk.getPosition().toString() + ").");
					continue;
				}

				if (!renderer.tryCreateAttributeBuffer(vertexCount, sgMesh::COMPONENTS_PER_VERTEX, &graphVoxelDef.attributeBufferID))
				{
					DebugLogError("Couldn't create attribute buffer for voxel ID " + std::to_string(voxelID) +
						" in chunk (" + chunk.getPosition().toString() + ").");
					graphVoxelDef.freeBuffers(renderer);
					continue;
				}

				std::array<double, sgMesh::MAX_VERTICES_PER_VOXEL * sgMesh::COMPONENTS_PER_VERTEX> vertices;
				std::array<double, sgMesh::MAX_VERTICES_PER_VOXEL * sgMesh::ATTRIBUTES_PER_VERTEX> attributes;
				std::array<int32_t, sgMesh::MAX_INDICES_PER_VOXEL> opaqueIndices, alphaTestedIndices;
				vertices.fill(0.0);
				attributes.fill(0.0);
				opaqueIndices.fill(0);
				alphaTestedIndices.fill(0);

				// Generate mesh data for this voxel definition.
				sgMesh::WriteVoxelMeshBuffers(voxelDef,
					BufferView<double>(vertices.data(), vertices.size()),
					BufferView<double>(attributes.data(), attributes.size()),
					BufferView<int32_t>(opaqueIndices.data(), opaqueIndices.size()),
					BufferView<int32_t>(alphaTestedIndices.data(), alphaTestedIndices.size()));

				renderer.populateVertexBuffer(graphVoxelDef.vertexBufferID, BufferView<const double>(vertices.data(), vertexCount));
				renderer.populateAttributeBuffer(graphVoxelDef.attributeBufferID, BufferView<const double>(attributes.data(), vertexCount));

				const int opaqueIndexCount = sgMesh::GetVoxelOpaqueIndexCount(voxelType);
				if (opaqueIndexCount > 0)
				{
					if (!renderer.tryCreateIndexBuffer(opaqueIndexCount, &graphVoxelDef.opaqueIndexBufferID))
					{
						DebugLogError("Couldn't create opaque index buffer for voxel ID " + std::to_string(voxelID) +
							" in chunk (" + chunk.getPosition().toString() + ").");
						graphVoxelDef.freeBuffers(renderer);
						continue;
					}

					renderer.populateIndexBuffer(graphVoxelDef.opaqueIndexBufferID,
						BufferView<const int32_t>(opaqueIndices.data(), opaqueIndexCount));
				}

				const int alphaTestedIndexCount = sgMesh::GetVoxelAlphaTestedIndexCount(voxelType);
				if (alphaTestedIndexCount > 0)
				{
					if (!renderer.tryCreateIndexBuffer(alphaTestedIndexCount, &graphVoxelDef.alphaTestedIndexBufferID))
					{
						DebugLogError("Couldn't create alpha-tested index buffer for voxel ID " + std::to_string(voxelID) +
							" in chunk (" + chunk.getPosition().toString() + ").");
						graphVoxelDef.freeBuffers(renderer);
						continue;
					}

					renderer.populateIndexBuffer(graphVoxelDef.alphaTestedIndexBufferID,
						BufferView<const int32_t>(alphaTestedIndices.data(), alphaTestedIndexCount));
				}
			}

			graphChunk.voxelDefs.emplace_back(std::move(graphVoxelDef));
		}

		// Assign voxel definition indices for each graph voxel.
		for (WEInt z = 0; z < Chunk::DEPTH; z++)
		{
			for (int y = 0; y < chunk.getHeight(); y++)
			{
				for (SNInt x = 0; x < Chunk::WIDTH; x++)
				{
					// Get the voxel def mapping's index and use it for this voxel.
					const Chunk::VoxelID voxelID = chunk.getVoxel(x, y, z);
					const auto defIter = graphChunk.voxelDefMappings.find(voxelID);
					DebugAssert(defIter != graphChunk.voxelDefMappings.end());
					graphChunk.voxels.set(x, y, z, defIter->second);
				}
			}
		}
	}
}

void SceneGraph::loadEntities(const LevelInstance &levelInst, const RenderCamera &camera,
	const EntityDefinitionLibrary &entityDefLibrary, double ceilingScale, bool nightLightsAreActive,
	bool playerHasLight, RendererSystem3D &renderer)
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
}

void SceneGraph::loadScene(const LevelInstance &levelInst, const SkyInstance &skyInst,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, const RenderCamera &camera,
	double ceilingScale, double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
	double daytimePercent, double latitude, const EntityDefinitionLibrary &entityDefLibrary,
	TextureManager &textureManager, Renderer &renderer, RendererSystem3D &renderer3D)
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
	}

	// @todo: load textures somewhere in here and in a way that their draw calls can be generated; maybe want to store
	// TextureAssetReferences with SceneGraphVoxelDefinition? Might want textures to be ref-counted if reused between chunks.

	this->loadTextures(activeLevelIndex, mapDefinition, citizenGenInfo, textureManager, renderer);
	this->loadVoxels(levelInst, camera, ceilingScale, chasmAnimPercent, nightLightsAreActive, renderer3D);
	this->loadEntities(levelInst, camera, entityDefLibrary, ceilingScale, nightLightsAreActive, playerHasLight, renderer3D);
	this->loadSky(skyInst, daytimePercent, latitude, renderer3D);
	this->loadWeather(skyInst, daytimePercent, renderer3D);

	// @todo: populate draw calls since update() only operates on dirty stuff from chunk manager/entity manager/etc.
	DebugNotImplemented();
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

void SceneGraph::updateVoxels(const LevelInstance &levelInst, const RenderCamera &camera, double ceilingScale,
	double chasmAnimPercent, bool nightLightsAreActive, RendererSystem3D &renderer)
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

	/*// Arbitrary value, just needs to be long enough to touch the farthest chunks in practice.
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
	}*/

	// @todo: sort opaque chunk geometry near to far
	// @todo: sort alpha-tested chunk geometry far to near
	// ^ for both of these, the goal is so we can essentially just memcpy each chunk's geometry into the scene graph's draw lists.
}

void SceneGraph::updateEntities(const LevelInstance &levelInst, const RenderCamera &camera,
	const EntityDefinitionLibrary &entityDefLibrary, double ceilingScale, bool nightLightsAreActive,
	bool playerHasLight, RendererSystem3D &renderer)
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
	}*/
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
	double ceilingScale, double chasmAnimPercent, bool nightLightsAreActive, bool playerHasLight,
	double daytimePercent, double latitude, const EntityDefinitionLibrary &entityDefLibrary,
	TextureManager &textureManager, RendererSystem3D &renderer)
{
	// @todo: update chunks first so we know which chunks need to be fully loaded in with loadVoxels(), etc..
	DebugNotImplemented();

	this->updateVoxels(levelInst, camera, ceilingScale, chasmAnimPercent, nightLightsAreActive, renderer);
	this->updateEntities(levelInst, camera, entityDefLibrary, ceilingScale, nightLightsAreActive, playerHasLight, renderer);
	this->updateSky(skyInst, daytimePercent, latitude);
	this->updateWeather(skyInst);
}*/
