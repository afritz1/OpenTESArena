#ifndef BUFFER_VIEW_2D_READ_ONLY_H
#define BUFFER_VIEW_2D_READ_ONLY_H

#include <algorithm>
#include <type_traits>

#include "../debug/Debug.h"

// Non-owning view over a 2D range of data stored in memory as a 1D array. More complex than 1D buffer
// view due to the look-up requirements of a 2D array.

// Data can be null. Only need assertions on things that reach into the buffer itself.

template <typename T>
class BufferView2DReadOnly
{
private:
	const T *data; // Start of original 2D array.
	const int width, height; // Dimensions of original 2D array.
	const int viewX, viewY; // View coordinates.
	const int viewWidth, viewHeight; // View dimensions.

	int getIndex(int x, int y) const
	{
		DebugAssert(x >= 0);
		DebugAssert(y >= 0);
		DebugAssert(x < this->viewWidth);
		DebugAssert(y < this->viewHeight);
		return (viewX + x) + ((viewY + y) * this->width);
	}
public:
	BufferView2DReadOnly() : BufferView2DReadOnly(nullptr, 0, 0)
	{
	}

	// View across a subset of a 2D range of data. The original 2D range's dimensions are required
	// for proper look-up (and bounds-checking).
	BufferView2DReadOnly(const T *data, int width, int height, int viewX, int viewY, int viewWidth, int viewHeight) : data(data), width(width), height(height), viewX(viewX), viewY(viewY), viewWidth(viewWidth), viewHeight(viewHeight)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		DebugAssert(viewX >= 0);
		DebugAssert(viewY >= 0);
		DebugAssert(viewWidth >= 0);
		DebugAssert(viewHeight >= 0);
		DebugAssert((viewX + viewWidth) <= width);
		DebugAssert((viewY + viewHeight) <= height);
	}

	// View across a 2D range of data.
	BufferView2DReadOnly(const T *data, int width, int height) : BufferView2DReadOnly(data, width, height, 0, 0, width, height)
	{
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	const T &get(int x, int y) const
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		return this->data[index];
	}

	int getWidth() const
	{
		return this->viewWidth;
	}

	int getHeight() const
	{
		return this->viewHeight;
	}
};

#endif
