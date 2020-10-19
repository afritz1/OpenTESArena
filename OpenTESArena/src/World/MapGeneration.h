#ifndef MAP_GENERATION_H
#define MAP_GENERATION_H

#include <optional>

#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"

#include "components/utilities/BufferView.h"

class BinaryAssetLibrary;
class CharacterClassLibrary;
class EntityDefinitionLibrary;
class ExeData;
class LevelDefinition;
class LevelInfoDefinition;
class TextureManager;

enum class WorldType;

namespace MapGeneration
{
	// Converts .MIF voxels into a more modern voxel + entity format.
	void readMifVoxels(const BufferView<const MIFFile::Level> &levels, WorldType worldType,
		bool isPalace, const std::optional<bool> &rulerIsMale, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
	void readMifLocks(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
	void readMifTriggers(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
		BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef);
}

#endif
