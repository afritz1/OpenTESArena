#ifndef BUFFER_VIEW_3D_H
#define BUFFER_VIEW_3D_H

#include <algorithm>
#include <type_traits>

#include "../debug/Debug.h"

// Non-owning view over a 3D range of data stored in memory as a 1D array. More complex than 2D buffer
// view due to the look-up requirements of a 3D array.

// Data can be null. Only need assertions on things that reach into the buffer itself.

template <typename T>
class BufferView3D
{
private:
	T *data; // Start of original 3D array.
	int width, height, depth; // Dimensions of original 3D array.
	int viewX, viewY, viewZ; // View coordinates.
	int viewWidth, viewHeight, viewDepth; // View dimensions.

	int getIndex(int x, int y, int z) const
	{
		DebugAssert(x >= 0);
		DebugAssert(y >= 0);
		DebugAssert(z >= 0);
		DebugAssert(x < this->viewWidth);
		DebugAssert(y < this->viewHeight);
		DebugAssert(z < this->viewDepth);
		return (viewX + x) + ((viewY + y) * this->width) +
			((viewZ + z) * this->width * this->height);
	}
public:
	BufferView3D()
	{
		this->reset();
	}

	// View across a subset of a 3D range of data. The original 3D range's dimensions are required
	// for proper look-up (and bounds-checking).
	BufferView3D(T *data, int width, int height, int depth, int viewX, int viewY, int viewZ,
		int viewWidth, int viewHeight, int viewDepth)
	{
		this->init(data, width, height, depth, viewX, viewY, viewZ, viewWidth, viewHeight, viewDepth);
	}

	// View across a 3D range of data.
	BufferView3D(T *data, int width, int height, int depth)
	{
		this->init(data, width, height, depth);
	}

	void init(T *data, int width, int height, int depth, int viewX, int viewY, int viewZ,
		int viewWidth, int viewHeight, int viewDepth)
	{
		DebugAssert(width >= 0);
		DebugAssert(height >= 0);
		DebugAssert(depth >= 0);
		DebugAssert(viewX >= 0);
		DebugAssert(viewY >= 0);
		DebugAssert(viewZ >= 0);
		DebugAssert(viewWidth >= 0);
		DebugAssert(viewHeight >= 0);
		DebugAssert(viewDepth >= 0);
		DebugAssert((viewX + viewWidth) <= width);
		DebugAssert((viewY + viewHeight) <= height);
		DebugAssert((viewZ + viewDepth) <= depth);
		this->data = data;
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->viewX = viewX;
		this->viewY = viewY;
		this->viewZ = viewZ;
		this->viewWidth = viewWidth;
		this->viewHeight = viewHeight;
		this->viewDepth = viewDepth;
	}

	void init(T *data, int width, int height, int depth)
	{
		this->init(data, width, height, depth, 0, 0, 0, width, height, depth);
	}

	bool isValid() const
	{
		return this->data != nullptr;
	}

	T &get(int x, int y, int z)
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		return this->data[index];
	}

	const T &get(int x, int y, int z) const
	{
		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
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

	int getDepth() const
	{
		return this->viewDepth;
	}

	void set(int x, int y, int z, const T &value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		this->data[index] = value;
	}

	void set(int x, int y, int z, T &&value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());
		const int index = this->getIndex(x, y, z);
		this->data[index] = std::move(value);
	}

	void fill(const T &value)
	{
		static_assert(!std::is_const_v<T>, "Cannot change const data.");

		DebugAssert(this->isValid());

		for (int z = 0; z < this->viewDepth; z++)
		{
			for (int y = 0; y < this->viewHeight; y++)
			{
				// Elements in a row are adjacent in memory.
				T *startPtr = this->data + this->getIndex(0, y, z);
				T *endPtr = startPtr + this->viewWidth;
				std::fill(startPtr, endPtr, value);
			}
		}
	}

	void reset()
	{
		this->data = nullptr;
		this->width = 0;
		this->height = 0;
		this->depth = 0;
		this->viewX = 0;
		this->viewY = 0;
		this->viewZ = 0;
		this->viewWidth = 0;
		this->viewHeight = 0;
		this->viewDepth = 0;
	}
};

#endif
