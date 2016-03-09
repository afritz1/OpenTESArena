#include <cassert>
#include <map>

#include "AccessoryArtifactData.h"
#include "AccessoryType.h"
#include "ArtifactName.h"
#include "MetalType.h"

const auto AccessoryArtifactDisplayNames = std::map<AccessoryArtifactName, std::string>
{
	{ AccessoryArtifactName::NecromancersAmulet, "Necromancer's Amulet" },
	{ AccessoryArtifactName::RingOfKhajiit, "Ring of Khajiit" },
	{ AccessoryArtifactName::RingOfPhynaster, "Ring of Phynaster" },
	{ AccessoryArtifactName::WarlocksRing, "Warlock's Ring" }
};

// Parent artifact names.
const auto AccessoryArtifactParentNames = std::map<AccessoryArtifactName, ArtifactName>
{
	{ AccessoryArtifactName::NecromancersAmulet, ArtifactName::NecromancersAmulet },
	{ AccessoryArtifactName::RingOfKhajiit, ArtifactName::RingOfKhajiit },
	{ AccessoryArtifactName::RingOfPhynaster, ArtifactName::RingOfPhynaster },
	{ AccessoryArtifactName::WarlocksRing, ArtifactName::WarlocksRing }
};

// AccessoryArtifactName -> AccessoryType mappings.
const auto AccessoryArtifactTypes = std::map<AccessoryArtifactName, AccessoryType>
{
	{ AccessoryArtifactName::NecromancersAmulet, AccessoryType::Amulet },
	{ AccessoryArtifactName::RingOfKhajiit, AccessoryType::Ring },
	{ AccessoryArtifactName::RingOfPhynaster, AccessoryType::Ring },
	{ AccessoryArtifactName::WarlocksRing, AccessoryType::Ring }
};

// Each accessory artifact has an associated metal. These are made up.
const auto AccessoryArtifactMetals = std::map<AccessoryArtifactName, MetalType>
{
	{ AccessoryArtifactName::NecromancersAmulet, MetalType::Mithril },
	{ AccessoryArtifactName::RingOfKhajiit, MetalType::Steel },
	{ AccessoryArtifactName::RingOfPhynaster, MetalType::Steel },
	{ AccessoryArtifactName::WarlocksRing, MetalType::Steel }
};

// Accessory flavor texts.
const auto AccessoryArtifactFlavorTexts = std::map<AccessoryArtifactName, std::string>
{
	{ AccessoryArtifactName::NecromancersAmulet, "The legendary Necromancer's Amulet, the last \
surviving relic of the mad sorcerer Mannimarco, grants any spellcaster who wears it the ability \
to absorb magical energy and regenerate from injury. The Amulet is mystically fortified to give \
the person wearing it an armor rating equivalent to plate armor, without the weight and \
restriction of movement. This makes the artifact popular amongst thieves and mages alike. It is \
the one flaw of the Amulet that it is unstable in this world - forever doomed to fade in and out \
of existence, reappearing at locations distant from that of its disappearance." },
	{ AccessoryArtifactName::RingOfKhajiit, "The Ring of the Khajiiti is an ancient relic, \
hundreds of years older than Rajhin, the thief who made the Ring famous. It was Rajhin who used \
the Ring's powers to make himself as invisible, silent, and quick as a breath of wind. Using the \
Ring he became the most successful burglar in Elsweyr's history. Rajhin's eventual fate is a \
mystery, but according to legend, the Ring rebelled against such constant use and disappeared, \
leaving Rajhin helpless before his enemies..." },
	{ AccessoryArtifactName::RingOfPhynaster, "The Ring of Phynaster was made hundreds of years \
ago by a person who needed good defenses to survive his adventurous life. Thanks to the Ring, \
Phynaster lived for hundreds of years, and since then it has passed from person to person. The \
ring improves its wearer's overall resistance to damage and grants total immunity to poison, \
spells, and electricity. Still, Phynaster was cunning and said to have cursed the Ring. It \
eventually disappears from its holder's possessions and returns to another resting place, \
uncontent to stay anywhere but with Phynaster himself." },
	{ AccessoryArtifactName::WarlocksRing, "The Warlock's Ring of the Arch-Mage Syrabane is \
one of the most popular relics of myth and fable. In Tamriel's ancient history, Syrabane saved \
all of the continent by judicious use of his Ring, and ever since, it has helped adventurers \
with less lofty goals. It is best known for its ability to reflect spells cast at its wearer \
and to improve his or her speed and health, though it may have additional powers. No adventurer \
can wear the Warlock's Ring for long, for it is said the Ring is Syrabane's alone to command." }
};

AccessoryArtifactData::AccessoryArtifactData(AccessoryArtifactName artifactName)
{
	this->artifactName = artifactName;
}

AccessoryArtifactData::~AccessoryArtifactData()
{

}

const AccessoryArtifactName &AccessoryArtifactData::getArtifactName() const
{
	return this->artifactName;
}

AccessoryType AccessoryArtifactData::getAccessoryType() const
{
	auto accessoryType = AccessoryArtifactTypes.at(this->getArtifactName());
	return accessoryType;
}

MetalType AccessoryArtifactData::getMetalType() const
{
	auto metalType = AccessoryArtifactMetals.at(this->getArtifactName());
	return metalType;
}

ArtifactName AccessoryArtifactData::getParentArtifactName() const
{
	auto parentName = AccessoryArtifactParentNames.at(this->getArtifactName());
	return parentName;
}

std::string AccessoryArtifactData::getDisplayName() const
{
	auto displayName = AccessoryArtifactDisplayNames.at(this->getArtifactName());
	assert(displayName.size() > 0);
	return displayName;
}

std::string AccessoryArtifactData::getFlavorText() const
{
	auto flavorText = AccessoryArtifactFlavorTexts.at(this->getArtifactName());
	assert(flavorText.size() > 0);
	return flavorText;
}
