#include "UiDrawCommand.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiDrawCommandList::UiDrawCommandList()
{
	this->clear();
}

int UiDrawCommandList::getTotalElementCount() const
{
	int count = 0;
	for (int i = 0; i < this->entryCount; i++)
	{
		count += this->entries[i].getCount();
	}

	return count;
}

void UiDrawCommandList::addElements(Span<const RenderElement2D> elements)
{
	if (this->entryCount >= static_cast<int>(std::size(this->entries)))
	{
		DebugLogErrorFormat("Too many entries in UI command buffer, can't add range of %d element(s).", elements.getCount());
		return;
	}

	this->entries[this->entryCount] = elements;
	this->entryCount++;
}

void UiDrawCommandList::clear()
{
	this->entryCount = 0;
}
