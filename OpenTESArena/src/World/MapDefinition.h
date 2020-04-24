#ifndef MAP_DEFINITION_H
#define MAP_DEFINITION_H

// Modern replacement for .MIF files and .RMD files.

class MIFFile;
class RMDFile;

class MapDefinition
{
private:
	// @todo: read-only data that defines each level in an "unbaked" format (context-free voxels).
	// @todo: extending old features like entity position in a voxel to be anywhere
	// in the voxel (useful for grass, etc.).
public:
	void init(const MIFFile &mif);
	void init(const RMDFile &rmd);
};

#endif
