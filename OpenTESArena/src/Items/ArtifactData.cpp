#include <cassert>

#include "ArtifactData.h"

ArtifactData::ArtifactData(const std::string &displayName, const std::string &flavorText, 
	const std::vector<int> &provinceIDs)
	: displayName(displayName), flavorText(flavorText), provinceIDs(provinceIDs) { }

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

const std::vector<int> &ArtifactData::getProvinceIDs() const
{
	return this->provinceIDs;
}
