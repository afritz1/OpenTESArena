#include <algorithm>
#include <string>

#include "MeshLibrary.h"
#include "../Assets/ArenaTypes.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"

namespace
{
	constexpr std::pair<const char*, int> MaterialNameTextureSlots[] =
	{
		{ "Ceiling", 0 },
		{ "ChasmFloor", -1 }, // Screen-space texture not part of voxel texture definitions
		{ "ChasmWall", 0 },
		{ "Diagonal", 0 },
		{ "Door", 0 },
		{ "Edge", 0 },
		{ "Floor", 0 },
		{ "RaisedSide", 0 },
		{ "RaisedBottom", 1 },
		{ "RaisedTop", 2 },
		{ "TransparentWall", 0 },
		{ "WallSide", 0 },
		{ "WallBottom", 1 },
		{ "WallTop", 2 }
	};
}

MeshLibraryEntry::MeshLibraryEntry()
{
	this->voxelType = static_cast<ArenaVoxelType>(-1);
	this->textureSlotIndex = -1;
}

bool MeshLibrary::init(const char *folderPath)
{
	std::vector<std::string> objFilenames = Directory::getFilesWithExtension(folderPath, ".obj");
	if (objFilenames.empty())
	{
		return false;
	}

	std::string voxelTypeNames[ARENA_VOXEL_TYPE_COUNT];
	for (int i = 0; i < ARENA_VOXEL_TYPE_COUNT; i++)
	{
		voxelTypeNames[i] = ArenaTypes::voxelTypeToString(static_cast<ArenaVoxelType>(i));
	}

	for (const std::string &objFilename : objFilenames)
	{
		ObjFile objFile;
		if (!objFile.init(objFilename.c_str()))
		{
			DebugLogErrorFormat("Couldn't add .OBJ file \"%s\" to mesh library.", objFilename.c_str());
			continue;
		}

		const std::string &materialName = objFile.materialName;

		ArenaVoxelType voxelType = ArenaVoxelType::None;
		for (int voxelTypeIndex = 0; voxelTypeIndex < ARENA_VOXEL_TYPE_COUNT; voxelTypeIndex++)
		{
			const std::string &voxelTypeName = voxelTypeNames[voxelTypeIndex];
			if (materialName.starts_with(voxelTypeName))
			{
				voxelType = static_cast<ArenaVoxelType>(voxelTypeIndex);
			}
		}

		if (voxelType == ArenaVoxelType::None)
		{
			DebugLogErrorFormat("Material name \"%s\" in .OBJ file \"%s\" doesn't match any voxel types.", materialName.c_str(), objFilename.c_str());
			continue;
		}

		MeshLibraryEntry entry;
		entry.voxelType = voxelType;
		entry.vertices = std::move(objFile.vertices);
		entry.vertexIndices = std::move(objFile.indices);
		entry.materialName = materialName;

		const auto textureIter = std::find_if(std::begin(MaterialNameTextureSlots), std::end(MaterialNameTextureSlots),
			[materialName](const auto &pair)
		{
			return pair.first == materialName;
		});

		if (textureIter == std::end(MaterialNameTextureSlots))
		{
			DebugLogErrorFormat("Couldn't find matching texture slot index for material name \"%s\" in .OBJ file \"%s\".", materialName.c_str(), objFilename.c_str());
			continue;
		}

		entry.textureSlotIndex = textureIter->second;
		this->entries.emplace_back(std::move(entry));
	}

	return true;
}

std::vector<const MeshLibraryEntry*> MeshLibrary::getEntriesOfType(ArenaVoxelType voxelType) const
{
	// @todo return a Span instead, these should all be presorted

	std::vector<const MeshLibraryEntry*> entryPtrs;
	for (const MeshLibraryEntry &entry : this->entries)
	{
		if (entry.voxelType == voxelType)
		{
			entryPtrs.emplace_back(&entry);
		}
	}
	
	std::sort(entryPtrs.begin(), entryPtrs.end(),
		[](const MeshLibraryEntry *a, const MeshLibraryEntry *b)
	{
		return a->textureSlotIndex < b->textureSlotIndex;
	});

	return entryPtrs;
}
