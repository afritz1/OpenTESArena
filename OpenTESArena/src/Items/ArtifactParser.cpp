#include <cassert>
#include <map>
#include <sstream>

#include "ArtifactParser.h"

#include "AccessoryArtifactData.h"
#include "AccessoryType.h"
#include "ArmorMaterialType.h"
#include "ArmorType.h"
#include "ArtifactData.h"
#include "BodyArmorArtifactData.h"
#include "ConsumableType.h"
#include "HeavyArmorMaterial.h"
#include "ItemType.h"
#include "LightArmorMaterial.h"
#include "MediumArmorMaterial.h"
#include "MetalType.h"
#include "MiscellaneousArtifactData.h"
#include "MiscellaneousItemType.h"
#include "ShieldArtifactData.h"
#include "ShieldType.h"
#include "TrinketType.h"
#include "WeaponArtifactData.h"
#include "WeaponType.h"
#include "../Entities/BodyPartName.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/String.h"

/*const std::map<std::string, ItemType> ArtifactParserItemTypes =
{
	{ "Accessory", ItemType::Accessory },
	{ "Armor", ItemType::Armor },
	{ "Consumable", ItemType::Consumable },
	{ "Miscellaneous", ItemType::Miscellaneous },
	{ "Trinket", ItemType::Trinket },
	{ "Weapon", ItemType::Weapon }
};

const std::map<std::string, AccessoryType> ArtifactParserAccessoryTypes =
{
	{ "Amulet", AccessoryType::Amulet },
	{ "Belt", AccessoryType::Belt },
	{ "Bracelet", AccessoryType::Bracelet },
	{ "Bracers", AccessoryType::Bracers },
	{ "Ring", AccessoryType::Ring },
	{ "Torc", AccessoryType::Torc }
};

const std::map<std::string, ArmorType> ArtifactParserArmorTypes =
{
	{ "Helm", ArmorType::Helm },
	{ "LeftPauldron", ArmorType::LeftPauldron },
	{ "RightPauldron", ArmorType::RightPauldron },
	{ "Cuirass", ArmorType::Cuirass },
	{ "Gauntlets", ArmorType::Gauntlets },
	{ "Shield", ArmorType::Shield },
	{ "Greaves", ArmorType::Greaves },
	{ "Boots", ArmorType::Boots }
};

const std::map<std::string, BodyPartName> ArtifactParserBodyArmorParts =
{
	{ "Helm", BodyPartName::Head },
	{ "LeftPauldron", BodyPartName::LeftShoulder },
	{ "RightPauldron", BodyPartName::RightShoulder },
	{ "Cuirass", BodyPartName::Chest },
	{ "Gauntlets", BodyPartName::Hands },
	{ "Greaves", BodyPartName::Legs },
	{ "Boots", BodyPartName::Feet }
};

const std::map<std::string, ConsumableType> ArtifactParserConsumableTypes =
{
	{ "Potion", ConsumableType::Potion }
};

const std::map<std::string, MiscellaneousItemType> ArtifactParserMiscellaneousTypes =
{
	{ "Book", MiscellaneousItemType::Book },
	{ "Key", MiscellaneousItemType::Key },
	{ "StaffPiece", MiscellaneousItemType::StaffPiece },
	{ "Torch", MiscellaneousItemType::Torch },
	{ "Unknown", MiscellaneousItemType::Unknown }
};

const std::map<std::string, ShieldType> ArtifactParserShieldTypes =
{
	{ "Buckler", ShieldType::Buckler },
	{ "Round", ShieldType::Round },
	{ "Kite", ShieldType::Kite },
	{ "Tower", ShieldType::Tower }
};

const std::map<std::string, TrinketType> ArtifactParserTrinketTypes =
{
	{ "Crystal", TrinketType::Crystal },
	{ "Mark", TrinketType::Mark }
};

const std::map<std::string, WeaponType> ArtifactParserWeaponTypes =
{
	{ "BattleAxe", WeaponType::BattleAxe },
	{ "Broadsword", WeaponType::Broadsword },
	{ "Claymore", WeaponType::Claymore },
	{ "Dagger", WeaponType::Dagger },
	{ "DaiKatana", WeaponType::DaiKatana },
	{ "Fists", WeaponType::Fists },
	{ "Flail", WeaponType::Flail },
	{ "Katana", WeaponType::Katana },
	{ "LongBow", WeaponType::LongBow },
	{ "Longsword", WeaponType::Longsword },
	{ "Mace", WeaponType::Mace },
	{ "Saber", WeaponType::Saber },
	{ "ShortBow", WeaponType::ShortBow },
	{ "Shortsword", WeaponType::Shortsword },
	{ "Staff", WeaponType::Staff },
	{ "Tanto", WeaponType::Tanto },
	{ "Wakizashi", WeaponType::Wakizashi },
	{ "WarAxe", WeaponType::WarAxe },
	{ "Warhammer", WeaponType::Warhammer }
};

const std::map<std::string, ArmorMaterialType> ArtifactParserMaterialTypes =
{
	{ "Leather", ArmorMaterialType::Leather },
	{ "Chain", ArmorMaterialType::Chain },
	{ "Iron", ArmorMaterialType::Plate },
	{ "Steel", ArmorMaterialType::Plate },
	{ "Silver", ArmorMaterialType::Plate },
	{ "Elven", ArmorMaterialType::Plate },
	{ "Dwarven", ArmorMaterialType::Plate },
	{ "Mithril", ArmorMaterialType::Plate },
	{ "Adamantium", ArmorMaterialType::Plate },
	{ "Ebony", ArmorMaterialType::Plate }
};

const std::map<std::string, MetalType> ArtifactParserMetalTypes =
{
	{ "Iron", MetalType::Iron },
	{ "Steel", MetalType::Steel },
	{ "Silver", MetalType::Silver },
	{ "Elven", MetalType::Elven },
	{ "Dwarven", MetalType::Dwarven },
	{ "Mithril", MetalType::Mithril },
	{ "Adamantium", MetalType::Adamantium },
	{ "Ebony", MetalType::Ebony }
};

// These paths might be obsolete soon.
const std::string ArtifactParser::PATH = "data/text/";
const std::string ArtifactParser::FILENAME = "artifacts.txt";

std::unique_ptr<ArtifactData> ArtifactParser::makeAccessory(const std::string &displayName,
	const std::string &description, const std::vector<int> &provinceIDs,
	const std::string &accessoryTypeToken, const std::string &metalToken)
{
	const auto accessoryType = ArtifactParserAccessoryTypes.at(accessoryTypeToken);
	const auto metalType = ArtifactParserMetalTypes.at(metalToken);
	return std::unique_ptr<ArtifactData>(new AccessoryArtifactData(
		displayName, description, provinceIDs, accessoryType, metalType));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeBodyArmor(const std::string &displayName,
	const std::string &description, const std::vector<int> &provinceIDs,
	const std::string &partNameToken, const std::string &materialToken)
{
	const auto partName = ArtifactParserBodyArmorParts.at(partNameToken);
	const auto materialType = ArtifactParserMaterialTypes.at(materialToken);

	std::unique_ptr<ArmorMaterial> armorMaterial;

	if (materialType == ArmorMaterialType::Leather)
	{
		armorMaterial = std::unique_ptr<ArmorMaterial>(new LightArmorMaterial());
	}
	else if (materialType == ArmorMaterialType::Chain)
	{
		armorMaterial = std::unique_ptr<ArmorMaterial>(new MediumArmorMaterial());
	}
	else if (materialType == ArmorMaterialType::Plate)
	{
		const auto metalType = ArtifactParserMetalTypes.at(materialToken);
		armorMaterial = std::unique_ptr<ArmorMaterial>(new HeavyArmorMaterial(metalType));
	}

	// Make sure all "if" branches above work as intended.
	assert(armorMaterial.get() != nullptr);

	return std::unique_ptr<ArtifactData>(new BodyArmorArtifactData(
		displayName, description, provinceIDs, armorMaterial.get(), partName));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeMiscellaneous(const std::string &displayName,
	const std::string &description, const std::vector<int> &provinceIDs,
	const std::string &miscTypeToken)
{
	const auto miscType = ArtifactParserMiscellaneousTypes.at(miscTypeToken);
	return std::unique_ptr<ArtifactData>(new MiscellaneousArtifactData(displayName,
		description, provinceIDs, miscType));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeShield(const std::string &displayName,
	const std::string &description, const std::vector<int> &provinceIDs,
	const std::string &shieldTypeToken, const std::string &metalToken)
{
	const auto shieldType = ArtifactParserShieldTypes.at(shieldTypeToken);
	const auto metalType = ArtifactParserMetalTypes.at(metalToken);
	return std::unique_ptr<ArtifactData>(new ShieldArtifactData(displayName,
		description, provinceIDs, shieldType, metalType));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeWeapon(const std::string &displayName,
	const std::string &description, const std::vector<int> &provinceIDs,
	const std::string &weaponTypeToken, const std::string &metalToken)
{
	const auto weaponType = ArtifactParserWeaponTypes.at(weaponTypeToken);
	const auto metalType = ArtifactParserMetalTypes.at(metalToken);
	return std::unique_ptr<ArtifactData>(new WeaponArtifactData(displayName,
		description, provinceIDs, weaponType, metalType));
}

std::vector<std::unique_ptr<ArtifactData>> ArtifactParser::parse()
{
	// This parser is very simple right now. All text must have the exact amount
	// of spacing and commas, and there must be a new line at the end of the file.
	// Comment lines must have the comment symbol in the first column.

	std::string fullPath(ArtifactParser::PATH + ArtifactParser::FILENAME);

	// Read the artifacts file into a string.
	std::string text = File::toString(fullPath);

	// Relevant parsing symbols.
	const char comment = '#';
	const char comma = ',';

	std::vector<std::unique_ptr<ArtifactData>> artifacts;
	std::istringstream iss(text);
	std::string line;

	// For each line, get the substrings between commas.
	while (std::getline(iss, line))
	{
		const char &firstColumn = line.at(0);

		// Ignore comments and blank lines.
		if ((firstColumn == comment) ||
			(firstColumn == '\r') ||
			(firstColumn == '\n'))
		{
			continue;
		}

		// Get the display name.
		int index = 0;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string displayName = line.substr(0, index);

		// Get the item type, any derived types, and any material/metal.
		index += 2;
		int oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string typesString = line.substr(oldIndex, index - oldIndex);
		std::vector<std::string> typeTokens = String::split(typesString);

		// Get the provinces.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string provincesString = line.substr(oldIndex, index - oldIndex);
		std::vector<std::string> provinceTokens = String::split(provincesString);

		// Get the description (ignore quotes, read to the end of the line).
		index += 3;
		oldIndex = index;
		while ((line.at(index) != '\r') && (line.at(index) != '\n'))
		{
			++index;
		}

		std::string description = line.substr(oldIndex, index - oldIndex - 1);

		// Verify that the strings each have a mapping.
		const auto &itemTypeToken = typeTokens.at(0);
		Debug::check(ArtifactParserItemTypes.find(itemTypeToken) != ArtifactParserItemTypes.end(),
			"Artifact Parser", "Invalid item type \"" + itemTypeToken + "\" for \"" +
			displayName + "\".");

		// Create the artifact data based on the item type.
		const auto itemType = ArtifactParserItemTypes.at(itemTypeToken);
		auto provinces = ArtifactParser::parseProvinces(provinceTokens);
		std::vector<std::string> derivedTokens(typeTokens.begin() + 1, typeTokens.end());
		std::unique_ptr<ArtifactData> artifactData;
		switch (itemType)
		{
		case ItemType::Accessory:
			// Make accessory artifact data.
			Debug::check(derivedTokens.size() == 2, "Artifact Parser",
				"Invalid accessory token count for \"" + displayName + "\".");
			artifactData = ArtifactParser::makeAccessory(displayName, description, provinces,
				derivedTokens.at(0), derivedTokens.at(1));
			break;
		case ItemType::Armor:
			// Decide what the armor type is.
			if (ArtifactParserArmorTypes.at(derivedTokens.at(0)) == ArmorType::Shield)
			{
				// Make shield artifact data.
				Debug::check(derivedTokens.size() == 3, "Artifact Parser",
					"Invalid shield token count for \"" + displayName + "\".");
				artifactData = ArtifactParser::makeShield(displayName, description, provinces,
					derivedTokens.at(1), derivedTokens.at(2));
			}
			else
			{
				// Make body armor artifact data.
				Debug::check(derivedTokens.size() == 2, "Artifact Parser",
					"Invalid body armor token count for \"" + displayName + "\".");
				artifactData = ArtifactParser::makeBodyArmor(displayName, description, provinces,
					derivedTokens.at(0), derivedTokens.at(1));
			}
			break;
		case ItemType::Miscellaneous:
			// Make miscellaneous artifact data.
			Debug::check(derivedTokens.size() == 1, "Artifact Parser",
				"Invalid miscellaneous token count for \"" + displayName + "\".");
			artifactData = ArtifactParser::makeMiscellaneous(displayName, description, provinces,
				derivedTokens.at(0));
			break;
		case ItemType::Weapon:
			// Make weapon artifact data.
			Debug::check(derivedTokens.size() == 2, "Artifact Parser",
				"Invalid weapon token count for \"" + displayName + "\".");
			artifactData = ArtifactParser::makeWeapon(displayName, description, provinces,
				derivedTokens.at(0), derivedTokens.at(1));
			break;
		default:
			Debug::crash("Artifact Parser", "Item type \"" + itemTypeToken + "\" not implemented.");
			break;
		}

		// Make sure the artifact data was set in one of the case statements.
		assert(artifactData.get() != nullptr);

		artifacts.push_back(std::move(artifactData));
	}

	return artifacts;
}*/
