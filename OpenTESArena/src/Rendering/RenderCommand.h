#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include "components/utilities/Span.h"

struct RenderDrawCall;

struct RenderCommandList
{
	static constexpr int MAX_ENTRIES = 16;

	// One per range of draw calls (voxels, entities, weather, sky, etc). Each range starts execution once the previous one
	// is complete, ensuring correctness in the final image. Meant for proper rendering of more involved effects like
	// screen-space reflections that impact the renderer's ability to multi-task.
	Span<const RenderDrawCall> entries[MAX_ENTRIES];
	int entryCount;

	RenderCommandList();

	int getTotalDrawCallCount() const;

	void addDrawCalls(Span<const RenderDrawCall> drawCalls);
};

#endif
