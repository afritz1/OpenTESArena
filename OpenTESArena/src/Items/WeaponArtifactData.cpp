#include <cassert>
#include <map>

#include "WeaponArtifactData.h"
#include "ArtifactName.h"
#include "MetalType.h"
#include "WeaponType.h"

const auto WeaponArtifactDisplayNames = std::map<WeaponArtifactName, std::string>
{
	{ WeaponArtifactName::AurielsBow, "Auriel's Bow" },
	{ WeaponArtifactName::Chrysamere, "Chrysamere" },
	{ WeaponArtifactName::EbonyBlade, "Ebony Blade" },
	{ WeaponArtifactName::StaffOfMagnus, "Staff Of Magnus" },
	{ WeaponArtifactName::Volendrung, "Volendrung" }
};

const auto WeaponArtifactParentNames = std::map<WeaponArtifactName, ArtifactName>
{
	{ WeaponArtifactName::AurielsBow, ArtifactName::AurielsBow },
	{ WeaponArtifactName::Chrysamere, ArtifactName::Chrysamere },
	{ WeaponArtifactName::EbonyBlade, ArtifactName::EbonyBlade },
	{ WeaponArtifactName::StaffOfMagnus, ArtifactName::StaffOfMagnus },
	{ WeaponArtifactName::Volendrung, ArtifactName::Volendrung }
};

const auto WeaponArtifactTypes = std::map<WeaponArtifactName, WeaponType>
{
	{ WeaponArtifactName::AurielsBow, WeaponType::LongBow },
	{ WeaponArtifactName::Chrysamere, WeaponType::Claymore },
	{ WeaponArtifactName::EbonyBlade, WeaponType::Katana },
	{ WeaponArtifactName::StaffOfMagnus, WeaponType::Staff },
	{ WeaponArtifactName::Volendrung, WeaponType::Warhammer }
};

// Some of these metals are made up.
const auto WeaponArtifactMetals = std::map<WeaponArtifactName, MetalType>
{
	{ WeaponArtifactName::AurielsBow, MetalType::Elven },
	{ WeaponArtifactName::Chrysamere, MetalType::Steel },
	{ WeaponArtifactName::EbonyBlade, MetalType::Ebony },
	{ WeaponArtifactName::StaffOfMagnus, MetalType::Mithril },
	{ WeaponArtifactName::Volendrung, MetalType::Dwarven }
};

const auto WeaponArtifactFlavorTexts = std::map<WeaponArtifactName, std::string>
{
	{ WeaponArtifactName::AurielsBow, "Auriel's Bow appears as a modest Elven Longbow, but \
it is one of the mightiest weapons ever to exist in Tamriel's history. Allegedly created \
and used, like its sister Auriel's Shield, by the great Elvish demi-god, the Bow can turn \
any arrow into a missile of death and any wielder invulnerable to any lesser attacks. \
Without Auriel's power behind it, however, the bow uses its own store of energy for its \
power. Once exhausted of this energy, the bow will vanish and reappear where ever chance \
puts it. Its most recent appearances have been subject of gossip for hundreds of years." },
	{ WeaponArtifactName::Chrysamere, "Chrysamere, the Paladin's Blade and Sword of Heroes, \
is an ancient claymore with offensive capabilities only surpassed by its defenses. It lends \
the wielder health, protects him or her from fire, and reflects any deletory spells cast \
against the wielder back to the caster. Seldom has Chrysamere been wielded by any bladesman \
for any length of time, for it chooses not to favor one champion." },
	{ WeaponArtifactName::EbonyBlade, "The Ebony Blade, sometimes called the Vampire or the \
Leech, resembles an ebony katana, but its power is very dark indeed. Every time the Ebony Blade \
strikes an opponent, part of the damage inflicted flows into the wielder as raw power. The \
Blade itself may not be any more evil than those who have used it, but at some point in its \
long existence, a charm was cast on it so it would not remain with one bladesman. The wizard \
who cast this charm sought to save the souls of any too infatuated by the Blade, and perhaps \
he was right..." },
	{ WeaponArtifactName::StaffOfMagnus, "The Staff of Magnus, one of the elder artifacts of \
Tamriel, was a metaphysical battery of sorts for its creator, the Arch-Mage Magnus. When used, \
it regenerates both a mage's health and mystical energy at remarkable rates. In time, the \
Staff will abandon the mage who wields it before he or she becomes too powerful and upsets the \
mystical balance it is sworn to protect." },
	{ WeaponArtifactName::Volendrung, "The Hammer of Might, Volendrung is said to have been \
created by the Dwarves of the now abandoned clan of Rourken, hundreds of years before they \
disappeared from the world of Tamriel. It has the ability to grant health to its wielder, but \
it is best known for the paralyzing and strength leeching effects it has when cast at an \
enemy. Like the Dwarves who created it, Volendrung is prone to disappearing suddenly, \
resurfacing sometimes in days, sometimes in eons." }
};

WeaponArtifactData::WeaponArtifactData(WeaponArtifactName artifactName)
{
	this->artifactName = artifactName;
}

WeaponArtifactData::~WeaponArtifactData()
{

}

const WeaponArtifactName &WeaponArtifactData::getArtifactName() const
{
	return this->artifactName;
}

WeaponType WeaponArtifactData::getWeaponType() const
{
	auto weaponType = WeaponArtifactTypes.at(this->getArtifactName());
	return weaponType;
}

MetalType WeaponArtifactData::getMetalType() const
{
	auto metalType = WeaponArtifactMetals.at(this->getArtifactName());
	return metalType;
}

ArtifactName WeaponArtifactData::getParentArtifactName() const
{
	auto parentName = WeaponArtifactParentNames.at(this->getArtifactName());
	return parentName;
}

std::string WeaponArtifactData::getDisplayName() const
{
	auto displayName = WeaponArtifactDisplayNames.at(this->getArtifactName());
	assert(displayName.size() > 0);
	return displayName;
}

std::string WeaponArtifactData::getFlavorText() const
{
	auto flavorText = WeaponArtifactFlavorTexts.at(this->getArtifactName());
	assert(flavorText.size() > 0);
	return flavorText;
}
