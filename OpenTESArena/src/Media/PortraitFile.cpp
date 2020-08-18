#include <array>
#include <cstdio>

#include "PortraitFile.h"
#include "TextureFile.h"
#include "TextureName.h"

#include "components/dos/DOSUtils.h"

std::string PortraitFile::getHeads(bool male, int raceID, bool trimmed)
{
	DOSUtils::FilenameBuffer filename;
	std::snprintf(filename.data(), filename.size(), "FACES%s%d%d.CIF",
		male ? "" : "F", trimmed ? 0 : 1, raceID);

	return std::string(filename.data());
}

std::string PortraitFile::getBody(bool male, int raceID)
{
	DOSUtils::FilenameBuffer filename;
	std::snprintf(filename.data(), filename.size(), "%s0%d.IMG",
		male ? "CHARBK" : "CHRBKF", raceID);

	return std::string(filename.data());
}

const std::string &PortraitFile::getShirt(bool male, bool magic)
{
	const TextureName textureName = [male, magic]()
	{
		if (male)
		{
			return magic ? TextureName::MaleMagicShirt : TextureName::MaleNonMagicShirt;
		}
		else
		{
			return magic ? TextureName::FemaleMagicShirt : TextureName::FemaleNonMagicShirt;
		}
	}();

	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

const std::string &PortraitFile::getPants(bool male)
{
	const TextureName textureName = male ? TextureName::MalePants : TextureName::FemalePants;
	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

const std::string &PortraitFile::getEquipment(bool male)
{
	const TextureName textureName = male ? TextureName::MaleEquipment : TextureName::FemaleEquipment;
	const std::string &filename = TextureFile::fromName(textureName);
	return filename;
}

Int2 PortraitFile::getShirtOffset(bool male, bool magic)
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

Int2 PortraitFile::getPantsOffset(bool male)
{
	return male ? Int2(229, 82) : Int2(212, 74);
}
