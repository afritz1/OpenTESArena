#include <cassert>

#include "ArtifactData.h"

ArtifactData::ArtifactData(const std::string &displayName, const std::string &flavorText, 
	const std::vector<ProvinceName> &provinces)
{
	this->displayName = displayName;
	this->flavorText = flavorText;
	this->provinces = provinces;
}

ArtifactData::~ArtifactData()
{

}

const std::string &ArtifactData::getDisplayName() const
{
	return this->displayName;
}

const std::string &ArtifactData::getFlavorText() const
{
	return this->flavorText;
}

const std::vector<ProvinceName> &ArtifactData::getProvinces() const
{
	return this->provinces;
}
