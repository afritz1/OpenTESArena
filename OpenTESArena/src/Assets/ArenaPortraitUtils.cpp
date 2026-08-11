#include "ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAsset.h"
#include "../Items/ArenaItemUtils.h"

#include "components/utilities/String.h"

std::string ArenaPortraitUtils::getHeads(bool male, int raceID, bool trimmed)
{
	return String::format("FACES%s%d%d.CIF", male ? "" : "F", trimmed ? 0 : 1, raceID);
}

std::string ArenaPortraitUtils::getBody(bool male, int raceID)
{
	return String::format("%s0%d.IMG", male ? "CHARBK" : "CHRBKF", raceID);
}

const std::string &ArenaPortraitUtils::getShirt(bool male, bool magic)
{
	if (male)
	{
		return magic ? ArenaTextureName::MaleMagicShirt : ArenaTextureName::MaleNonMagicShirt;
	}
	else
	{
		return magic ? ArenaTextureName::FemaleMagicShirt : ArenaTextureName::FemaleNonMagicShirt;
	}
}

const std::string &ArenaPortraitUtils::getPants(bool male)
{
	return male ? ArenaTextureName::MalePants : ArenaTextureName::FemalePants;
}

const std::string &ArenaPortraitUtils::getEquipment(bool male)
{
	return male ? ArenaTextureName::MaleEquipment : ArenaTextureName::FemaleEquipment;
}

TextureAsset ArenaPortraitUtils::getWeapon(ArenaWeaponTypeID weaponID, bool male)
{
	const std::string filename = ArenaPortraitUtils::getEquipment(male);
	return TextureAsset(filename, weaponID);
}

TextureAsset ArenaPortraitUtils::getArmor(ArenaArmorTypeID armorID, ArenaArmorMaterialType armorMaterialType, bool male)
{
	const std::string filename = ArenaPortraitUtils::getEquipment(male);
	constexpr int baseArmorIndex = ArenaItemUtils::MeleeWeaponCount + ArenaItemUtils::RangedWeaponCount;
	
	int armorMaterialOffset = 0;

	const bool isShield = (armorID >= 7) && (armorID <= 11);
	if (!isShield)
	{
		switch (armorMaterialType)
		{
		case ArenaArmorMaterialType::None:
		case ArenaArmorMaterialType::Plate:
			armorMaterialOffset = 0;
			break;
		case ArenaArmorMaterialType::Chain:
			armorMaterialOffset = 11;
			break;
		case ArenaArmorMaterialType::Leather:
			armorMaterialOffset = 18;
			break;
		default:
			break;
		}
	}

	return TextureAsset(filename, baseArmorIndex + armorMaterialOffset + armorID);
}

Int2 ArenaPortraitUtils::getShirtOffset(bool male, bool magic)
{
	if (male)
	{
		return magic ? Int2(215, 35) : Int2(186, 12);
	}
	else
	{
		return magic ? Int2(220, 33) : Int2(220, 35);
	}
}

Int2 ArenaPortraitUtils::getPantsOffset(bool male)
{
	return male ? Int2(229, 82) : Int2(212, 74);
}
