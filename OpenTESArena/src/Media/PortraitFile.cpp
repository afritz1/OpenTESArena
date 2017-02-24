#include <cassert>
#include <unordered_map>

#include "PortraitFile.h"

#include "TextureFile.h"
#include "TextureName.h"
#include "../Entities/CharacterRaceName.h"
#include "../Entities/GenderName.h"

namespace std
{
	// Hash specializations, since GCC doesn't support enum classes used as keys
	// in unordered_maps.
	template <>
	struct hash<GenderName>
	{
		size_t operator()(const GenderName &x) const
		{
			return static_cast<size_t>(x);
		}
	};

	template <>
	struct hash<CharacterRaceName>
	{
		size_t operator()(const CharacterRaceName &x) const
		{
			return static_cast<size_t>(x);
		}
	};

	template <>
	struct hash<std::pair<GenderName, CharacterRaceName>>
	{
		size_t operator()(const std::pair<GenderName, CharacterRaceName> &x) const
		{
			return std::hash<GenderName>()(x.first) ^
				std::hash<CharacterRaceName>()(x.second);
		}
	};

	template <>
	struct hash<std::pair<GenderName, bool>>
	{
		size_t operator()(const std::pair<GenderName, bool> &x) const
		{
			return std::hash<GenderName>()(x.first) ^ std::hash<bool>()(x.second);
		}
	};
}

namespace
{
	// Pairings of genders and races to portrait head filenames designed for the
	// character sheet.
	const std::unordered_map<std::pair<GenderName, CharacterRaceName>,
		TextureName> HeadTextureNames =
	{
		{ { GenderName::Female, CharacterRaceName::Argonian }, TextureName::FemaleArgonianHeads },
		{ { GenderName::Female, CharacterRaceName::Breton }, TextureName::FemaleBretonHeads },
		{ { GenderName::Female, CharacterRaceName::DarkElf }, TextureName::FemaleDarkElfHeads },
		{ { GenderName::Female, CharacterRaceName::HighElf }, TextureName::FemaleHighElfHeads },
		{ { GenderName::Female, CharacterRaceName::Khajiit }, TextureName::FemaleKhajiitHeads },
		{ { GenderName::Female, CharacterRaceName::Nord }, TextureName::FemaleNordHeads },
		{ { GenderName::Female, CharacterRaceName::Redguard }, TextureName::FemaleRedguardHeads },
		{ { GenderName::Female, CharacterRaceName::WoodElf }, TextureName::FemaleWoodElfHeads },

		{ { GenderName::Male, CharacterRaceName::Argonian }, TextureName::MaleArgonianHeads },
		{ { GenderName::Male, CharacterRaceName::Breton }, TextureName::MaleBretonHeads },
		{ { GenderName::Male, CharacterRaceName::DarkElf }, TextureName::MaleDarkElfHeads },
		{ { GenderName::Male, CharacterRaceName::HighElf }, TextureName::MaleHighElfHeads },
		{ { GenderName::Male, CharacterRaceName::Khajiit }, TextureName::MaleKhajiitHeads },
		{ { GenderName::Male, CharacterRaceName::Nord }, TextureName::MaleNordHeads },
		{ { GenderName::Male, CharacterRaceName::Redguard }, TextureName::MaleRedguardHeads },
		{ { GenderName::Male, CharacterRaceName::WoodElf }, TextureName::MaleWoodElfHeads }
	};

	// Pairings of genders and races to portrait head filenames designed for 
	// the in-game interface.
	const std::unordered_map<std::pair<GenderName, CharacterRaceName>,
		TextureName> TrimmedHeadTextureNames =
	{
		{ { GenderName::Female, CharacterRaceName::Argonian }, TextureName::FemaleArgonianTrimmedHeads },
		{ { GenderName::Female, CharacterRaceName::Breton }, TextureName::FemaleBretonTrimmedHeads },
		{ { GenderName::Female, CharacterRaceName::DarkElf }, TextureName::FemaleDarkElfTrimmedHeads },
		{ { GenderName::Female, CharacterRaceName::HighElf }, TextureName::FemaleHighElfTrimmedHeads },
		{ { GenderName::Female, CharacterRaceName::Khajiit }, TextureName::FemaleKhajiitTrimmedHeads },
		{ { GenderName::Female, CharacterRaceName::Nord }, TextureName::FemaleNordTrimmedHeads },
		{ { GenderName::Female, CharacterRaceName::Redguard }, TextureName::FemaleRedguardTrimmedHeads },
		{ { GenderName::Female, CharacterRaceName::WoodElf }, TextureName::FemaleWoodElfTrimmedHeads },

		{ { GenderName::Male, CharacterRaceName::Argonian }, TextureName::MaleArgonianTrimmedHeads },
		{ { GenderName::Male, CharacterRaceName::Breton }, TextureName::MaleBretonTrimmedHeads },
		{ { GenderName::Male, CharacterRaceName::DarkElf }, TextureName::MaleDarkElfTrimmedHeads },
		{ { GenderName::Male, CharacterRaceName::HighElf }, TextureName::MaleHighElfTrimmedHeads },
		{ { GenderName::Male, CharacterRaceName::Khajiit }, TextureName::MaleKhajiitTrimmedHeads },
		{ { GenderName::Male, CharacterRaceName::Nord }, TextureName::MaleNordTrimmedHeads },
		{ { GenderName::Male, CharacterRaceName::Redguard }, TextureName::MaleRedguardTrimmedHeads },
		{ { GenderName::Male, CharacterRaceName::WoodElf }, TextureName::MaleWoodElfTrimmedHeads }
	};

	// Pairings of genders and races to portrait background filenames.
	const std::unordered_map<std::pair<GenderName, CharacterRaceName>,
		TextureName> BodyTextureNames =
	{
		{ { GenderName::Female, CharacterRaceName::Argonian }, TextureName::FemaleArgonianBackground },
		{ { GenderName::Female, CharacterRaceName::Breton }, TextureName::FemaleBretonBackground },
		{ { GenderName::Female, CharacterRaceName::DarkElf }, TextureName::FemaleDarkElfBackground },
		{ { GenderName::Female, CharacterRaceName::HighElf }, TextureName::FemaleHighElfBackground},
		{ { GenderName::Female, CharacterRaceName::Khajiit }, TextureName::FemaleKhajiitBackground},
		{ { GenderName::Female, CharacterRaceName::Nord }, TextureName::FemaleNordBackground},
		{ { GenderName::Female, CharacterRaceName::Redguard }, TextureName::FemaleRedguardBackground},
		{ { GenderName::Female, CharacterRaceName::WoodElf }, TextureName::FemaleWoodElfBackground },

		{ { GenderName::Male, CharacterRaceName::Argonian }, TextureName::MaleArgonianBackground },
		{ { GenderName::Male, CharacterRaceName::Breton }, TextureName::MaleBretonBackground },
		{ { GenderName::Male, CharacterRaceName::DarkElf }, TextureName::MaleDarkElfBackground },
		{ { GenderName::Male, CharacterRaceName::HighElf }, TextureName::MaleHighElfBackground },
		{ { GenderName::Male, CharacterRaceName::Khajiit }, TextureName::MaleKhajiitBackground },
		{ { GenderName::Male, CharacterRaceName::Nord }, TextureName::MaleNordBackground },
		{ { GenderName::Male, CharacterRaceName::Redguard }, TextureName::MaleRedguardBackground },
		{ { GenderName::Male, CharacterRaceName::WoodElf }, TextureName::MaleWoodElfBackground }
	};

	// Pixel offsets for shirt textures in the equipment screen.
	const std::unordered_map<std::pair<GenderName, bool>, Int2> ShirtOffsets =
	{
		{ { GenderName::Female, false }, Int2(220, 35) },
		{ { GenderName::Female, true }, Int2(220, 33) },
		{ { GenderName::Male, false }, Int2(186, 12) },
		{ { GenderName::Male, true }, Int2(215, 35) }
	};

	// Pixel offsets for pants textures in the equipment screen.
	const std::unordered_map<GenderName, Int2> PantsOffsets =
	{
		{ GenderName::Female, Int2(212, 74) },
		{ GenderName::Male, Int2(229, 82) }
	};
}

const std::string &PortraitFile::getHeads(GenderName gender, 
	CharacterRaceName race, bool trimmed)
{
	const TextureName textureName = trimmed ?
		TrimmedHeadTextureNames.at(std::make_pair(gender, race)) :
		HeadTextureNames.at(std::make_pair(gender, race));
	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

const std::string &PortraitFile::getBody(GenderName gender, CharacterRaceName race)
{
	const TextureName textureName = BodyTextureNames.at(std::make_pair(gender, race));
	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

const std::string &PortraitFile::getShirt(GenderName gender, bool magic)
{
	const TextureName textureName = [gender, magic]()
	{
		if (gender == GenderName::Female)
		{
			return magic ? TextureName::FemaleMagicShirt : TextureName::FemaleNonMagicShirt;
		}
		else
		{
			return magic ? TextureName::MaleMagicShirt : TextureName::MaleNonMagicShirt;
		}
	}();

	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

const std::string &PortraitFile::getPants(GenderName gender)
{
	const TextureName textureName = (gender == GenderName::Female) ?
		TextureName::FemalePants : TextureName::MalePants;
	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

const std::string &PortraitFile::getEquipment(GenderName gender)
{
	const TextureName textureName = (gender == GenderName::Female) ?
		TextureName::FemaleEquipment : TextureName::MaleEquipment;
	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

const Int2 &PortraitFile::getShirtOffset(GenderName gender, bool magic)
{
	const Int2 &offset = ShirtOffsets.at(std::make_pair(gender, magic));
	return offset;
}

const Int2 &PortraitFile::getPantsOffset(GenderName gender)
{
	const Int2 &offset = PantsOffsets.at(gender);
	return offset;
}
