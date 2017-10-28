#include <cassert>

#include "ArmorArtifactData.h"
#include "ItemType.h"

ArmorArtifactData::ArmorArtifactData(const std::string &displayName,
	const std::string &flavorText, const std::vector<int> &provinceIDs)
	: ArtifactData(displayName, flavorText, provinceIDs)
{ 

}

ArmorArtifactData::~ArmorArtifactData()
{

}

ItemType ArmorArtifactData::getItemType() const
{
	return ItemType::Armor;
}
