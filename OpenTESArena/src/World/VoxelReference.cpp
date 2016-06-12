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

int VoxelReference::getTriangleCount() const
{
	return this->count;
}
