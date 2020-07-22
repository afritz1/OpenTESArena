#ifndef BUFFER_REF_2D_H
#define BUFFER_REF_2D_H

#include "../debug/Debug.h"

// This only exists because dangling pointers are bad and classes like the texture manager
// shouldn't return raw texture references when it knows that the reference could become
// invalidated by a call to one of the manager's other functions.

// Intended for 2D image-like buffers.

template <typename ContainerT, typename T>
class BufferRef2D
{
private:
	ContainerT *container;
	int index;
public:
	BufferRef2D(ContainerT *container, int index)
	{
		this->container = container;
		this->index = index;
	}

	T &get()
	{
		ContainerT &containerRef = *this->container;
		DebugAssertIndex(containerRef, this->index);
		return containerRef[this->index];
	}

	const T &get() const
	{
		const ContainerT &containerRef = *this->container;
		DebugAssertIndex(containerRef, this->index);
		return containerRef[this->index];
	}

	int getWidth() const
	{
		return this->get().getWidth();
	}

	int getHeight() const
	{
		return this->get().getHeight();
	}
};

#endif
