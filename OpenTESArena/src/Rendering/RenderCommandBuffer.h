#ifndef RENDER_COMMAND_BUFFER_H
#define RENDER_COMMAND_BUFFER_H

#include <vector>

#include "RenderDrawCall.h"

#include "components/utilities/BufferView.h"

struct RenderCommandBuffer
{
	static constexpr int MAX_ENTRIES = 4096; // Allowing a lot more than usual for large view distance involving puddles.

	// One per range of draw calls (voxels, entities, weather, sky, etc). Each range starts execution once the previous one
	// is complete, ensuring correctness in the final image. Meant for proper rendering of more involved effects like
	// screen-space reflections that impact the renderer's ability to multi-task.
	BufferView<const RenderDrawCall> entries[MAX_ENTRIES];
	int entryCount;

	RenderCommandBuffer();

	int getTotalDrawCallCount() const;

	void addDrawCalls(BufferView<const RenderDrawCall> drawCalls);
	void clear();
};

#endif
