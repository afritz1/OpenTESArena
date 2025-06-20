#ifndef MESH_LIBRARY_H
#define MESH_LIBRARY_H

#include <optional>
#include <string>
#include <vector>

#include "components/utilities/ObjFile.h"
#include "components/utilities/Singleton.h"
#include "components/utilities/Span.h"

enum class ArenaVoxelType;
enum class VoxelFacing3D;

struct MeshLibraryEntry
{
	ArenaVoxelType voxelType;
	std::vector<ObjVertex> vertices;
	std::vector<int32_t> vertexIndices;
	std::string materialName;
	int textureSlotIndex;
	std::optional<VoxelFacing3D> facing;

	MeshLibraryEntry();
};

class MeshLibrary : public Singleton<MeshLibrary>
{
public:
	std::vector<MeshLibraryEntry> entries;

	bool init(const char *folderPath);

	Span<const MeshLibraryEntry> getEntriesOfType(ArenaVoxelType voxelType) const;
	const MeshLibraryEntry *getEntryWithTypeAndFacing(ArenaVoxelType voxelType, VoxelFacing3D facing) const;
};

#endif
