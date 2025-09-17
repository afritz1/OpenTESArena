#include <algorithm>

#include "Renderer.h"
#include "RenderVoxelChunk.h"
#include "../World/ChunkUtils.h"

RenderVoxelCombinedFaceDrawCallEntry::RenderVoxelCombinedFaceDrawCallEntry()
{
	this->transformIndex = -1;
}

RenderVoxelNonCombinedDrawCallEntry::RenderVoxelNonCombinedDrawCallEntry()
{
	this->transformBufferID = -1;
}

RenderVoxelDoorDrawCallsEntry::RenderVoxelDoorDrawCallsEntry()
{
	this->transformBufferID = -1;
	this->drawCallCount = 0;
}

void RenderVoxelChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	this->meshInstMappings.emplace(VoxelChunk::AIR_SHAPE_DEF_ID, RenderVoxelChunk::AIR_MESH_INST_ID);

	// Add empty mesh instance for air.
	this->addMeshInst(RenderMeshInstance());
}

RenderMeshInstID RenderVoxelChunk::addMeshInst(RenderMeshInstance &&meshInst)
{
	const RenderMeshInstID id = static_cast<RenderMeshInstID>(this->meshInsts.size());
	this->meshInsts.emplace_back(std::move(meshInst));
	return id;
}

void RenderVoxelChunk::freeDoorMaterial(SNInt x, int y, WEInt z, Renderer &renderer)
{
	const VoxelInt3 voxel(x, y, z);

	for (int i = 0; i < static_cast<int>(this->doorMaterialInstEntries.size()); i++)
	{
		const RenderVoxelMaterialInstanceEntry &entry = this->doorMaterialInstEntries[i];
		if (entry.voxel == voxel)
		{
			renderer.freeMaterialInstance(entry.materialInstID);
			this->doorMaterialInstEntries.erase(this->doorMaterialInstEntries.begin() + i);
			break;
		}
	}
}

void RenderVoxelChunk::freeFadeMaterial(SNInt x, int y, WEInt z, Renderer &renderer)
{
	const VoxelInt3 voxel(x, y, z);

	for (int i = 0; i < static_cast<int>(this->fadeMaterialInstEntries.size()); i++)
	{
		const RenderVoxelMaterialInstanceEntry &entry = this->fadeMaterialInstEntries[i];
		if (entry.voxel == voxel)
		{
			renderer.freeMaterialInstance(entry.materialInstID);
			this->fadeMaterialInstEntries.erase(this->fadeMaterialInstEntries.begin() + i);
			break;
		}
	}
}

void RenderVoxelChunk::freeBuffers(Renderer &renderer)
{
	for (RenderMeshInstance &meshInst : this->meshInsts)
	{
		meshInst.freeBuffers(renderer);
	}

	for (RenderVoxelNonCombinedDrawCallEntry &entry : this->nonCombinedDrawCallEntries)
	{
		renderer.freeUniformBuffer(entry.transformBufferID);
		entry.transformBufferID = -1;
	}

	for (RenderVoxelDoorDrawCallsEntry &entry : this->doorDrawCallsEntries)
	{
		renderer.freeUniformBuffer(entry.transformBufferID);
		entry.transformBufferID = -1;
	}

	if (this->transformHeap.uniformBufferID >= 0)
	{
		renderer.freeUniformBuffer(this->transformHeap.uniformBufferID);
		this->transformHeap.uniformBufferID = -1;
	}

	for (RenderVoxelMaterialInstanceEntry &entry : this->doorMaterialInstEntries)
	{
		renderer.freeMaterialInstance(entry.materialInstID);
		entry.materialInstID = -1;
	}

	for (RenderVoxelMaterialInstanceEntry &entry : this->fadeMaterialInstEntries)
	{
		renderer.freeMaterialInstance(entry.materialInstID);
		entry.materialInstID = -1;
	}
}

void RenderVoxelChunk::clear()
{
	Chunk::clear();
	this->meshInsts.clear();
	this->meshInstMappings.clear();
	this->combinedFaceDrawCallEntries.clear();
	this->nonCombinedDrawCallEntries.clear();
	this->doorDrawCallsEntries.clear();
	this->doorMaterialInstEntries.clear();
	this->fadeMaterialInstEntries.clear();
	this->transformHeap.clear();
}
