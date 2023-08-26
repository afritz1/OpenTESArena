#ifndef BOUNDING_BOX_H
#define BOUNDING_BOX_H

#include "Vector3.h"

struct BoundingBox3D
{
	Double3 min, max;
	double width, height, depth;

	BoundingBox3D();

	void init(const Double3 &min, const Double3 &max);

	bool contains(const Double3 &point) const;
	bool contains(const BoundingBox3D &bbox) const;
	
	void expandToInclude(const Double3 &point);
	void expandToInclude(const BoundingBox3D &bbox);
};

#endif
