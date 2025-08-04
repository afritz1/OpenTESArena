#ifndef UI_COMMAND_BUFFER_H
#define UI_COMMAND_BUFFER_H

#include "components/utilities/Span.h"

struct UiDrawCall;

struct UiCommandBuffer
{
	static constexpr int MAX_ENTRIES = 8;

	// One per range of UI draw calls. Each range starts execution once the previous one is complete.
	Span<const UiDrawCall> entries[MAX_ENTRIES];
	int entryCount;

	UiCommandBuffer();

	int getTotalDrawCallCount() const;

	void addDrawCalls(Span<const UiDrawCall> drawCalls);
	void clear();
};

#endif
