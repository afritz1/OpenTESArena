#include <algorithm>

#include "ArenaLevelUtils.h"
#include "../Assets/ExeData.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/RMDFile.h"
#include "../Assets/TextureManager.h"
#include "../Math/Random.h"
#include "../Rendering/Renderer.h"
#include "../Voxels/ArenaVoxelUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"

uint8_t ArenaLevelUtils::getVoxelMostSigByte(ArenaVoxelID voxelID)
{
	return (voxelID & 0x7F00) >> 8;
}

uint8_t ArenaLevelUtils::getVoxelLeastSigByte(ArenaVoxelID voxelID)
{
	return voxelID & 0x007F;
}

double ArenaLevelUtils::convertCeilingHeightToScale(int ceilingHeight)
{
	return static_cast<double>(ceilingHeight) / MIFUtils::ARENA_UNITS;
}

int ArenaLevelUtils::getMap2VoxelHeight(ArenaVoxelID map2Voxel)
{
	if ((map2Voxel & 0x80) == 0x80)
	{
		return 2;
	}
	else if ((map2Voxel & 0x8000) == 0x8000)
	{
		return 3;
	}
	else if ((map2Voxel & 0x8080) == 0x8080)
	{
		return 4;
	}
	else
	{
		return 1;
	}
}

int ArenaLevelUtils::getMap2Height(Span2D<const ArenaVoxelID> map2)
{
	DebugAssert(map2.isValid());

	int currentMap2Height = 1;
	for (SNInt z = 0; z < map2.getHeight(); z++)
	{
		for (WEInt x = 0; x < map2.getWidth(); x++)
		{
			const ArenaVoxelID map2Voxel = map2.get(x, z);
			const int map2Height = ArenaLevelUtils::getMap2VoxelHeight(map2Voxel);
			currentMap2Height = std::max(currentMap2Height, map2Height);
		}
	}

	return currentMap2Height;
}

int ArenaLevelUtils::getMifLevelHeight(const MIFLevel &level, const INFCeiling *ceiling)
{
	const Span2D<const ArenaVoxelID> map2 = level.getMAP2();

	if (map2.isValid())
	{
		return 2 + ArenaLevelUtils::getMap2Height(map2);
	}
	else
	{
		const bool hasCeiling = (ceiling != nullptr) && !ceiling->outdoorDungeon;
		return hasCeiling ? 3 : 2;
	}
}

uint16_t ArenaLevelUtils::getDoorVoxelOffset(WEInt x, SNInt y)
{
	return (y << 8) + (x << 1);
}

std::string ArenaLevelUtils::getDoorVoxelMifName(WEInt x, SNInt y, int menuID, uint32_t rulerSeed,
	bool palaceIsMainQuestDungeon, ArenaCityType cityType, MapType mapType, const ExeData &exeData)
{
	// Get the menu type associated with the *MENU ID.
	const ArenaMenuType menuType = ArenaVoxelUtils::getMenuType(menuID, mapType);

	// Check special case first: if it's a palace block in the center province's city,
	// the .MIF name is hardcoded.
	const bool isFinalDungeonEntrance = palaceIsMainQuestDungeon &&
		(menuType == ArenaMenuType::Palace);

	if (isFinalDungeonEntrance)
	{
		return String::toUppercase(exeData.locations.finalDungeonMifName);
	}

	// Get the prefix associated with the menu type.
	const std::string menuName = [&exeData, cityType, menuType]()
	{
		const std::string name = [&exeData, cityType, menuType]() -> std::string
		{
			// Mappings of menu types to menu .MIF prefix indices. Menus that have no .MIF
			// filename mapping are considered special cases. TOWNPAL and VILPAL are not used
			// since the palace type can be deduced from the current city type.
			constexpr int NO_INDEX = -1;
			constexpr std::array<std::pair<ArenaMenuType, int>, 12> MenuMifMappings =
			{
				{
					{ ArenaMenuType::CityGates, NO_INDEX },
					{ ArenaMenuType::Crypt, 7 },
					{ ArenaMenuType::Dungeon, NO_INDEX },
					{ ArenaMenuType::Equipment, 5 },
					{ ArenaMenuType::House, 1 },
					{ ArenaMenuType::MagesGuild, 6 },
					{ ArenaMenuType::Noble, 2 },
					{ ArenaMenuType::None, NO_INDEX },
					{ ArenaMenuType::Palace, 0 },
					{ ArenaMenuType::Tavern, 3 },
					{ ArenaMenuType::Temple, 4 },
					{ ArenaMenuType::Tower, 10 }
				}
			};

			// See if the given menu type has a .MIF prefix mapping.
			const auto iter = std::find_if(MenuMifMappings.begin(), MenuMifMappings.end(),
				[menuType](const std::pair<ArenaMenuType, int> &pair)
			{
				return pair.first == menuType;
			});

			if (iter != MenuMifMappings.end())
			{
				const int index = iter->second;

				if (index != NO_INDEX)
				{
					// Get the menu's .MIF prefix index. If it's a palace, then decide which palace
					// prefix to use based on the location type.
					const int menuMifIndex = [cityType, menuType, index]()
					{
						if (menuType == ArenaMenuType::Palace)
						{
							if (cityType == ArenaCityType::CityState)
							{
								return 0;
							}
							else if (cityType == ArenaCityType::Town)
							{
								return 8;
							}
							else if (cityType == ArenaCityType::Village)
							{
								return 9;
							}
							else
							{
								DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(cityType)));
							}
						}
						else
						{
							return index;
						}
					}();

					const auto &prefixes = exeData.locations.menuMifPrefixes;
					DebugAssertIndex(prefixes, menuMifIndex);
					return prefixes[menuMifIndex];
				}
				else
				{
					// The menu has no valid .MIF prefix.
					return std::string();
				}
			}
			else
			{
				DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(menuType)));
			}
		}();

		return String::toUppercase(name);
	}();

	// Some menu names don't map to an actual building type, so they are special cases
	// and should be ignored by the caller.
	const bool isSpecialCase = menuName.size() == 0;

	if (isSpecialCase)
	{
		// No .MIF filename. The caller knows not to try and load a .MIF file.
		return std::string();
	}
	else
	{
		// Offset is based on X and Y position in world; used with variant calculation.
		const uint16_t offset = ArenaLevelUtils::getDoorVoxelOffset(x, y);

		// Decide which variant of the interior to use.
		const int variantID = [rulerSeed, offset, menuType]()
		{
			// Palaces have fewer .MIF files to choose from, and their variant depends
			// on the ruler seed. Although there are five city-state palace .MIF files,
			// only three of them are used.
			const bool isPalace = menuType == ArenaMenuType::Palace;
			const int palaceCount = 3;
			return isPalace ? (((rulerSeed >> 8) & 0xFFFF) % palaceCount) :
				((Bytes::ror(offset, 4) ^ offset) % 8);
		}();

		// Generate .MIF filename.
		return menuName + std::to_string(variantID + 1) + ".MIF";
	}
}

int ArenaLevelUtils::getDoorVoxelLockLevel(WEInt x, SNInt y, ArenaRandom &random)
{
	const uint16_t offset = ArenaLevelUtils::getDoorVoxelOffset(x, y);
	const uint32_t seed = offset + (offset << 16);
	random.srand(seed);
	return (random.next() % 10) + 1; // 0..9 + 1.
}

int ArenaLevelUtils::getServiceSaveFileNumber(WEInt doorX, SNInt doorY)
{
	return (doorY << 8) + doorX;
}

int ArenaLevelUtils::getWildernessServiceSaveFileNumber(int wildX, int wildY)
{
	return (wildY << 16) + wildX;
}

ObjectTextureID ArenaLevelUtils::allocGameWorldPaletteTexture(const std::string &filename, TextureManager &textureManager, Renderer &renderer)
{
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(filename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID from \"" + filename + "\".");
		return -1;
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	const ObjectTextureID paletteTextureID = renderer.createObjectTexture(static_cast<int>(palette.size()), 1, 4);
	if (paletteTextureID < 0)
	{
		DebugLogError("Couldn't create palette texture \"" + filename + "\".");
		return -1;
	}

	LockedTexture lockedTexture = renderer.lockObjectTexture(paletteTextureID);
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock palette texture \"" + filename + "\" for writing.");
		return -1;
	}

	DebugAssert(lockedTexture.bytesPerTexel == 4);
	uint32_t *paletteTexels = reinterpret_cast<uint32_t*>(lockedTexture.texels.begin());
	std::transform(palette.begin(), palette.end(), paletteTexels,
		[](const Color &paletteColor)
	{
		return paletteColor.toARGB();
	});

	renderer.unlockObjectTexture(paletteTextureID);
	return paletteTextureID;
}

ObjectTextureID ArenaLevelUtils::allocLightTableTexture(const std::string &filename, TextureManager &textureManager, Renderer &renderer)
{
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(filename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogErrorFormat("Couldn't get light table texture builder ID from \"%s\".", filename.c_str());
		return -1;
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const ObjectTextureID textureID = renderer.createObjectTexture(textureBuilder.width, textureBuilder.height, textureBuilder.bytesPerTexel);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't create light table texture \"%s\".", filename.c_str());
		return -1;
	}

	if (!renderer.populateObjectTexture(textureID, textureBuilder.texels))
	{
		DebugLogErrorFormat("Couldn't populate light table texture \"%s\".", filename.c_str());
	}

	return textureID;
}
