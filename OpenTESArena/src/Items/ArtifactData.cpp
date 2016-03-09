#include <cassert>
#include <map>

#include "ArtifactData.h"
#include "ArtifactName.h"
#include "../World/ProvinceName.h"

// Each artifact is found in one or more provinces.
const auto ProvinceArtifacts = std::map<ProvinceName, std::vector<ArtifactName>>
{
	// Black Marsh.
	{ ProvinceName::BlackMarsh, { ArtifactName::EbonyMail, ArtifactName::LordsMail,
	ArtifactName::RingOfPhynaster } },

	{ ProvinceName::Elsweyr, { ArtifactName::AurielsBow, ArtifactName::Chrysamere,
	ArtifactName::StaffOfMagnus, ArtifactName::OghmaInfinium, ArtifactName::RingOfPhynaster,
	ArtifactName::WarlocksRing } },

	{ ProvinceName::Hammerfell, { ArtifactName::SpellBreaker, ArtifactName::Volendrung,
	ArtifactName::RingOfKhajiit } },

	{ ProvinceName::HighRock, { ArtifactName::AurielsShield, ArtifactName::KingOrgnumsCoffer,
	ArtifactName::NecromancersAmulet, ArtifactName::RingOfKhajiit } },

	{ ProvinceName::ImperialProvince, { } },

	{ ProvinceName::Morrowind, { ArtifactName::Chrysamere, ArtifactName::Volendrung,
	ArtifactName::WarlocksRing } },

	{ ProvinceName::Skyrim, { ArtifactName::AurielsShield, ArtifactName::LordsMail,
	ArtifactName::AurielsBow, ArtifactName::EbonyBlade, ArtifactName::OghmaInfinium,
	ArtifactName::RingOfPhynaster } },

	{ ProvinceName::SummersetIsle, { ArtifactName::AurielsShield, ArtifactName::KingOrgnumsCoffer,
	ArtifactName::NecromancersAmulet, ArtifactName::RingOfPhynaster, ArtifactName::SkeletonsKey } },

	{ ProvinceName::Valenwood, { ArtifactName::SpellBreaker, ArtifactName::AurielsBow,
	ArtifactName::EbonyBlade, ArtifactName::StaffOfMagnus, ArtifactName::RingOfKhajiit,
	ArtifactName::SkeletonsKey } }
};

ArtifactData::ArtifactData()
{
	
}

ArtifactData::~ArtifactData()
{

}

std::vector<ArtifactName> ArtifactData::getArtifactsInProvince(ProvinceName province)
{
	auto artifacts = ProvinceArtifacts.at(province);
	assert(artifacts.size() > 0);
	return artifacts;
}
