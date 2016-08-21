#include "VoxelReference.h"

VoxelReference::VoxelReference(int offset, int count)
{
	this->offset = offset;
	this->count = count;
}

VoxelReference::~VoxelReference()
{

}

int VoxelReference::getOffset() const
{
	return this->offset;
}

int VoxelReference::getRectangleCount() const
{
	return this->count;
}
