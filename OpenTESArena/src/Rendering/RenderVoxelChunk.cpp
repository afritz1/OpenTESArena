#include <algorithm>

#include "Renderer.h"
#include "RendererSystem3D.h"
#include "RenderVoxelChunk.h"
#include "../World/ChunkUtils.h"

#include "components/utilities/BufferView3D.h"

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
	this->nextID = 0;
}

BufferView<RenderDrawCall> RenderVoxelDrawCallHeap::get(RenderVoxelDrawCallRangeID id)
{
	DebugAssertIndex(this->drawCallRanges, id);
	const RenderVoxelDrawCallRange &drawCallRange = this->drawCallRanges[id];

	RenderDrawCall *rangeBegin = std::begin(this->drawCalls) + drawCallRange.index;
	DebugAssert((rangeBegin + drawCallRange.count) <= std::end(this->drawCalls));
	return BufferView<RenderDrawCall>(rangeBegin, drawCallRange.count);
}

BufferView<const RenderDrawCall> RenderVoxelDrawCallHeap::get(RenderVoxelDrawCallRangeID id) const
{
	DebugAssertIndex(this->drawCallRanges, id);
	const RenderVoxelDrawCallRange &drawCallRange = this->drawCallRanges[id];

	const RenderDrawCall *rangeBegin = std::begin(this->drawCalls) + drawCallRange.index;
	DebugAssert((rangeBegin + drawCallRange.count) <= std::end(this->drawCalls));
	return BufferView<const RenderDrawCall>(rangeBegin, drawCallRange.count);
}

RenderVoxelDrawCallRangeID RenderVoxelDrawCallHeap::alloc(int drawCallCount)
{
	DebugAssert(drawCallCount > 0);

	RenderVoxelDrawCallRangeID rangeID;
	if (!this->freedIDs.empty())
	{
		rangeID = this->freedIDs.back();
		this->freedIDs.pop_back();
	}
	else
	{
		if (this->nextID == MAX_DRAW_CALL_RANGES)
		{
			DebugLogError("No more draw call range IDs available.");
			return -1;
		}

		rangeID = this->nextID;
		this->nextID++;
	}

	DebugAssertIndex(this->drawCallRanges, rangeID);
	RenderVoxelDrawCallRange &drawCallRange = this->drawCallRanges[rangeID];
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

RenderVoxelDrawCallRangeID RenderVoxelDrawCallHeap::addDrawCall(RenderVoxelDrawCallRangeID id)
{
	// @todo: design this better; maybe make a way of knowing ahead of time how many draw calls a voxel needs?
	const BufferView<RenderDrawCall> existingDrawCalls = this->get(id);
	const int existingDrawCallCount = existingDrawCalls.getCount();
	constexpr int maxDrawCallsPerVoxel = 8; // Arbitrary
	RenderDrawCall tempDrawCalls[maxDrawCallsPerVoxel];
	DebugAssert(existingDrawCallCount <= std::size(tempDrawCalls));
	std::copy(existingDrawCalls.begin(), existingDrawCalls.end(), std::begin(tempDrawCalls));

	this->free(id);
	const RenderVoxelDrawCallRangeID newID = this->alloc(existingDrawCallCount + 1);
	BufferView<RenderDrawCall> newDrawCalls = this->get(newID);
	std::copy(std::begin(tempDrawCalls), std::begin(tempDrawCalls) + existingDrawCallCount, newDrawCalls.begin());
	
	return newID;
}

void RenderVoxelDrawCallHeap::free(RenderVoxelDrawCallRangeID id)
{
	DebugAssert(id >= 0);

	const auto freedIdIter = std::find(this->freedIDs.begin(), this->freedIDs.end(), id);
	if (freedIdIter != this->freedIDs.end())
	{
		DebugLogWarning("Already freed draw call range ID " + std::to_string(id) + ".");
		return;
	}

	// Free the draw call slots.
	DebugAssertIndex(this->drawCallRanges, id);
	RenderVoxelDrawCallRange &drawCallRange = this->drawCallRanges[id];
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

	// Free the draw call range slot.
	drawCallRange.clear();
	this->freedIDs.push_back(id);
}

void RenderVoxelDrawCallHeap::clear()
{
	for (RenderDrawCall &drawCall : this->drawCalls)
	{
		drawCall.clear();
	}

	this->freedDrawCalls.clear();
	this->nextDrawCall = 0;

	for (RenderVoxelDrawCallRange &drawCallRange : this->drawCallRanges)
	{
		drawCallRange.clear();
	}

	this->freedIDs.clear();
	this->nextID = 0;
}

void RenderVoxelChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->meshInstIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->meshInstIDs.fill(RenderVoxelChunk::AIR_MESH_INST_ID);
	this->meshInstMappings.emplace(VoxelChunk::AIR_MESH_DEF_ID, RenderVoxelChunk::AIR_MESH_INST_ID);
	
	this->staticDrawCallRangeIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->staticDrawCallRangeIDs.fill(-1);

	this->doorDrawCallRangeIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->doorDrawCallRangeIDs.fill(-1);

	this->chasmDrawCallRangeIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->chasmDrawCallRangeIDs.fill(-1);

	this->fadingDrawCallRangeIDs.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->fadingDrawCallRangeIDs.fill(-1);

	// Add empty mesh instance for air.
	this->addMeshInst(RenderVoxelMeshInstance());
}

RenderVoxelMeshInstID RenderVoxelChunk::addMeshInst(RenderVoxelMeshInstance &&meshInst)
{
	const RenderVoxelMeshInstID id = static_cast<RenderVoxelMeshInstID>(this->meshInsts.size());
	this->meshInsts.emplace_back(std::move(meshInst));
	return id;
}

void RenderVoxelChunk::freeStaticDrawCalls()
{
	if (this->staticDrawCallRangeIDs.isValid())
	{
		for (WEInt z = 0; z < this->staticDrawCallRangeIDs.getDepth(); z++)
		{
			for (int y = 0; y < this->staticDrawCallRangeIDs.getHeight(); y++)
			{
				for (SNInt x = 0; x < this->staticDrawCallRangeIDs.getWidth(); x++)
				{
					const RenderVoxelDrawCallRangeID rangeID = this->staticDrawCallRangeIDs.get(x, y, z);
					if (rangeID >= 0)
					{
						this->drawCallHeap.free(rangeID);
					}

					this->staticDrawCallRangeIDs.set(x, y, z, -1);
				}
			}
		}
	}
}

void RenderVoxelChunk::freeAnimatingDrawCalls()
{
	auto freeRangeIDs = [this](BufferView3D<RenderVoxelDrawCallRangeID> rangeIDs)
	{
		if (rangeIDs.isValid())
		{
			for (WEInt z = 0; z < rangeIDs.getDepth(); z++)
			{
				for (int y = 0; y < rangeIDs.getHeight(); y++)
				{
					for (SNInt x = 0; x < rangeIDs.getWidth(); x++)
					{
						const RenderVoxelDrawCallRangeID rangeID = rangeIDs.get(x, y, z);
						if (rangeID >= 0)
						{
							this->drawCallHeap.free(rangeID);
						}

						rangeIDs.set(x, y, z, -1);
					}
				}
			}
		}
	};

	freeRangeIDs(this->doorDrawCallRangeIDs);
	freeRangeIDs(this->chasmDrawCallRangeIDs);
	freeRangeIDs(this->fadingDrawCallRangeIDs);
}

void RenderVoxelChunk::freeDrawCalls(SNInt x, int y, WEInt z)
{
	auto tryFreeRangeID = [this, x, y, z](BufferView3D<RenderVoxelDrawCallRangeID> ranges)
	{
		const RenderVoxelDrawCallRangeID rangeID = ranges.get(x, y, z);
		if (rangeID >= 0)
		{
			this->drawCallHeap.free(rangeID);
			ranges.set(x, y, z, -1);
		}
	};

	tryFreeRangeID(this->staticDrawCallRangeIDs);
	tryFreeRangeID(this->doorDrawCallRangeIDs);
	tryFreeRangeID(this->chasmDrawCallRangeIDs);
	tryFreeRangeID(this->fadingDrawCallRangeIDs);
}

void RenderVoxelChunk::freeBuffers(Renderer &renderer)
{
	for (RenderVoxelMeshInstance &meshInst : this->meshInsts)
	{
		meshInst.freeBuffers(renderer);
	}

	for (const auto &pair : this->transformBuffers)
	{
		renderer.freeUniformBuffer(pair.second);
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
	this->meshInstIDs.clear();
	this->chasmWallIndexBufferIDsMap.clear();
	this->transformBuffers.clear();
	this->doorTransformBuffers.clear();
	this->drawCallHeap.clear();
	this->staticDrawCallRangeIDs.clear();
	this->doorDrawCallRangeIDs.clear();
	this->chasmDrawCallRangeIDs.clear();
	this->fadingDrawCallRangeIDs.clear();
}
