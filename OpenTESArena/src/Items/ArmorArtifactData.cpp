#include <cassert>

#include "ArmorArtifactData.h"

#include "ItemType.h"

ArmorArtifactData::ArmorArtifactData(const std::string &displayName,
	const std::string &flavorText, const std::vector<ProvinceName> &provinces)
	: ArtifactData(displayName, flavorText, provinces) 
{ 

}

ArmorArtifactData::~ArmorArtifactData()
{

}

ItemType ArmorArtifactData::getItemType() const
{
	return ItemType::Armor;
}
