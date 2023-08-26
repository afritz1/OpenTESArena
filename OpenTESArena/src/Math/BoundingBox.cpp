#include "BoundingBox.h"

#include "components/debug/Debug.h"

BoundingBox3D::BoundingBox3D()
{
	this->width = 0.0;
	this->height = 0.0;
	this->depth = 0.0;
	this->halfWidth = 0.0;
	this->halfHeight = 0.0;
	this->halfDepth = 0.0;
}

void BoundingBox3D::init(const Double3 &min, const Double3 &max)
{
	DebugAssert(min.x <= max.x);
	DebugAssert(min.y <= max.y);
	DebugAssert(min.z <= max.z);
	this->min = min;
	this->max = max;
	this->width = max.x - min.x;
	this->height = max.y - min.y;
	this->depth = max.z - min.z;
	this->halfWidth = this->width * 0.50;
	this->halfHeight = this->height * 0.50;
	this->halfDepth = this->depth * 0.50;
}

void BoundingBox3D::init(const Double3 &center, double width, double height, double depth)
{
	DebugAssert(width >= 0);
	DebugAssert(height >= 0);
	DebugAssert(depth >= 0);
	this->halfWidth = width * 0.50;
	this->halfHeight = height * 0.50;
	this->halfDepth = depth * 0.50;
	this->min = Double3(center.x - this->halfWidth, center.y - this->halfHeight, center.z - this->halfDepth);
	this->max = Double3(center.x + this->halfWidth, center.y + this->halfHeight, center.z + this->halfDepth);
	this->width = width;
	this->height = height;
	this->depth = depth;
}

bool BoundingBox3D::contains(const Double3 &point) const
{
	const bool containsX = (point.x >= this->min.x) && (point.x <= this->max.x);
	const bool containsY = (point.y >= this->min.y) && (point.y <= this->max.y);
	const bool containsZ = (point.z >= this->min.z) && (point.z <= this->max.z);
	return containsX && containsY && containsZ;
}

bool BoundingBox3D::contains(const BoundingBox3D &bbox) const
{
	const bool containsX = (bbox.min.x >= this->min.x) && (bbox.max.x <= this->max.x);
	const bool containsY = (bbox.min.y >= this->min.y) && (bbox.max.y <= this->max.y);
	const bool containsZ = (bbox.min.z >= this->min.z) && (bbox.max.z <= this->max.z);
	return containsX && containsY && containsZ;
}

void BoundingBox3D::expandToInclude(const Double3 &point)
{
	if (point.x < this->min.x)
	{
		this->min.x = point.x;
	}
	else if (point.x > this->max.x)
	{
		this->max.x = point.x;
	}

	if (point.y < this->min.y)
	{
		this->min.y = point.y;
	}
	else if (point.y > this->max.y)
	{
		this->max.y = point.y;
	}

	if (point.z < this->min.z)
	{
		this->min.z = point.z;
	}
	else if (point.z > this->max.z)
	{
		this->max.z = point.z;
	}

	this->width = this->max.x - this->min.x;
	this->height = this->max.y - this->min.y;
	this->depth = this->max.z - this->min.z;
	this->halfWidth = this->width * 0.50;
	this->halfHeight = this->height * 0.50;
	this->halfDepth = this->depth * 0.50;
}

void BoundingBox3D::expandToInclude(const BoundingBox3D &bbox)
{
	if (bbox.min.x < this->min.x)
	{
		this->min.x = bbox.min.x;
	}
	
	if (bbox.max.x > this->max.x)
	{
		this->max.x = bbox.max.x;
	}

	if (bbox.min.y < this->min.y)
	{
		this->min.y = bbox.min.y;
	}
	
	if (bbox.max.y > this->max.y)
	{
		this->max.y = bbox.max.y;
	}

	if (bbox.min.z < this->min.z)
	{
		this->min.z = bbox.min.z;
	}
	
	if (bbox.max.z > this->max.z)
	{
		this->max.z = bbox.max.z;
	}

	this->width = this->max.x - this->min.x;
	this->height = this->max.y - this->min.y;
	this->depth = this->max.z - this->min.z;
	this->halfWidth = this->width * 0.50;
	this->halfHeight = this->height * 0.50;
	this->halfDepth = this->depth * 0.50;
}
