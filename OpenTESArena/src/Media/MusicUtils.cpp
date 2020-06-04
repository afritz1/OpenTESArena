#include <array>

#include "MusicUtils.h"

#include "components/debug/Debug.h"

bool MusicUtils::tryGetInteriorMusicType(const std::string_view &mifName,
	MusicDefinition::InteriorMusicDefinition::Type *outType)
{
	// Check against all of the non-dungeon interiors first.
	const bool isEquipmentStore = mifName.find("EQUIP") != std::string::npos;
	const bool isHouse = (mifName.find("BS") != std::string::npos) ||
		(mifName.find("NOBLE") != std::string::npos);
	const bool isMagesGuild = mifName.find("MAGE") != std::string::npos;
	const bool isPalace = (mifName.find("PALACE") != std::string::npos) ||
		(mifName.find("TOWNPAL") != std::string::npos) ||
		(mifName.find("VILPAL") != std::string::npos);
	const bool isTavern = mifName.find("TAVERN") != std::string::npos;
	const bool isTemple = mifName.find("TEMPLE") != std::string::npos;

	if (isEquipmentStore)
	{
		*outType = MusicDefinition::InteriorMusicDefinition::Type::Equipment;
	}
	else if (isHouse)
	{
		*outType = MusicDefinition::InteriorMusicDefinition::Type::House;
	}
	else if (isMagesGuild)
	{
		*outType = MusicDefinition::InteriorMusicDefinition::Type::MagesGuild;
	}
	else if (isPalace)
	{
		*outType = MusicDefinition::InteriorMusicDefinition::Type::Palace;
	}
	else if (isTavern)
	{
		*outType = MusicDefinition::InteriorMusicDefinition::Type::Tavern;
	}
	else if (isTemple)
	{
		*outType = MusicDefinition::InteriorMusicDefinition::Type::Temple;
	}
	else
	{
		// Not a special interior -- it's a dungeon.
		return false;
	}

	return true;
}
