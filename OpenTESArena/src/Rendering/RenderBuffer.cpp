#include "RenderBuffer.h"

#include "components/debug/Debug.h"

LockedBuffer::LockedBuffer()
{
	this->bytesPerElement = 0;
}

LockedBuffer::LockedBuffer(Span<std::byte> bytes, int bytesPerElement)
{
	// bytes.count >= elements * bytesPerElement due to alignment taking extra bytes.
	this->bytes = bytes;
	this->bytesPerElement = bytesPerElement;
}

bool LockedBuffer::isValid() const
{
	return this->bytes.isValid();
}

Span<int> LockedBuffer::getInts()
{
	DebugAssert(this->bytesPerElement == sizeof(int));
	return Span<int>(reinterpret_cast<int*>(this->bytes.begin()), this->bytes.getCount() / sizeof(int));
}

Span<float> LockedBuffer::getFloats()
{
	DebugAssert(this->bytesPerElement == sizeof(float));
	return Span<float>(reinterpret_cast<float*>(this->bytes.begin()), this->bytes.getCount() / sizeof(float));
}

Span<double> LockedBuffer::getDoubles()
{
	DebugAssert(this->bytesPerElement == sizeof(double));
	return Span<double>(reinterpret_cast<double*>(this->bytes.begin()), this->bytes.getCount() / sizeof(double));
}
