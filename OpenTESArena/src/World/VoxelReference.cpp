#include "VoxelReference.h"

VoxelReference::VoxelReference(int offset, int count)
{
	this->offset = offset;
	this->count = count;
}

VoxelReference::~VoxelReference()
{

}

const int &VoxelReference::getOffset() const
{
	return this->offset;
}

const int &VoxelReference::getTriangleCount() const
{
	return this->count;
}
