#include "RenderCommandBuffer.h"
#include "RenderDrawCall.h"

#include "components/debug/Debug.h"

RenderCommandBuffer::RenderCommandBuffer()
{
	this->clear();
}

int RenderCommandBuffer::getTotalDrawCallCount() const
{
	int count = 0;
	for (int i = 0; i < this->entryCount; i++)
	{
		count += this->entries[i].getCount();
	}

	return count;
}

void RenderCommandBuffer::addDrawCalls(Span<const RenderDrawCall> drawCalls)
{
	if (this->entryCount >= static_cast<int>(std::size(this->entries)))
	{
		DebugLogError("Too many entries in command buffer, can't add range of " + std::to_string(drawCalls.getCount()) + " draw call(s).");
		return;
	}

	this->entries[this->entryCount] = drawCalls;
	this->entryCount++;
}

void RenderCommandBuffer::clear()
{
	this->entryCount = 0;
}
