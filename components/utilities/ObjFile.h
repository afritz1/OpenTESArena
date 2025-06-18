#ifndef OBJ_FILE_H
#define OBJ_FILE_H

#include <vector>

struct ObjVertex
{
	double positionX, positionY, positionZ;
	double normalX, normalY, normalZ;
	double texCoordU, texCoordV;

	ObjVertex();
};

// Wavefront .OBJ file for 3D meshes, intended for rendering.
struct ObjFile
{
	std::string materialName; // usemtl
	std::vector<ObjVertex> vertices;

	bool init(const char *filename);
};

#endif
