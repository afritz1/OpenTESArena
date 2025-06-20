#include <algorithm>
#include <string>

#include "MeshLibrary.h"
#include "../Assets/ArenaTypes.h"
#include "../Voxels/VoxelFacing.h"
#include "../Voxels/VoxelUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"
#include "components/utilities/String.h"

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

	// The voxel face that a mesh intends to cover.
	constexpr std::pair<const char*, VoxelFacing3D> FacingMappings[] =
	{
		{ "North", VoxelFacing3D::NegativeX },
		{ "East", VoxelFacing3D::NegativeZ },
		{ "South", VoxelFacing3D::PositiveX },
		{ "West", VoxelFacing3D::PositiveZ },
		{ "Bottom", VoxelFacing3D::NegativeY },
		{ "Top", VoxelFacing3D::PositiveY }
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
			[materialName](const std::pair<const char*, int> &pair)
		{
			return pair.first == materialName;
		});

		if (textureIter == std::end(MaterialNameTextureSlots))
		{
			DebugLogErrorFormat("Couldn't find matching texture slot index for material name \"%s\" in .OBJ file \"%s\".", materialName.c_str(), objFilename.c_str());
			continue;
		}

		entry.textureSlotIndex = textureIter->second;

		size_t lastDirectorySeparatorIndex = objFilename.find_last_of("/\\");
		if (lastDirectorySeparatorIndex != std::string::npos)
		{
			lastDirectorySeparatorIndex++;
		}
		else
		{
			lastDirectorySeparatorIndex = 0;
		}

		std::string baseFilename = objFilename.substr(lastDirectorySeparatorIndex, objFilename.size() - lastDirectorySeparatorIndex);
		std::optional<VoxelFacing3D> facing;
		for (const std::pair<const char*, VoxelFacing3D> &facingMapping : FacingMappings)
		{
			const char *orientationName = facingMapping.first;
			if (baseFilename.find(orientationName) != std::string::npos)
			{
				facing = facingMapping.second;
				break;
			}
		}

		entry.facing = facing;
		this->entries.emplace_back(std::move(entry));
	}

	std::sort(this->entries.begin(), this->entries.end(),
		[](const MeshLibraryEntry &a, const MeshLibraryEntry &b)
	{
		if (a.voxelType != b.voxelType)
		{
			return static_cast<int>(a.voxelType) < static_cast<int>(b.voxelType);
		}

		const int aFacingIndex = VoxelUtils::getFacingIndex(a.facing.value_or(VoxelFacing3D::PositiveX));
		const int bFacingIndex = VoxelUtils::getFacingIndex(b.facing.value_or(VoxelFacing3D::PositiveX));
		return aFacingIndex < bFacingIndex;
	});

	return true;
}

Span<const MeshLibraryEntry> MeshLibrary::getEntriesOfType(ArenaVoxelType voxelType) const
{
	auto entryComparer = [voxelType](const MeshLibraryEntry &entry) { return entry.voxelType == voxelType; };
	const auto beginIter = std::find_if(this->entries.begin(), this->entries.end(), entryComparer);
	if (beginIter == this->entries.end())
	{
		return Span<const MeshLibraryEntry>();
	}

	const auto endIter = std::find_if_not(beginIter, this->entries.end(), entryComparer);
	int startIndex = static_cast<int>(std::distance(this->entries.begin(), beginIter));
	const int count = static_cast<int>(std::distance(beginIter, endIter));
	return Span<const MeshLibraryEntry>(this->entries.data() + startIndex, count);
}

const MeshLibraryEntry *MeshLibrary::getEntryWithTypeAndFacing(ArenaVoxelType voxelType, VoxelFacing3D facing) const
{
	Span<const MeshLibraryEntry> entries = this->getEntriesOfType(voxelType);
	for (const MeshLibraryEntry &entry : entries)
	{
		if (entry.facing == facing)
		{
			return &entry;
		}
	}

	return nullptr;
}
