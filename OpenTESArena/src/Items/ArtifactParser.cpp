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
#include "../World/ProvinceName.h"

const auto ArtifactParserItemTypes = std::map<std::string, ItemType>
{
	{ "Accessory", ItemType::Accessory },
	{ "Armor", ItemType::Armor },
	{ "Consumable", ItemType::Consumable },
	{ "Miscellaneous", ItemType::Miscellaneous },
	{ "Trinket", ItemType::Trinket },
	{ "Weapon", ItemType::Weapon }
};

const auto ArtifactParserAccessoryTypes = std::map<std::string, AccessoryType>
{
	{ "Amulet", AccessoryType::Amulet },
	{ "Belt", AccessoryType::Belt },
	{ "Bracelet", AccessoryType::Bracelet },
	{ "Bracers", AccessoryType::Bracers },
	{ "Ring", AccessoryType::Ring },
	{ "Torc", AccessoryType::Torc }
};

const auto ArtifactParserArmorTypes = std::map<std::string, ArmorType>
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

const auto ArtifactParserBodyArmorParts = std::map<std::string, BodyPartName>
{
	{ "Helm", BodyPartName::Head },
	{ "LeftPauldron", BodyPartName::LeftShoulder },
	{ "RightPauldron", BodyPartName::RightShoulder },
	{ "Cuirass", BodyPartName::Chest },
	{ "Gauntlets", BodyPartName::Hands },
	{ "Greaves", BodyPartName::Legs },
	{ "Boots", BodyPartName::Feet }
};

const auto ArtifactParserConsumableTypes = std::map<std::string, ConsumableType>
{
	{ "Food", ConsumableType::Food },
	{ "Potion", ConsumableType::Potion }
};

const auto ArtifactParserMiscellaneousTypes = std::map<std::string, MiscellaneousItemType>
{
	{ "Book", MiscellaneousItemType::Book },
	{ "Key", MiscellaneousItemType::Key },
	{ "StaffPiece", MiscellaneousItemType::StaffPiece },
	{ "Torch", MiscellaneousItemType::Torch },
	{ "Unknown", MiscellaneousItemType::Unknown }
};

const auto ArtifactParserShieldTypes = std::map<std::string, ShieldType>
{
	{ "Buckler", ShieldType::Buckler },
	{ "Round", ShieldType::Round },
	{ "Kite", ShieldType::Kite },
	{ "Tower", ShieldType::Tower }
};

const auto ArtifactParserTrinketTypes = std::map<std::string, TrinketType>
{
	{ "Crystal", TrinketType::Crystal },
	{ "Mark", TrinketType::Mark }
};

const auto ArtifactParserWeaponTypes = std::map<std::string, WeaponType>
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

const auto ArtifactParserMaterialTypes = std::map<std::string, ArmorMaterialType>
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

const auto ArtifactParserMetalTypes = std::map<std::string, MetalType>
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

const auto ArtifactParserProvinces = std::map<std::string, ProvinceName>
{
	{ "BlackMarsh", ProvinceName::BlackMarsh },
	{ "Elsweyr", ProvinceName::Elsweyr },
	{ "Hammerfell", ProvinceName::Hammerfell },
	{ "HighRock", ProvinceName::HighRock },
	{ "ImperialProvince", ProvinceName::ImperialProvince },
	{ "Morrowind", ProvinceName::Morrowind },
	{ "Skyrim", ProvinceName::Skyrim },
	{ "SummersetIsle", ProvinceName::SummersetIsle },
	{ "Valenwood", ProvinceName::Valenwood }
};

const std::string ArtifactParser::PATH = "data/text/";
const std::string ArtifactParser::FILENAME = "artifacts.txt";

std::vector<ProvinceName> ArtifactParser::parseProvinces(
	const std::vector<std::string> &provinceTokens)
{
	auto provinces = std::vector<ProvinceName>();

	for (const auto &provinceToken : provinceTokens)
	{
		provinces.push_back(ArtifactParserProvinces.at(provinceToken));
	}

	return provinces;
}

std::unique_ptr<ArtifactData> ArtifactParser::makeAccessory(const std::string &displayName,
	const std::string &description, const std::vector<ProvinceName> &provinces,
	const std::string &accessoryTypeToken, const std::string &metalToken)
{
	auto accessoryType = ArtifactParserAccessoryTypes.at(accessoryTypeToken);
	auto metalType = ArtifactParserMetalTypes.at(metalToken);
	return std::unique_ptr<ArtifactData>(new AccessoryArtifactData(
		displayName, description, provinces, accessoryType, metalType));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeBodyArmor(const std::string &displayName,
	const std::string &description, const std::vector<ProvinceName> &provinces,
	const std::string &partNameToken, const std::string &materialToken)
{
	auto partName = ArtifactParserBodyArmorParts.at(partNameToken);
	auto materialType = ArtifactParserMaterialTypes.at(materialToken);
	auto armorMaterial = std::unique_ptr<ArmorMaterial>(nullptr);

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
		auto metalType = ArtifactParserMetalTypes.at(materialToken);
		armorMaterial = std::unique_ptr<ArmorMaterial>(new HeavyArmorMaterial(metalType));
	}

	assert(armorMaterial.get() != nullptr);
	return std::unique_ptr<ArtifactData>(new BodyArmorArtifactData(
		displayName, description, provinces, armorMaterial.get(), partName));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeMiscellaneous(const std::string &displayName,
	const std::string &description, const std::vector<ProvinceName> &provinces,
	const std::string &miscTypeToken)
{
	auto miscType = ArtifactParserMiscellaneousTypes.at(miscTypeToken);
	return std::unique_ptr<ArtifactData>(new MiscellaneousArtifactData(displayName,
		description, provinces, miscType));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeShield(const std::string &displayName,
	const std::string &description, const std::vector<ProvinceName> &provinces,
	const std::string &shieldTypeToken, const std::string &metalToken)
{
	auto shieldType = ArtifactParserShieldTypes.at(shieldTypeToken);
	auto metalType = ArtifactParserMetalTypes.at(metalToken);
	return std::unique_ptr<ArtifactData>(new ShieldArtifactData(displayName,
		description, provinces, shieldType, metalType));
}

std::unique_ptr<ArtifactData> ArtifactParser::makeWeapon(const std::string &displayName,
	const std::string &description, const std::vector<ProvinceName> &provinces,
	const std::string &weaponTypeToken, const std::string &metalToken)
{
	auto weaponType = ArtifactParserWeaponTypes.at(weaponTypeToken);
	auto metalType = ArtifactParserMetalTypes.at(metalToken);
	return std::unique_ptr<ArtifactData>(new WeaponArtifactData(displayName,
		description, provinces, weaponType, metalType));
}

std::vector<std::unique_ptr<ArtifactData>> ArtifactParser::parse()
{
	// This parser is very simple right now. All text must have the exact amount
	// of spacing and commas, and there must be a new line at the end of the file.
	// Comment lines must have the comment symbol in the first column.

	auto fullPath = ArtifactParser::PATH + ArtifactParser::FILENAME;

	// Read the artifacts file into a string.
	auto text = File::toString(fullPath);

	// Relevant parsing symbols.
	const char comment = '#';
	const char comma = ',';

	auto artifacts = std::vector<std::unique_ptr<ArtifactData>>();
	std::istringstream iss;
	iss.str(text);

	auto line = std::string();

	// For each line, get the substrings between commas.
	while (std::getline(iss, line))
	{
		// Ignore comments and blank lines.
		if ((line.at(0) == comment) || (line.at(0) == '\r') || (line.at(0) == '\n'))
		{
			continue;
		}

		// Get the display name.
		int index = 0;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto displayName = line.substr(0, index);

		// Get the item type, any derived types, and any material/metal.
		index += 2;
		int oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto typesString = line.substr(oldIndex, index - oldIndex);
		auto typeTokens = String::split(typesString);

		// Get the provinces.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto provincesString = line.substr(oldIndex, index - oldIndex);
		auto provinceTokens = String::split(provincesString);

		// Get the description (ignore quotes, read to the end of the line).
		index += 3;
		oldIndex = index;
		while ((line.at(index) != '\r') && (line.at(index) != '\n'))
		{
			++index;
		}

		auto description = line.substr(oldIndex, index - oldIndex - 1);

		// Verify that the strings each have a mapping.
		const auto &itemTypeToken = typeTokens.at(0);
		Debug::check(ArtifactParserItemTypes.find(itemTypeToken) != ArtifactParserItemTypes.end(),
			"Artifact Parser", "Invalid item type \"" + itemTypeToken + "\" for \"" +
			displayName + "\".");

		for (const auto &province : provinceTokens)
		{
			Debug::check(ArtifactParserProvinces.find(province) != ArtifactParserProvinces.end(),
				"Artifact Parser", "Invalid province \"" + province + "\" for \"" +
				displayName + "\".");
		}

		// Create the artifact data based on the item type.
		auto itemType = ArtifactParserItemTypes.at(itemTypeToken);
		auto derivedTokens = std::vector<std::string>(typeTokens.begin() + 1, typeTokens.end());
		auto provinces = ArtifactParser::parseProvinces(provinceTokens);
		auto artifactData = std::unique_ptr<ArtifactData>(nullptr);
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

		assert(artifactData.get() != nullptr);
		artifacts.push_back(std::move(artifactData));
	}

	return artifacts;
}
