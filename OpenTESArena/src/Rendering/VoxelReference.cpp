#include "VoxelReference.h"

VoxelReference::VoxelReference(int32_t offset, int32_t count)
{
	this->offset = offset;
	this->count = count;
}

VoxelReference::~VoxelReference()
{

}

int32_t VoxelReference::getOffset() const
{
	return this->offset;
}

int32_t VoxelReference::getRectangleCount() const
{
	return this->count;
}
