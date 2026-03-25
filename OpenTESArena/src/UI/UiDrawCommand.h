#ifndef UI_DRAW_COMMAND_H
#define UI_DRAW_COMMAND_H

#include "components/utilities/Span.h"

struct RenderElement2D;

struct UiDrawCommandList
{
	static constexpr int MAX_ENTRIES = 8;

	// One per range of UI shapes to draw. Each range starts execution once the previous one is complete.
	Span<const RenderElement2D> entries[MAX_ENTRIES];
	int entryCount;

	UiDrawCommandList();

	int getTotalElementCount() const;

	void addElements(Span<const RenderElement2D> elements);
	void clear();
};

#endif
