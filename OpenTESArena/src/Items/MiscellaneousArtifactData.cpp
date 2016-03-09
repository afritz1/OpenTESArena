#include <cassert>
#include <map>

#include "MiscellaneousArtifactData.h"
#include "ArtifactName.h"
#include "MiscellaneousItemType.h"

const auto MiscellaneousArtifactDisplayNames = std::map<MiscellaneousArtifactName, std::string>
{
	{ MiscellaneousArtifactName::KingOrgnumsCoffer, "King Orgnum's Coffer" },
	{ MiscellaneousArtifactName::OghmaInfinium, "Oghma Infinium" },
	{ MiscellaneousArtifactName::SkeletonsKey, "Skeleton's Key" }
};

const auto MiscellaneousArtifactParentNames = std::map<MiscellaneousArtifactName, ArtifactName>
{
	{ MiscellaneousArtifactName::KingOrgnumsCoffer, ArtifactName::KingOrgnumsCoffer },
	{ MiscellaneousArtifactName::OghmaInfinium, ArtifactName::OghmaInfinium },
	{ MiscellaneousArtifactName::SkeletonsKey, ArtifactName::SkeletonsKey }
};

const auto MiscellaneousArtifactTypes = std::map<MiscellaneousArtifactName, MiscellaneousItemType>
{
	{ MiscellaneousArtifactName::KingOrgnumsCoffer, MiscellaneousItemType::Unknown },
	{ MiscellaneousArtifactName::OghmaInfinium, MiscellaneousItemType::Book },
	{ MiscellaneousArtifactName::SkeletonsKey, MiscellaneousItemType::BoneKey }
};

const auto MiscellaneousArtifactFlavorTexts = std::map<MiscellaneousArtifactName, std::string>
{
	{ MiscellaneousArtifactName::KingOrgnumsCoffer, "King Orgnum's Coffer is a small-sized \
chest, ordinary in appearance. It is remarkably light, almost weightless, which offers a clue \
to its true magic. Once a day, the Coffer will create gold from naught. When King Orghum \
himself possessed the Coffer, the supply within was limitless. Those who have found it since \
report that the Coffer eventually disappears after having dispersed enough gold to shame even \
the wealthiest of merchants. Where and why it vanishes is still a mystery." },
	{ MiscellaneousArtifactName::OghmaInfinium, "The Oghma Infinium is a tome of knowledge \
written by the Ageless One, the wizard-sage Xarses. All who read the Infinium are filled with \
the energy of the artifact which can be manipulated to raise one's abilities to near demi-god \
proportions. Once used, legend has it, the Infinium will disappear from its wielder." },
	{ MiscellaneousArtifactName::SkeletonsKey, "The power of the Skeleton's Key is very \
simple, indeed. With it, any non-magically locked door or chest is instantly accessible to \
even the clumsiest of lockpickers. A particularly skillful lockpicker may even open some \
magically barred doors with the Key. The two limitations placed on the Key by wizards who \
sought to protect their storehouses were that the Key could only be used once a day and it \
would never be the property of one thief for too long. Some of those who have possessed the \
Key have made themselves rich before it disappeared, others have broken into places they \
never should have entered ..." }
};

MiscellaneousArtifactData::MiscellaneousArtifactData(MiscellaneousArtifactName artifactName)
{
	this->artifactName = artifactName;
}

MiscellaneousArtifactData::~MiscellaneousArtifactData()
{

}

const MiscellaneousArtifactName &MiscellaneousArtifactData::getArtifactName() const
{
	return this->artifactName;
}

MiscellaneousItemType MiscellaneousArtifactData::getMiscellaneousItemType() const
{
	auto miscItemType = MiscellaneousArtifactTypes.at(this->getArtifactName());
	return miscItemType;
}

ArtifactName MiscellaneousArtifactData::getParentArtifactName() const
{
	auto parentName = MiscellaneousArtifactParentNames.at(this->getArtifactName());
	return parentName;
}

std::string MiscellaneousArtifactData::getDisplayName() const
{
	auto displayName = MiscellaneousArtifactDisplayNames.at(this->getArtifactName());
	assert(displayName.size() > 0);
	return displayName;
}

std::string MiscellaneousArtifactData::getFlavorText() const
{
	auto flavorText = MiscellaneousArtifactFlavorTexts.at(this->getArtifactName());
	assert(flavorText.size() > 0);
	return flavorText;
}
