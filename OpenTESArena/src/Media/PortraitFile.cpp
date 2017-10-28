#include <cassert>
#include <unordered_map>

#include "PortraitFile.h"
#include "TextureFile.h"
#include "TextureName.h"
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
	struct hash<std::pair<GenderName, int>>
	{
		size_t operator()(const std::pair<GenderName, int> &x) const
		{
			return std::hash<GenderName>()(x.first) ^ std::hash<int>()(x.second);
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

std::string PortraitFile::getHeads(GenderName gender, int raceID, bool trimmed)
{
	std::string filename("FACES");

	// Append characters to the filename based on the arguments.
	filename += (gender == GenderName::Female) ? "F" : "";
	filename += trimmed ? "0" : "1";
	filename += std::to_string(raceID);
	filename += ".CIF";

	return filename;
}

std::string PortraitFile::getBody(GenderName gender, int raceID)
{
	std::string filename;

	// Append characters to the filename based on the arguments.
	filename += (gender == GenderName::Female) ? "CHRBKF" : "CHARBK";
	filename += "0";
	filename += std::to_string(raceID);
	filename += ".IMG";

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
