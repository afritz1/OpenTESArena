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
	std::copy(vertices.get(), vertices.end(), this->vertices.get());

	this->normals.init(normals.getCount());
	std::copy(normals.get(), normals.end(), this->normals.get());

	this->indices.init(indices.getCount());
	std::copy(indices.get(), indices.end(), this->indices.get());

	DebugAssert((indices.getCount() % INDICES_PER_TRIANGLE) == 0); // Needs to be in position XYZ + normal XYZ tuple format.
	this->triangleCount = indices.getCount() / INDICES_PER_TRIANGLE;
}
