#ifndef BUFFER_VIEW_2D_H
#define BUFFER_VIEW_2D_H

#include <algorithm>
#include <type_traits>

#include "../debug/Debug.h"

// Non-owning view over a 2D range of data stored in memory as a 1D array. More complex than 1D buffer
// view due to the look-up requirements of a 2D array.

// Data can be null. Only need assertions on things that reach into the buffer itself.

template <typename T>
class BufferView2D
{
private:
	T *data; // Start of original 2D array.
	int width, height; // Dimensions of original 2D array.
	int viewX, viewY; // View coordinates.
	int viewWidth, viewHeight; // View dimensions.

	int getIndex(int x, int y) const
	{
		DebugAssert(x >= 0);
		DebugAssert(y >= 0);
		DebugAssert(x < this->viewWidth);
		DebugAssert(y < this->viewHeight);
		return (viewX + x) + ((viewY + y) * this->width);
	}
public:
	BufferView2D()
	{
		this->reset();
	}

	// View across a subset of a 2D range of data. The original 2D range's dimensions are required
	// for proper look-up (and bounds-checking).
	BufferView2D(T *data, int width, int height, int viewX, int viewY, int viewWidth, int viewHeight)
	{
		this->init(data, width, height, viewX, viewY, viewWidth, viewHeight);
	}

	// View across a 2D range of data.
	BufferView2D(T *data, int width, int height)
	{
		this->init(data, width, height);
	}

	void init(T *data, int width, int height, int viewX, int viewY, int viewWidth, int viewHeight)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		DebugAssert(viewX >= 0);
		DebugAssert(viewY >= 0);
		DebugAssert(viewWidth >= 0);
		DebugAssert(viewHeight >= 0);
		DebugAssert((viewX + viewWidth) <= width);
		DebugAssert((viewY + viewHeight) <= height);
		this->data = data;
		this->width = width;
		this->height = height;
		this->viewX = viewX;
		this->viewY = viewY;
		this->viewWidth = viewWidth;
		this->viewHeight = viewHeight;
	}

	void init(T *data, int width, int height)
	{
		this->init(data, width, height, 0, 0, width, height);
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	T &get(int x, int y)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		return this->data[index];
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

	void set(int x, int y, const T &value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		this->data[index] = value;
	}

	void set(int x, int y, T &&value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y);
		this->data[index] = std::move(value);
	}

	void fill(const T &value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");
		
		DebugAssert(this->isValid());

		for (int y = 0; y < this->viewHeight; y++)
		{
			// Elements in a row are adjacent in memory.
			T *startPtr = this->data + this->getIndex(0, y);
			T *endPtr = startPtr + this->viewWidth;
			std::fill(startPtr, endPtr, value);
		}
	}

	void reset()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
		this->viewX = 0;
		this->viewY = 0;
		this->viewWidth = 0;
		this->viewHeight = 0;
	}
};

#endif
