#include <cassert>
#include <map>

#include "ShieldArtifactData.h"
#include "ArtifactName.h"
#include "MetalType.h"
#include "ShieldType.h"

const auto ShieldArtifactDisplayNames = std::map<ShieldArtifactName, std::string>
{
	{ ShieldArtifactName::AurielsShield, "Auriel's Shield" },
	{ ShieldArtifactName::SpellBreaker, "Spell Breaker" }
};

const auto ShieldArtifactParentNames = std::map<ShieldArtifactName, ArtifactName>
{
	{ ShieldArtifactName::AurielsShield, ArtifactName::AurielsShield },
	{ ShieldArtifactName::SpellBreaker, ArtifactName::SpellBreaker }
};

const auto ShieldArtifactTypes = std::map<ShieldArtifactName, ShieldType>
{
	{ ShieldArtifactName::AurielsShield, ShieldType::Kite }, // Verify this.
	{ ShieldArtifactName::SpellBreaker, ShieldType::Tower }
};

const auto ShieldArtifactMetals = std::map<ShieldArtifactName, MetalType>
{
	{ ShieldArtifactName::AurielsShield, MetalType::Ebony },
	{ ShieldArtifactName::SpellBreaker, MetalType::Dwarven }
};

const auto ShieldArtifactFlavorTexts = std::map<ShieldArtifactName, std::string>
{
	{ ShieldArtifactName::AurielsShield, "Auriel's Shield, an Ebony shield said to have \
once belonged to the quasi-mythical Elvish deity Auriel, can make its wielder nigh \
invulnerable. In its resistance to fire and magick, Auriel's Shield is unsurpassed. To \
defend its wielder from any attacks it cannot absorb, the Shield lends him or her health. \
Like many artifacts of Tamriel, the Shield has life and personality of its own, and does not \
feel bound to its user. A popular fable tells the tale of it abandoning one wielder in her \
greatest hour of need, but this is perhaps, apocryphal." },
	{ ShieldArtifactName::SpellBreaker, "Spell Breaker, superficially a Dwarven tower shield, \
is one of the most prized ancient relics of Tamriel. Aside from its historic importance dating \
from the Battle of Rourken-Shalidor, the Spell Breaker protects its wielder almost \
completely from any spellcaster, either by dispelling magicks or silencing any mage about \
to cast a spell. It is said that the Spell Breaker still searches for its original owner, \
and will not remain the property of any one else for long. For most, possessing Spell Breaker \
for any time is power enough." }
};

ShieldArtifactData::ShieldArtifactData(ShieldArtifactName artifactName)
{
	this->artifactName = artifactName;
}

ShieldArtifactData::~ShieldArtifactData()
{

}

const ShieldArtifactName &ShieldArtifactData::getArtifactName() const
{
	return this->artifactName;
}

ShieldType ShieldArtifactData::getShieldType() const
{
	auto shieldType = ShieldArtifactTypes.at(this->getArtifactName());
	return shieldType;
}

MetalType ShieldArtifactData::getMetalType() const
{
	auto metalType = ShieldArtifactMetals.at(this->getArtifactName());
	return metalType;
}

ArtifactName ShieldArtifactData::getParentArtifactName() const
{
	auto parentName = ShieldArtifactParentNames.at(this->getArtifactName());
	return parentName;
}

std::string ShieldArtifactData::getDisplayName() const
{
	auto displayName = ShieldArtifactDisplayNames.at(this->getArtifactName());
	assert(displayName.size() > 0);
	return displayName;
}

std::string ShieldArtifactData::getFlavorText() const
{
	auto flavorText = ShieldArtifactFlavorTexts.at(this->getArtifactName());
	assert(flavorText.size() > 0);
	return flavorText;
}
