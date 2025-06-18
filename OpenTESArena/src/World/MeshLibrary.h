#ifndef MESH_LIBRARY_H
#define MESH_LIBRARY_H

#include <string>
#include <vector>

#include "components/utilities/ObjFile.h"
#include "components/utilities/Singleton.h"

enum class ArenaVoxelType;

struct MeshLibraryEntry
{
	ArenaVoxelType voxelType;
	std::vector<ObjVertex> vertices;
	std::string materialName;
	int textureSlotIndex;

	MeshLibraryEntry();
};

class MeshLibrary : public Singleton<MeshLibrary>
{
public:
	std::vector<MeshLibraryEntry> entries;

	bool init(const char *folderPath);

	std::vector<const MeshLibraryEntry*> getEntriesOfType(ArenaVoxelType voxelType) const;
};

#endif
