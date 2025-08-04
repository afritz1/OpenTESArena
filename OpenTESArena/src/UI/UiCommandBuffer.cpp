#include "UiCommandBuffer.h"
#include "UiDrawCall.h"

#include "components/debug/Debug.h"

UiCommandBuffer::UiCommandBuffer()
{
	this->clear();
}

int UiCommandBuffer::getTotalDrawCallCount() const
{
	int count = 0;
	for (int i = 0; i < this->entryCount; i++)
	{
		count += this->entries[i].getCount();
	}

	return count;
}

void UiCommandBuffer::addDrawCalls(Span<const UiDrawCall> drawCalls)
{
	if (this->entryCount >= static_cast<int>(std::size(this->entries)))
	{
		DebugLogErrorFormat("Too many entries in UI command buffer, can't add range of %d draw call(s).", drawCalls.getCount());
		return;
	}

	this->entries[this->entryCount] = drawCalls;
	this->entryCount++;
}

void UiCommandBuffer::clear()
{
	this->entryCount = 0;
}
