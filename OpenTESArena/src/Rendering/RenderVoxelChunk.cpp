#include <algorithm>

#include "Renderer.h"
#include "RenderVoxelChunk.h"
#include "../World/ChunkUtils.h"

RenderVoxelDrawCallRange::RenderVoxelDrawCallRange()
{
	this->clear();
}

void RenderVoxelDrawCallRange::clear()
{
	this->index = 0;
	this->count = 0;
}

RenderVoxelDrawCallHeap::RenderVoxelDrawCallHeap()
{
	this->nextDrawCall = 0;
}

Span<RenderDrawCall> RenderVoxelDrawCallHeap::get(RenderVoxelDrawCallRangeID id)
{
	const RenderVoxelDrawCallRange &drawCallRange = this->drawCallRangesPool.get(id);
	RenderDrawCall *rangeBegin = std::begin(this->drawCalls) + drawCallRange.index;
	DebugAssert((rangeBegin + drawCallRange.count) <= std::end(this->drawCalls));
	return Span<RenderDrawCall>(rangeBegin, drawCallRange.count);
}

Span<const RenderDrawCall> RenderVoxelDrawCallHeap::get(RenderVoxelDrawCallRangeID id) const
{
	const RenderVoxelDrawCallRange &drawCallRange = this->drawCallRangesPool.get(id);
	const RenderDrawCall *rangeBegin = std::begin(this->drawCalls) + drawCallRange.index;
	DebugAssert((rangeBegin + drawCallRange.count) <= std::end(this->drawCalls));
	return Span<const RenderDrawCall>(rangeBegin, drawCallRange.count);
}

RenderVoxelDrawCallRangeID RenderVoxelDrawCallHeap::alloc(int drawCallCount)
{
	DebugAssert(drawCallCount > 0);

	const RenderVoxelDrawCallRangeID rangeID = this->drawCallRangesPool.alloc();
	if (rangeID < 0)
	{
		DebugLogError("Couldn't allocate draw call range ID.");
		return -1;
	}

	RenderVoxelDrawCallRange &drawCallRange = this->drawCallRangesPool.get(rangeID);
	if (!this->freedDrawCalls.empty())
	{
		// Find a suitable sequence of free draw call slots.
		int sequenceStartIndex = -1;
		int sequenceLength = 0;
		for (int i = 0; i < static_cast<int>(this->freedDrawCalls.size()); i++)
		{
			const int freedIndex = this->freedDrawCalls[i];
			if (i > 0)
			{
				const int prevFreedIndex = this->freedDrawCalls[i - 1];
				if (prevFreedIndex != (freedIndex - 1))
				{
					sequenceLength = 0;
				}
			}

			sequenceLength++;
			if (sequenceLength == drawCallCount)
			{
				sequenceStartIndex = freedIndex - (sequenceLength - 1);
				const auto eraseBegin = this->freedDrawCalls.begin() + (i - (sequenceLength - 1));
				const auto eraseEnd = eraseBegin + sequenceLength;
				this->freedDrawCalls.erase(eraseBegin, eraseEnd);
				break;
			}
		}

		if (sequenceStartIndex >= 0)
		{
			drawCallRange.index = sequenceStartIndex;
			drawCallRange.count = drawCallCount;
		}
	}

	// Allocate from the end of the draw calls list if the freed list didn't have a suitable sequence.
	if (drawCallRange.count == 0) 
	{
		if ((this->nextDrawCall + drawCallCount) > MAX_DRAW_CALLS)
		{
			DebugLogError("Not enough draw calls available for request of " + std::to_string(drawCallCount) + ".");
			return -1;
		}

		drawCallRange.index = this->nextDrawCall;
		drawCallRange.count = drawCallCount;
		this->nextDrawCall += drawCallCount;
	}

	return rangeID;
}

void RenderVoxelDrawCallHeap::free(RenderVoxelDrawCallRangeID id)
{
	DebugAssert(id >= 0);

	if (this->drawCallRangesPool.isFreedKey(id))
	{
		DebugLogWarning("Already freed draw call range ID " + std::to_string(id) + ".");
		return;
	}

	RenderVoxelDrawCallRange &drawCallRange = this->drawCallRangesPool.get(id);
	for (int i = 0; i < drawCallRange.count; i++)
	{
		const int drawCallIndexToFree = drawCallRange.index + i;

		DebugAssertIndex(this->drawCalls, drawCallIndexToFree);
		RenderDrawCall &drawCallToFree = this->drawCalls[drawCallIndexToFree];
		drawCallToFree.clear();

		// Insert so the freed draw calls stay sorted/coalesced.
		const auto insertIter = std::lower_bound(this->freedDrawCalls.begin(), this->freedDrawCalls.end(), drawCallIndexToFree);
		this->freedDrawCalls.insert(insertIter, drawCallIndexToFree);
	}

	this->drawCallRangesPool.free(id);
}

void RenderVoxelDrawCallHeap::clear()
{
	for (RenderDrawCall &drawCall : this->drawCalls)
	{
		drawCall.clear();
	}

	this->freedDrawCalls.clear();
	this->nextDrawCall = 0;

	this->drawCallRangesPool.clear();
}

RenderVoxelCombinedFaceTransformKey::RenderVoxelCombinedFaceTransformKey()
{
	this->facing = static_cast<VoxelFacing3D>(-1);
}

RenderVoxelCombinedFaceDrawCallEntry::RenderVoxelCombinedFaceDrawCallEntry()
{
	this->rangeID = -1;
}

bool RenderVoxelCombinedFaceTransformKey::operator==(const RenderVoxelCombinedFaceTransformKey &other) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->minVoxel != other.minVoxel)
	{
		return false;
	}

	if (this->maxVoxel != other.maxVoxel)
	{
		return false;
	}

	if (this->facing != other.facing)
	{
		return false;
	}

	return true;
}

void RenderVoxelChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);

	this->meshInstMappings.emplace(VoxelChunk::AIR_SHAPE_DEF_ID, RenderVoxelChunk::AIR_MESH_INST_ID);

	this->transformBufferID = -1;
	
	this->drawCallRangeIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->drawCallRangeIDs.fill(-1);

	// Add empty mesh instance for air.
	this->addMeshInst(RenderMeshInstance());
}

RenderMeshInstID RenderVoxelChunk::addMeshInst(RenderMeshInstance &&meshInst)
{
	const RenderMeshInstID id = static_cast<RenderMeshInstID>(this->meshInsts.size());
	this->meshInsts.emplace_back(std::move(meshInst));
	return id;
}

void RenderVoxelChunk::freeDrawCalls(SNInt x, int y, WEInt z)
{
	const RenderVoxelDrawCallRangeID rangeID = this->drawCallRangeIDs.get(x, y, z);
	if (rangeID >= 0)
	{
		this->drawCallHeap.free(rangeID);
		this->drawCallRangeIDs.set(x, y, z, -1);
	}
}

void RenderVoxelChunk::freeBuffers(Renderer &renderer)
{
	for (RenderMeshInstance &meshInst : this->meshInsts)
	{
		meshInst.freeBuffers(renderer);
	}

	for (auto &pair : this->combinedFaceTransforms)
	{
		UniformBufferID &transformBufferID = pair.second;

		if (transformBufferID >= 0)
		{
			renderer.freeUniformBuffer(transformBufferID);
			transformBufferID = -1;
		}
	}

	if (this->transformBufferID >= 0)
	{
		renderer.freeUniformBuffer(this->transformBufferID);
		this->transformBufferID = -1;
	}

	for (const auto &pair : this->doorTransformBuffers)
	{
		renderer.freeUniformBuffer(pair.second);
	}
}

void RenderVoxelChunk::clear()
{
	Chunk::clear();
	this->meshInsts.clear();
	this->meshInstMappings.clear();
	this->combinedFaceDrawCallEntries.clear();
	this->combinedFaceTransforms.clear();
	this->transformBufferID = -1;
	this->doorTransformBuffers.clear();
	this->drawCallHeap.clear();
	this->drawCallRangeIDs.clear();
}
