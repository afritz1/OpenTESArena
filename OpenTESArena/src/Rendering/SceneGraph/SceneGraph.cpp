#include <numeric>

#include "SceneGraph.h"
#include "../../Entities/EntityManager.h"
#include "../../World/ChunkManager.h"
#include "../../World/VoxelGeometry.h"

#include "components/debug/Debug.h"

const BufferView<const int> SceneGraph::getVisibleGeometry() const
{
	DebugUnhandledReturn(BufferView<const int>);
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
