#ifndef COLLISION_MESH_INSTANCE_H
#define COLLISION_MESH_INSTANCE_H

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

struct CollisionMeshInstance
{
	Buffer<double> vertices; // Minimum-required to represent mesh (no duplication).
	Buffer<double> normals; // One XYZ triplet per quad.
	Buffer<int> indices; // Tuple format (0: vertex XYZ, 1: normal XYZ, ...).

	void init(const BufferView<const double> &vertices, const BufferView<const double> &normals,
		const BufferView<const int> &indices);
};

#endif
