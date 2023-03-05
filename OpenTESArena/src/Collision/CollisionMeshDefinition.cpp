#include <algorithm>

#include "CollisionMeshDefinition.h"

CollisionMeshDefinition::CollisionMeshDefinition()
{
	this->triangleCount = -1;
}

void CollisionMeshDefinition::init(const BufferView<const double> &vertices, const BufferView<const double> &normals,
	const BufferView<const int> &indices)
{
	this->vertices.init(vertices.getCount());
	std::copy(vertices.begin(), vertices.end(), this->vertices.begin());

	this->normals.init(normals.getCount());
	std::copy(normals.begin(), normals.end(), this->normals.begin());

	this->indices.init(indices.getCount());
	std::copy(indices.begin(), indices.end(), this->indices.begin());

	DebugAssert((indices.getCount() % INDICES_PER_TRIANGLE) == 0); // Needs to be in position XYZ + normal XYZ tuple format.
	this->triangleCount = indices.getCount() / INDICES_PER_TRIANGLE;
}
