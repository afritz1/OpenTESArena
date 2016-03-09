#include <cassert>
#include <iostream>
#include <map>

#include "BodyArmorArtifactData.h"
#include "ArtifactName.h"
#include "ArmorMaterial.h"
#include "HeavyArmorMaterial.h"
#include "LightArmorMaterial.h"
#include "MediumArmorMaterial.h"
#include "Metal.h"
#include "../Entities/BodyPartName.h"

const auto BodyArmorArtifactDisplayNames = std::map<BodyArmorArtifactName, std::string>
{
	{ BodyArmorArtifactName::EbonyMail, "Ebony Mail" },
	{ BodyArmorArtifactName::LordsMail, "Lord's Mail" }
};

// Parent artifact names.
const auto BodyArmorArtifactParentNames = std::map<BodyArmorArtifactName, ArtifactName>
{
	{ BodyArmorArtifactName::EbonyMail, ArtifactName::EbonyMail },
	{ BodyArmorArtifactName::LordsMail, ArtifactName::LordsMail }
};

const auto BodyArmorArtifactBodyParts = std::map<BodyArmorArtifactName, BodyPartName>
{
	{ BodyArmorArtifactName::EbonyMail, BodyPartName::Chest },
	{ BodyArmorArtifactName::LordsMail, BodyPartName::Chest }
};

// This mapping has to use unique pointers in global memory (ick!) because ArmorMaterials
// are abstract, and using ArmorMaterialType mappings instead would allow there to be 
// holes in the "Artifact -> ArmorMaterial" mappings, which would be a bad thing. These
// pointers won't be freed until the program closes, which is fine. I couldn't use a 
// unique_ptr in the std::map because the compiler complained about not having the copy 
// constructor, and std::move didn't help either. I think that's Visual C++'s fault.
const auto EbonyMaterial = std::unique_ptr<ArmorMaterial>(new HeavyArmorMaterial(MetalType::Ebony));
const auto SteelMaterial = std::unique_ptr<ArmorMaterial>(new HeavyArmorMaterial(MetalType::Steel));
const auto BodyArmorArtifactMaterials = std::map<BodyArmorArtifactName, const ArmorMaterial*>
{
	{ BodyArmorArtifactName::EbonyMail, EbonyMaterial.get() },
	{ BodyArmorArtifactName::LordsMail, SteelMaterial.get() }
};

const auto BodyArmorArtifactFlavorTexts = std::map<BodyArmorArtifactName, std::string>
{
	{ BodyArmorArtifactName::EbonyMail, "The Ebony Mail is an artifact created before recorded \
history, according to legend, by the Dark Elven goddess Boethiah. It is she who determines who \
should possess the Mail and for how long a time. If judged worthy, its power grants the wearer \
invulnerability to all common magical attacks that drain talents and health. It is Boethiah \
alone who determines when a person is ineligible to bear the Ebony Mail any longer, and the \
goddess can be very capricious..." },
	{ BodyArmorArtifactName::LordsMail, "The Lord's Mail, sometimes called the Armor of Morihaus, \
the Gift of Kynareth, is an ancient cuirass of unsurpassable quality. It grants the \
wearer the power to regenerate lost health, resist the effects of spells, and cure oneself \
of poison when used. It is said that whenever Kynareth deigns the wearer unworthy, the Lord's \
Mail will be taken away and hidden for the next chosen one." }
};

BodyArmorArtifactData::BodyArmorArtifactData(BodyArmorArtifactName artifactName)
{
	this->artifactName = artifactName;
}

BodyArmorArtifactData::~BodyArmorArtifactData()
{

}

const BodyArmorArtifactName &BodyArmorArtifactData::getArtifactName() const
{
	return this->artifactName;
}

BodyPartName BodyArmorArtifactData::getBodyPartName() const
{
	auto partName = BodyArmorArtifactBodyParts.at(this->getArtifactName());
	return partName;
}

std::unique_ptr<ArmorMaterial> BodyArmorArtifactData::getArmorMaterial() const
{
	const auto *material = BodyArmorArtifactMaterials.at(this->getArtifactName());
	auto armorMaterial = material->clone();
	assert(armorMaterial.get() != nullptr);
	return armorMaterial;
}

ArtifactName BodyArmorArtifactData::getParentArtifactName() const
{
	auto parentName = BodyArmorArtifactParentNames.at(this->getArtifactName());
	return parentName;
}

std::string BodyArmorArtifactData::getDisplayName() const
{
	auto displayName = BodyArmorArtifactDisplayNames.at(this->getArtifactName());
	assert(displayName.size() > 0);
	return displayName;
}

std::string BodyArmorArtifactData::getFlavorText() const
{
	auto flavorText = BodyArmorArtifactFlavorTexts.at(this->getArtifactName());
	assert(flavorText.size() > 0);
	return flavorText;
}
