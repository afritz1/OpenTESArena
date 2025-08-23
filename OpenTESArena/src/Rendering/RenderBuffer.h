#ifndef RENDER_BUFFER_H
#define RENDER_BUFFER_H

#include <cstddef>

#include "components/utilities/Span.h"

struct LockedBuffer
{
	Span<std::byte> bytes;
	int elementCount;
	int bytesPerElement; // Requested at creation time.
	int bytesPerStride; // Potentially greater than bytes per element due to device alignment requirements.

	LockedBuffer();
	LockedBuffer(Span<std::byte> bytes, int elementCount, int bytesPerElement, int bytesPerStride);

	bool isValid() const;
	bool isContiguous() const;

	Span<int> getInts();
	Span<float> getFloats();
	Span<double> getDoubles();
};

#endif
