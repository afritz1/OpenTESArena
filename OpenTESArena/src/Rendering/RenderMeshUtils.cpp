#include <algorithm>

#include "RenderMeshUtils.h"

#include "components/debug/Debug.h"

RenderTransformHeap::RenderTransformHeap()
{
	this->uniformBufferID = -1;
}

int RenderTransformHeap::alloc()
{
	int matrixIndex = this->pool.alloc();
	if (matrixIndex < 0)
	{
		DebugLogError("Can't allocate any more matrices in transform heap.");
	}

	return matrixIndex;
}

void RenderTransformHeap::free(int transformIndex)
{
	this->pool.free(transformIndex);
}

void RenderTransformHeap::clear()
{
	this->uniformBufferID = -1;
	this->pool.clear();
}
