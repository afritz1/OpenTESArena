#include <cassert>

#include "MiscellaneousArtifactData.h"

#include "ItemType.h"
#include "MiscellaneousItemType.h"

MiscellaneousArtifactData::MiscellaneousArtifactData(const std::string &displayName, 
	const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
	const MiscellaneousItemType &miscItemType)
	: ArtifactData(displayName, flavorText, provinces)
{
	this->miscItemType = miscItemType;

	assert(this->miscItemType == miscItemType);
}

MiscellaneousArtifactData::~MiscellaneousArtifactData()
{

}

std::unique_ptr<ArtifactData> MiscellaneousArtifactData::clone() const
{
	return std::unique_ptr<ArtifactData>(new MiscellaneousArtifactData(
		this->getDisplayName(), this->getFlavorText(), this->getProvinces(),
		this->getMiscellaneousItemType()));
}

const MiscellaneousItemType &MiscellaneousArtifactData::getMiscellaneousItemType() const
{
	return this->miscItemType;
}

ItemType MiscellaneousArtifactData::getItemType() const
{
	return ItemType::Miscellaneous;
}
