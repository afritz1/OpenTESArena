#ifndef UI_COMMAND_H
#define UI_COMMAND_H

#include "components/utilities/Span.h"

struct UiDrawCall;

struct UiCommandList
{
	static constexpr int MAX_ENTRIES = 8;

	// One per range of UI draw calls. Each range starts execution once the previous one is complete.
	Span<const UiDrawCall> entries[MAX_ENTRIES];
	int entryCount;

	UiCommandList();

	int getTotalDrawCallCount() const;

	void addDrawCalls(Span<const UiDrawCall> drawCalls);
	void clear();
};

#endif
