#ifndef RENDER_BUFFER_H
#define RENDER_BUFFER_H

#include <cstddef>

#include "components/utilities/Span.h"

struct LockedBuffer
{
	Span<std::byte> bytes;
	int bytesPerElement;

	LockedBuffer();
	LockedBuffer(Span<std::byte> bytes, int bytesPerElement);

	bool isValid() const;

	Span<int> getInts();
	Span<float> getFloats();
	Span<double> getDoubles();
};

#endif
