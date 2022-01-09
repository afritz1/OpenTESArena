#include <numeric>

#include "SceneGraph.h"
#include "../../Entities/EntityManager.h"
#include "../../World/ChunkManager.h"
#include "../../World/LevelInstance.h"
#include "../../World/VoxelFacing2D.h"
#include "../../World/VoxelGeometry.h"

#include "components/debug/Debug.h"

namespace sgGeometry
{
	constexpr int MAX_TRIANGLES_PER_VOXEL = 12;

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
		const Double2 &uv2, ObjectTextureID textureID, const Double3 &voxelPosition, double ceilingScale, int index,
		BufferView<RenderTriangle> &outTriangles)
	{
		Double3 worldV0, worldV1, worldV2;
		MakeWorldSpaceVertices(voxelPosition, v0, v1, v2, ceilingScale, &worldV0, &worldV1, &worldV2);

		RenderTriangle &triangle = outTriangles.get(index);
		triangle.init(worldV0, worldV1, worldV2, uv0, uv1, uv2, textureID);
	}

	// Geometry generation functions (currently in world space, might be chunk space or something else later).
	int WriteWall(const ChunkInt2 &chunk, const Int3 &voxel, double ceilingScale, ObjectTextureID sideTextureID,
		ObjectTextureID floorTextureID, ObjectTextureID ceilingTextureID, BufferView<RenderTriangle> &outTriangles)
	{
		constexpr int triangleCount = 12;
		DebugAssert(outTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// X=0
		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, sideTextureID, voxelPosition, ceilingScale, 0, outTriangles);
		WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideTextureID, voxelPosition, ceilingScale, 1, outTriangles);
		// X=1
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, sideTextureID, voxelPosition, ceilingScale, 2, outTriangles);
		WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideTextureID, voxelPosition, ceilingScale, 3, outTriangles);
		// Y=0
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, floorTextureID, voxelPosition, ceilingScale, 4, outTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(1.0, 0.0, 1.0), UV_BR, UV_TR, UV_TL, floorTextureID, voxelPosition, ceilingScale, 5, outTriangles);
		// Y=1
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), UV_TL, UV_BL, UV_BR, ceilingTextureID, voxelPosition, ceilingScale, 6, outTriangles);
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, ceilingTextureID, voxelPosition, ceilingScale, 7, outTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, sideTextureID, voxelPosition, ceilingScale, 8, outTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, sideTextureID, voxelPosition, ceilingScale, 9, outTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, sideTextureID, voxelPosition, ceilingScale, 10, outTriangles);
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, sideTextureID, voxelPosition, ceilingScale, 11, outTriangles);

		return triangleCount;
	}

	int WriteFloor(const ChunkInt2 &chunk, const Int3 &voxel, double ceilingScale, ObjectTextureID textureID,
		BufferView<RenderTriangle> &outTriangles)
	{
		constexpr int triangleCount = 2;
		DebugAssert(outTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 1.0, 1.0), Double3(1.0, 1.0, 1.0), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 0, outTriangles);
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 1.0, 0.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 1, outTriangles);

		return triangleCount;
	}

	int WriteCeiling(const ChunkInt2 &chunk, const Int3 &voxel, double ceilingScale, ObjectTextureID textureID,
		BufferView<RenderTriangle> &outTriangles)
	{
		constexpr int triangleCount = 2;
		DebugAssert(outTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 0, outTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(1.0, 0.0, 1.0), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 1, outTriangles);

		return triangleCount;
	}

	int WriteRaised(const ChunkInt2 &chunk, const Int3 &voxel, double ceilingScale, double yOffset, double ySize,
		double vTop, double vBottom, ObjectTextureID sideTextureID, ObjectTextureID floorTextureID,
		ObjectTextureID ceilingTextureID, BufferView<RenderTriangle> &outTriangles)
	{
		constexpr int triangleCount = 12;
		DebugAssert(outTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		const double yBottom = yOffset;
		const double yTop = yOffset + ySize;

		const Double2 sideUvTL(0.0, vTop);
		const Double2 sideUvTR(1.0, vTop);
		const Double2 sideUvBL(0.0, vBottom);
		const Double2 sideUvBR(1.0, vBottom);

		// X=0
		WriteTriangle(Double3(0.0, yTop, 0.0), Double3(0.0, yBottom, 0.0), Double3(0.0, yBottom, 1.0), sideUvTL, sideUvBL, sideUvBR, sideTextureID, voxelPosition, ceilingScale, 0, outTriangles);
		WriteTriangle(Double3(0.0, yBottom, 1.0), Double3(0.0, yTop, 1.0), Double3(0.0, yTop, 0.0), sideUvBR, sideUvTR, sideUvTL, sideTextureID, voxelPosition, ceilingScale, 1, outTriangles);
		// X=1
		WriteTriangle(Double3(1.0, yTop, 1.0), Double3(1.0, yBottom, 1.0), Double3(1.0, yBottom, 0.0), sideUvTL, sideUvBL, sideUvBR, sideTextureID, voxelPosition, ceilingScale, 2, outTriangles);
		WriteTriangle(Double3(1.0, yBottom, 0.0), Double3(1.0, yTop, 0.0), Double3(1.0, yTop, 1.0), sideUvBR, sideUvTR, sideUvTL, sideTextureID, voxelPosition, ceilingScale, 3, outTriangles);
		// Y=bottom
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(0.0, yBottom, 1.0), Double3(0.0, yBottom, 0.0), UV_TL, UV_BL, UV_BR, floorTextureID, voxelPosition, ceilingScale, 4, outTriangles);
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(1.0, yBottom, 0.0), Double3(1.0, yBottom, 1.0), UV_BR, UV_TR, UV_TL, floorTextureID, voxelPosition, ceilingScale, 5, outTriangles);
		// Y=top
		WriteTriangle(Double3(1.0, yTop, 0.0), Double3(0.0, yTop, 0.0), Double3(0.0, yTop, 1.0), UV_TL, UV_BL, UV_BR, ceilingTextureID, voxelPosition, ceilingScale, 6, outTriangles);
		WriteTriangle(Double3(0.0, yTop, 1.0), Double3(1.0, yTop, 1.0), Double3(1.0, yTop, 0.0), UV_BR, UV_TR, UV_TL, ceilingTextureID, voxelPosition, ceilingScale, 7, outTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, yTop, 0.0), Double3(1.0, yBottom, 0.0), Double3(0.0, yBottom, 0.0), sideUvTL, sideUvBL, sideUvBR, sideTextureID, voxelPosition, ceilingScale, 8, outTriangles);
		WriteTriangle(Double3(0.0, yBottom, 0.0), Double3(0.0, yTop, 0.0), Double3(1.0, yTop, 0.0), sideUvBR, sideUvTR, sideUvTL, sideTextureID, voxelPosition, ceilingScale, 9, outTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, yTop, 1.0), Double3(0.0, yBottom, 1.0), Double3(1.0, yBottom, 1.0), sideUvTL, sideUvBL, sideUvBR, sideTextureID, voxelPosition, ceilingScale, 10, outTriangles);
		WriteTriangle(Double3(1.0, yBottom, 1.0), Double3(1.0, yTop, 1.0), Double3(0.0, yTop, 1.0), sideUvBR, sideUvTR, sideUvTL, sideTextureID, voxelPosition, ceilingScale, 11, outTriangles);

		return triangleCount;
	}

	int WriteDiagonal(const ChunkInt2 &chunk, const Int3 &voxel, double ceilingScale, bool flipped, ObjectTextureID textureID,
		BufferView<RenderTriangle> &outTriangles)
	{
		constexpr int triangleCount = 4; // @todo: might change to 2 in the future if a back-face culling bool is added.
		DebugAssert(outTriangles.getCount() == triangleCount);
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
		WriteTriangle(Double3(startX, 1.0, startZ), Double3(startX, 0.0, startZ), Double3(endX, 0.0, endZ), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 0, outTriangles);
		WriteTriangle(Double3(endX, 0.0, endZ), Double3(endX, 1.0, endZ), Double3(startX, 1.0, startZ), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 1, outTriangles);
		// Back side
		WriteTriangle(Double3(endX, 1.0, endZ), Double3(endX, 0.0, endZ), Double3(startX, 0.0, startZ), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 2, outTriangles);
		WriteTriangle(Double3(startX, 0.0, startZ), Double3(startX, 1.0, startZ), Double3(endX, 1.0, endZ), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 3, outTriangles);

		return triangleCount;
	}

	int WriteTransparentWall(const ChunkInt2 &chunk, const Int3 &voxel, double ceilingScale, ObjectTextureID textureID,
		BufferView<RenderTriangle> &outTriangles)
	{
		constexpr int triangleCount = 8;
		DebugAssert(outTriangles.getCount() == triangleCount);
		const Double3 voxelPosition = MakeVoxelPosition(chunk, voxel, ceilingScale);

		// X=0
		WriteTriangle(Double3(0.0, 1.0, 0.0), Double3(0.0, 0.0, 0.0), Double3(0.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 0, outTriangles);
		WriteTriangle(Double3(0.0, 0.0, 1.0), Double3(0.0, 1.0, 1.0), Double3(0.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 1, outTriangles);
		// X=1
		WriteTriangle(Double3(1.0, 1.0, 1.0), Double3(1.0, 0.0, 1.0), Double3(1.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 2, outTriangles);
		WriteTriangle(Double3(1.0, 0.0, 0.0), Double3(1.0, 1.0, 0.0), Double3(1.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 3, outTriangles);
		// Z=0
		WriteTriangle(Double3(1.0, 1.0, 0.0), Double3(1.0, 0.0, 0.0), Double3(0.0, 0.0, 0.0), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 4, outTriangles);
		WriteTriangle(Double3(0.0, 0.0, 0.0), Double3(0.0, 1.0, 0.0), Double3(1.0, 1.0, 0.0), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 5, outTriangles);
		// Z=1
		WriteTriangle(Double3(0.0, 1.0, 1.0), Double3(0.0, 0.0, 1.0), Double3(1.0, 0.0, 1.0), UV_TL, UV_BL, UV_BR, textureID, voxelPosition, ceilingScale, 6, outTriangles);
		WriteTriangle(Double3(1.0, 0.0, 1.0), Double3(1.0, 1.0, 1.0), Double3(0.0, 1.0, 1.0), UV_BR, UV_TR, UV_TL, textureID, voxelPosition, ceilingScale, 7, outTriangles);

		return triangleCount;
	}

	int WriteEdge(const ChunkInt2 &chunk, const Int3 &voxel, double ceilingScale, VoxelFacing2D facing, double yOffset,
		bool flipped, ObjectTextureID textureID, BufferView<RenderTriangle> &outTriangles)
	{
		constexpr int triangleCount = 4; // @todo: might change to 2 in the future if a back-face culling bool is added.
		DebugAssert(outTriangles.getCount() == triangleCount);
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
		WriteTriangle(Double3(startX, yTop, startZ), Double3(startX, yBottom, startZ), Double3(endX, yBottom, endZ), frontUvTL, frontUvBL, frontUvBR, textureID, voxelPosition, ceilingScale, 0, outTriangles);
		WriteTriangle(Double3(endX, yBottom, endZ), Double3(endX, yTop, endZ), Double3(startX, yTop, startZ), frontUvBR, frontUvTR, frontUvTL, textureID, voxelPosition, ceilingScale, 1, outTriangles);
		// Back side
		WriteTriangle(Double3(endX, yTop, endZ), Double3(endX, yBottom, endZ), Double3(startX, yBottom, startZ), backUvTL, backUvBL, backUvBR, textureID, voxelPosition, ceilingScale, 2, outTriangles);
		WriteTriangle(Double3(startX, yBottom, startZ), Double3(startX, yTop, startZ), Double3(endX, yTop, endZ), backUvBR, backUvTR, backUvTL, textureID, voxelPosition, ceilingScale, 3, outTriangles);

		return triangleCount;
	}
}

BufferView<const RenderTriangle> SceneGraph::getAllGeometry() const
{
	return BufferView<const RenderTriangle>(this->voxelTriangles.data(), static_cast<int>(this->voxelTriangles.size()));
}

void SceneGraph::updateVoxels(const LevelInstance &levelInst, double ceilingScale, double chasmAnimPercent,
	bool nightLightsAreActive)
{
	this->clearVoxels();

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

					const ChunkInt2 chunkPos = chunk.getCoord();
					const VoxelInt3 voxelPos(x, y, z);
					std::array<RenderTriangle, sgGeometry::MAX_TRIANGLES_PER_VOXEL> trianglesBuffer;
					int triangleCount;
					if (voxelDef.type == ArenaTypes::VoxelType::Wall)
					{
						const VoxelDefinition::WallData &wall = voxelDef.wall;
						const ObjectTextureID sideTextureID = levelInst.getVoxelTextureID(wall.sideTextureAssetRef);
						const ObjectTextureID floorTextureID = levelInst.getVoxelTextureID(wall.floorTextureAssetRef);
						const ObjectTextureID ceilingTextureID = levelInst.getVoxelTextureID(wall.ceilingTextureAssetRef);
						triangleCount = sgGeometry::WriteWall(chunkPos, voxelPos, ceilingScale, sideTextureID,
							floorTextureID, ceilingTextureID, BufferView<RenderTriangle>(trianglesBuffer.data(), 12));
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
					{
						const VoxelDefinition::FloorData &floor = voxelDef.floor;
						const ObjectTextureID textureID = levelInst.getVoxelTextureID(floor.textureAssetRef);
						triangleCount = sgGeometry::WriteFloor(chunkPos, voxelPos, ceilingScale, textureID,
							BufferView<RenderTriangle>(trianglesBuffer.data(), 2));
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
					{
						const VoxelDefinition::CeilingData &ceiling = voxelDef.ceiling;
						const ObjectTextureID textureID = levelInst.getVoxelTextureID(ceiling.textureAssetRef);
						triangleCount = sgGeometry::WriteCeiling(chunkPos, voxelPos, ceilingScale, textureID,
							BufferView<RenderTriangle>(trianglesBuffer.data(), 2));
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
					{
						const VoxelDefinition::RaisedData &raised = voxelDef.raised;
						const ObjectTextureID sideTextureID = levelInst.getVoxelTextureID(raised.sideTextureAssetRef);
						const ObjectTextureID floorTextureID = levelInst.getVoxelTextureID(raised.floorTextureAssetRef);
						const ObjectTextureID ceilingTextureID = levelInst.getVoxelTextureID(raised.ceilingTextureAssetRef);
						triangleCount = sgGeometry::WriteRaised(chunkPos, voxelPos, ceilingScale, raised.yOffset, raised.ySize,
							raised.vTop, raised.vBottom, sideTextureID, floorTextureID, ceilingTextureID,
							BufferView<RenderTriangle>(trianglesBuffer.data(), 12));
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
					{
						const VoxelDefinition::DiagonalData &diagonal = voxelDef.diagonal;
						const ObjectTextureID textureID = levelInst.getVoxelTextureID(diagonal.textureAssetRef);
						triangleCount = sgGeometry::WriteDiagonal(chunkPos, voxelPos, ceilingScale, diagonal.type1, textureID,
							BufferView<RenderTriangle>(trianglesBuffer.data(), 4));
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
					{
						const VoxelDefinition::TransparentWallData &transparentWall = voxelDef.transparentWall;
						const ObjectTextureID textureID = levelInst.getVoxelTextureID(transparentWall.textureAssetRef);
						triangleCount = sgGeometry::WriteTransparentWall(chunkPos, voxelPos, ceilingScale, textureID,
							BufferView<RenderTriangle>(trianglesBuffer.data(), 8));
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
					{
						const VoxelDefinition::EdgeData &edge = voxelDef.edge;
						const ObjectTextureID textureID = levelInst.getVoxelTextureID(edge.textureAssetRef);
						triangleCount = sgGeometry::WriteEdge(chunkPos, voxelPos, ceilingScale, edge.facing, edge.yOffset,
							edge.flipped, textureID, BufferView<RenderTriangle>(trianglesBuffer.data(), 4));
					}
					else
					{
						triangleCount = 0;
					}

					this->voxelTriangles.insert(this->voxelTriangles.end(), trianglesBuffer.cbegin(), trianglesBuffer.cbegin() + triangleCount);
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

void SceneGraph::updateEntities(const EntityManager &entityManager, bool nightLightsAreActive, bool playerHasLight)
{
	this->clearEntities();

	const int entityCount = entityManager.getCount();
	Buffer<const Entity*> entityPtrs(entityCount);

	//DebugNotImplemented();
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
	this->voxelTriangles.clear();
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
