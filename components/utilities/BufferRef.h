#ifndef BUFFER_REF_H
#define BUFFER_REF_H

#include "../debug/Debug.h"

template <typename ContainerT, typename T>
class BufferRef
{
private:
	ContainerT *container;
	int index;
public:
	BufferRef(ContainerT *container, int index)
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

	int getCount() const
	{
		return static_cast<int>(std::size(this->get()));
	}
};

#endif
