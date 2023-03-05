#include <algorithm>

#include "ChunkUtils.h"
#include "ChunkManager.h"

#include "components/debug/Debug.h"

ChunkManager::ChunkManager()
{
	this->centerChunkPosIndex = -1;
}

BufferView<const ChunkInt2> ChunkManager::getActiveChunkPositions() const
{
	return BufferView<const ChunkInt2>(this->activeChunkPositions);
}

BufferView<const ChunkInt2> ChunkManager::getNewChunkPositions() const
{
	return BufferView<const ChunkInt2>(this->newChunkPositions);
}

BufferView<const ChunkInt2> ChunkManager::getFreedChunkPositions() const
{
	return BufferView<const ChunkInt2>(this->freedChunkPositions);
}

int ChunkManager::getCenterChunkIndex() const
{
	return this->centerChunkPosIndex;
}

std::optional<int> ChunkManager::tryGetChunkIndex(const ChunkInt2 &position) const
{
	const auto iter = std::find(this->activeChunkPositions.begin(), this->activeChunkPositions.end(), position);
	if (iter != this->activeChunkPositions.end())
	{
		return static_cast<int>(std::distance(this->activeChunkPositions.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

void ChunkManager::update(const ChunkInt2 &centerChunkPos, int chunkDistance)
{
	DebugAssert(chunkDistance > 0);
	DebugAssert(this->newChunkPositions.empty());
	DebugAssert(this->freedChunkPositions.empty());

	const int oldChunkCount = static_cast<int>(this->activeChunkPositions.size());
	const int newChunkCount = ChunkUtils::getChunkCount(chunkDistance);
	const bool activeChunksNeedUpdate = [this, &centerChunkPos, oldChunkCount, newChunkCount]()
	{
		if (this->centerChunkPosIndex < 0)
		{
			return true;
		}

		DebugAssertIndex(this->activeChunkPositions, this->centerChunkPosIndex);
		const ChunkInt2 oldCenterChunkPos = this->activeChunkPositions[this->centerChunkPosIndex];
		if (oldCenterChunkPos != centerChunkPos)
		{
			return true;
		}

		if (newChunkCount != oldChunkCount)
		{
			return true;
		}

		return false;
	}();

	if (!activeChunksNeedUpdate)
	{
		return;
	}

	for (int i = oldChunkCount - 1; i >= 0; i--)
	{
		const ChunkInt2 chunkPos = this->activeChunkPositions[i];
		if (!ChunkUtils::isWithinActiveRange(centerChunkPos, chunkPos, chunkDistance))
		{
			this->freedChunkPositions.emplace_back(chunkPos);
		}
	}

	ChunkInt2 minChunkPos, maxChunkPos;
	ChunkUtils::getSurroundingChunks(centerChunkPos, chunkDistance, &minChunkPos, &maxChunkPos);

	for (WEInt y = minChunkPos.y; y <= maxChunkPos.y; y++)
	{
		for (SNInt x = minChunkPos.x; x <= maxChunkPos.x; x++)
		{
			const ChunkInt2 chunkPos(x, y);
			const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunkPos);
			if (!chunkIndex.has_value())
			{
				this->newChunkPositions.emplace_back(chunkPos);
			}
		}
	}

	this->activeChunkPositions.clear();
	for (WEInt y = minChunkPos.y; y <= maxChunkPos.y; y++)
	{
		for (SNInt x = minChunkPos.x; x <= maxChunkPos.x; x++)
		{
			const ChunkInt2 chunkPos(x, y);
			if (chunkPos == centerChunkPos)
			{
				this->centerChunkPosIndex = static_cast<int>(this->activeChunkPositions.size());
			}

			this->activeChunkPositions.emplace_back(chunkPos);
		}
	}
}

void ChunkManager::cleanUp()
{
	this->newChunkPositions.clear();
	this->freedChunkPositions.clear();
}
