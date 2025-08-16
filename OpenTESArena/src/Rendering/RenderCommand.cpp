#include "RenderCommand.h"
#include "RenderDrawCall.h"

#include "components/debug/Debug.h"

RenderCommandList::RenderCommandList()
{
	this->clear();
}

int RenderCommandList::getTotalDrawCallCount() const
{
	int count = 0;
	for (int i = 0; i < this->entryCount; i++)
	{
		count += this->entries[i].getCount();
	}

	return count;
}

void RenderCommandList::addDrawCalls(Span<const RenderDrawCall> drawCalls)
{
	if (drawCalls.getCount() == 0)
	{
		return;
	}

	if (this->entryCount >= static_cast<int>(std::size(this->entries)))
	{
		DebugLogErrorFormat("Too many entries in command buffer, can't add range of %d draw call(s).", drawCalls.getCount());
		return;
	}

	this->entries[this->entryCount] = drawCalls;
	this->entryCount++;
}

void RenderCommandList::clear()
{
	this->entryCount = 0;
}
