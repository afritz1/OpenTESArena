#include <numeric>

#include "SceneGraph.h"
#include "../../Entities/EntityManager.h"
#include "../../World/ChunkManager.h"
#include "../../World/VoxelGeometry.h"

#include "components/debug/Debug.h"

namespace sgGeometry
{
	std::vector<RenderTriangle> MakeCube(const Double3 &point, ObjectTextureID textureID)
	{
		std::vector<RenderTriangle> triangles;

		auto p = [&point](double x, double y, double z)
		{
			return point + Double3(x, y, z);
		};

		const Double2 uvTL(0.0, 0.0);
		const Double2 uvTR(1.0, 0.0);
		const Double2 uvBL(0.0, 1.0);
		const Double2 uvBR(1.0, 1.0);

		// Cube
		// X=0
		triangles.emplace_back(RenderTriangle(p(0.0, 1.0, 0.0), p(0.0, 0.0, 0.0), p(0.0, 0.0, 1.0), uvTL, uvBL, uvBR, textureID));
		triangles.emplace_back(RenderTriangle(p(0.0, 0.0, 1.0), p(0.0, 1.0, 1.0), p(0.0, 1.0, 0.0), uvBR, uvTR, uvTL, textureID));
		// X=1
		triangles.emplace_back(RenderTriangle(p(1.0, 1.0, 1.0), p(1.0, 0.0, 1.0), p(1.0, 0.0, 0.0), uvTL, uvBL, uvBR, textureID));
		triangles.emplace_back(RenderTriangle(p(1.0, 0.0, 0.0), p(1.0, 1.0, 0.0), p(1.0, 1.0, 1.0), uvBR, uvTR, uvTL, textureID));
		// Y=0
		triangles.emplace_back(RenderTriangle(p(1.0, 0.0, 1.0), p(0.0, 0.0, 1.0), p(0.0, 0.0, 0.0), uvTL, uvBL, uvBR, textureID));
		triangles.emplace_back(RenderTriangle(p(0.0, 0.0, 0.0), p(1.0, 0.0, 0.0), p(1.0, 0.0, 1.0), uvBR, uvTR, uvTL, textureID));
		// Y=1
		triangles.emplace_back(RenderTriangle(p(1.0, 1.0, 0.0), p(0.0, 1.0, 0.0), p(0.0, 1.0, 1.0), uvTL, uvBL, uvBR, textureID));
		triangles.emplace_back(RenderTriangle(p(0.0, 1.0, 1.0), p(1.0, 1.0, 1.0), p(1.0, 1.0, 0.0), uvBR, uvTR, uvTL, textureID));
		// Z=0
		triangles.emplace_back(RenderTriangle(p(1.0, 1.0, 0.0), p(1.0, 0.0, 0.0), p(0.0, 0.0, 0.0), uvTL, uvBL, uvBR, textureID));
		triangles.emplace_back(RenderTriangle(p(0.0, 0.0, 0.0), p(0.0, 1.0, 0.0), p(1.0, 1.0, 0.0), uvBR, uvTR, uvTL, textureID));
		// Z=1
		triangles.emplace_back(RenderTriangle(p(0.0, 1.0, 1.0), p(0.0, 0.0, 1.0), p(1.0, 0.0, 1.0), uvTL, uvBL, uvBR, textureID));
		triangles.emplace_back(RenderTriangle(p(1.0, 0.0, 1.0), p(1.0, 1.0, 1.0), p(0.0, 1.0, 1.0), uvBR, uvTR, uvTL, textureID));

		return triangles;
	}

	std::vector<RenderTriangle> MakeDebugMesh1(ObjectTextureID textureID)
	{
		return MakeCube(Double3::Zero, textureID);
	}

	std::vector<RenderTriangle> MakeDebugMesh2(ObjectTextureID textureID)
	{
		std::vector<RenderTriangle> triangles;

		for (int y = 0; y < 3; y += 2)
		{
			const Double3 point(
				24.0,
				static_cast<double>(y),
				0.0);
			std::vector<RenderTriangle> cubeTriangles = MakeCube(point, textureID);
			triangles.insert(triangles.end(), cubeTriangles.begin(), cubeTriangles.end());
		}

		return triangles;
	}

	std::vector<RenderTriangle> MakeDebugMesh3(ObjectTextureID textureID)
	{
		std::vector<RenderTriangle> triangles;

		for (int z = 0; z < 32; z += 2)
		{
			for (int y = 0; y < 4; y += 2)
			{
				for (int x = 0; x < 32; x += 2)
				{
					const Double3 point(
						static_cast<double>(x),
						static_cast<double>(y),
						static_cast<double>(z));
					std::vector<RenderTriangle> cubeTriangles = MakeCube(point, textureID);
					triangles.insert(triangles.end(), cubeTriangles.begin(), cubeTriangles.end());
				}
			}
		}

		return triangles;
	}

	static const std::vector<RenderTriangle> DebugTriangles = sgGeometry::MakeDebugMesh2(0);
}

BufferView<const RenderTriangle> SceneGraph::getAllGeometry() const
{
	const auto &triangles = sgGeometry::DebugTriangles;
	return BufferView<const RenderTriangle>(triangles.data(), static_cast<int>(triangles.size()));
}

void SceneGraph::updateVoxels(const ChunkManager &chunkManager, double ceilingScale, double chasmAnimPercent)
{
	this->clearVoxels();

	// Compare chunk manager chunk count w/ our grid size and resize if needed.
	const int chunkCount = chunkManager.getChunkCount();
	if (this->chunkRenderInsts.size() != chunkCount)
	{
		this->chunkRenderInsts.resize(chunkCount);
	}

	// Populate render defs and insts for each chunk.
	for (int i = 0; i < chunkCount; i++)
	{
		const Chunk &chunk = chunkManager.getChunk(i);
		const SNInt chunkWidth = Chunk::WIDTH;
		const int chunkHeight = chunk.getHeight();
		const WEInt chunkDepth = Chunk::DEPTH;

		ChunkRenderDefinition chunkRenderDef;
		chunkRenderDef.init(chunkWidth, chunkHeight, chunkDepth);

		for (WEInt z = 0; z < chunkDepth; z++)
		{
			for (int y = 0; y < chunkHeight; y++)
			{
				for (SNInt x = 0; x < chunkWidth; x++)
				{
					const Chunk::VoxelID voxelID = chunk.getVoxel(x, y, z);
					const VoxelDefinition &voxelDef = chunk.getVoxelDef(voxelID);
					const VoxelInstance *voxelInst = nullptr; // @todo: need to get the voxel inst for this voxel (if any).

					// @todo: looks like VoxelGeometry::getQuads() isn't aware that we want ALL geometry all the time;
					// so like chasm walls, diagonals, and edge voxels need to be handled differently than this. I don't think
					// passing a VoxelInstance would help. We need the "total possible geometry" for the voxel so certain faces
					// can be enabled/disabled by the voxel render def logic type.
					std::array<Quad, VoxelRenderDefinition::MAX_RECTS> quads;
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
					chunkRenderDef.voxelRenderDefIDs.set(x, y, z, static_cast<VoxelRenderDefID>(this->voxelRenderDefs.size()) - 1);
				}
			}
		}

		this->chunkRenderDefs.emplace_back(std::move(chunkRenderDef));

		ChunkRenderInstance chunkRenderInst;
		for (int i = 0; i < chunk.getVoxelInstCount(); i++)
		{
			const VoxelInstance &voxelInst = chunk.getVoxelInst(i);
			VoxelRenderInstance voxelRenderInst;
			// @todo

			chunkRenderInst.addVoxelRenderInstance(std::move(voxelRenderInst));
		}

		this->chunkRenderInsts.emplace_back(std::move(chunkRenderInst));
	}

	DebugNotImplemented();
}

void SceneGraph::updateEntities(const EntityManager &entityManager, bool nightLightsAreActive, bool playerHasLight)
{
	this->clearEntities();

	const int entityCount = entityManager.getCount();
	Buffer<const Entity*> entityPtrs(entityCount);

	DebugNotImplemented();
}

void SceneGraph::updateSky(const SkyInstance &skyInst, double daytimePercent, double latitude)
{
	this->clearSky();
	DebugNotImplemented();
}

void SceneGraph::updateVisibleGeometry(const RenderCamera &camera)
{
	// @todo: clear current geometry/light/etc. buffers

	DebugNotImplemented();
}

void SceneGraph::clearVoxels()
{
	this->voxelRenderDefs.clear();
	this->chunkRenderDefs.clear();
	this->chunkRenderInsts.clear();
}

void SceneGraph::clearEntities()
{
	this->entityRenderDefs.clear();
	this->entityRenderInsts.clear();
}

void SceneGraph::clearSky()
{
	this->skyObjectRenderDefs.clear();
	this->skyObjectRenderInsts.clear();
}

void SceneGraph::clear()
{
	this->clearVoxels();
	this->clearEntities();
	this->clearSky();
}
