#include "RenderBuffer.h"

#include "components/debug/Debug.h"

LockedBuffer::LockedBuffer()
{
	this->bytesPerElement = 0;
}

LockedBuffer::LockedBuffer(Span<std::byte> bytes, int elementCount, int bytesPerElement, int bytesPerStride)
{
	this->bytes = bytes;
	this->elementCount = elementCount;
	this->bytesPerElement = bytesPerElement;
	this->bytesPerStride = bytesPerStride;
}

bool LockedBuffer::isValid() const
{
	return this->bytes.isValid();
}

bool LockedBuffer::isContiguous() const
{
	return (this->elementCount == 1) || (this->bytesPerElement == this->bytesPerStride);
}

Span<int> LockedBuffer::getInts()
{
	DebugAssert(this->isContiguous());
	DebugAssert(this->bytesPerElement == sizeof(int));
	return Span<int>(reinterpret_cast<int*>(this->bytes.begin()), this->bytes.getCount() / sizeof(int));
}

Span<float> LockedBuffer::getFloats()
{
	DebugAssert(this->isContiguous());
	DebugAssert(this->bytesPerElement == sizeof(float));
	return Span<float>(reinterpret_cast<float*>(this->bytes.begin()), this->bytes.getCount() / sizeof(float));
}

Span<double> LockedBuffer::getDoubles()
{
	DebugAssert(this->isContiguous());
	DebugAssert(this->bytesPerElement == sizeof(double));
	return Span<double>(reinterpret_cast<double*>(this->bytes.begin()), this->bytes.getCount() / sizeof(double));
}
