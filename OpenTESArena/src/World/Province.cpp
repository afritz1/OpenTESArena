#include <cassert>
#include <map>

#include "Province.h"
#include "LocationName.h"

// City-state/Town/Village map location names. Doesn't include dungeons.
const auto ProvinceCivilizations = std::map<ProvinceName, std::vector<LocationName>>
{
	// Black Marsh.
	{ ProvinceName::BlackMarsh, { LocationName::AltenCorimont, LocationName::AltenMarkmont,
	LocationName::AltenMeirhall, LocationName::Archon, LocationName::Blackrose,
	LocationName::Branchgrove, LocationName::Branchmont, LocationName::Chasecreek,
	LocationName::Chasepoint, LocationName::Gideon, LocationName::Glenbridge,
	LocationName::Greenglade, LocationName::Greenspring, LocationName::Helstrom,
	LocationName::Lilmoth, LocationName::Longmont, LocationName::Moonmarch,
	LocationName::PortdunMont, LocationName::Riverbridge, LocationName::Riverwalk,
	LocationName::Rockgrove, LocationName::Rockguard, LocationName::Rockpark,
	LocationName::Rockpoint, LocationName::Rockspring, LocationName::Seafalls,
	LocationName::Seaspring, LocationName::Soulrest, LocationName::Stonewastes,
	LocationName::Stormhold, LocationName::TenmarWall, LocationName::Thorn } },

	// Elsweyr.
	{ ProvinceName::Elsweyr, { LocationName::Alabaster, LocationName::BlackHeights,
	LocationName::BrukreichBridge, LocationName::Chasegrove, LocationName::Chasemoor,
	LocationName::CoriDarglade, LocationName::Corinthe, LocationName::DarkarnPlace,
	LocationName::DarvulkHaven, LocationName::DuncoriWalk, LocationName::Dune,
	LocationName::EinMeirvale, LocationName::Greenhall, LocationName::HeimthorMount,
	LocationName::HelkarnLand, LocationName::KingsWalk, LocationName::MarkgranBrook,
	LocationName::MeirLynmount, LocationName::NeumarWalk, LocationName::Orcrest,
	LocationName::PortneuView, LocationName::Rimmen, LocationName::Riverhold,
	LocationName::RiverKeep, LocationName::Seaplace, LocationName::Senchal,
	LocationName::SouthGuard, LocationName::TardornWood, LocationName::TenmarForest,
	LocationName::Torval, LocationName::ValleyGuard, LocationName::VerkarthHills } },

	// Hammerfell.
	{ ProvinceName::Hammerfell, { LocationName::Chaseguard, LocationName::Chasetown,
	LocationName::CliffKeep, LocationName::CortenMont, LocationName::DragonGate,
	LocationName::DragonGrove, LocationName::Dragonstar, LocationName::Elinhir,
	LocationName::Gilane, LocationName::Hegathe, LocationName::HeldornMount,
	LocationName::KarnverFalls, LocationName::LainebonPlace, LocationName::Lainlyn,
	LocationName::NimbelMoor, LocationName::NorthHall, LocationName::Rihad,
	LocationName::Riverpoint, LocationName::Riverview, LocationName::Roseguard,
	LocationName::Sentinel, LocationName::Shadymarch, LocationName::Skaven,
	LocationName::Stonedale, LocationName::Stonemoor, LocationName::Sunkeep,
	LocationName::Taneth, LocationName::ThorstadPlace, LocationName::VerkarthCity,
	LocationName::VulkneuTown, LocationName::VulnimGate } },

	// High Rock.
	{ ProvinceName::HighRock, { LocationName::BlackWastes, LocationName::Camlorn,
	LocationName::CloudSpring, LocationName::Daggerfall, LocationName::DunkarnHaven,
	LocationName::DunlainFalls, LocationName::EagleBrook, LocationName::EbonWastes,
	LocationName::Evermore, LocationName::Farrun, LocationName::Glenpoint,
	LocationName::IlessenHills, LocationName::Jehanna, LocationName::KarthgranVale,
	LocationName::KingsGuard, LocationName::MarkwastenMoor, LocationName::MeirDarguard,
	LocationName::MeirThorvale, LocationName::Moonguard, LocationName::NormarHeights,
	LocationName::NorthPoint, LocationName::NorvulkHills, LocationName::OldGate,
	LocationName::PortdunCreek, LocationName::RavenSpring, LocationName::ReichGradkeep,
	LocationName::Shornhelm, LocationName::ThorkanPark, LocationName::VermeirWastes,
	LocationName::Wayrest, LocationName::WhiteHaven, LocationName::WindKeep } },

	// Imperial Province.
	{ ProvinceName::ImperialProvince, { LocationName::ImperialCity } },

	// Morrowind.
	{ ProvinceName::Morrowind, { LocationName::AmberForest, LocationName::Blacklight,
	LocationName::CorkarthRun, LocationName::CormarView, LocationName::DarnimWatch,
	LocationName::DragonGlade, LocationName::EagleMoor, LocationName::Ebonheart,
	LocationName::Firewatch, LocationName::GlenHaven, LocationName::Greenheights,
	LocationName::HeimlynKeep, LocationName::HelnimWall, LocationName::KarththorDale,
	LocationName::KarththorHeights, LocationName::Kragenmoor, LocationName::MarkgranForest,
	LocationName::Mournhold, LocationName::Narsis, LocationName::Necrom,
	LocationName::Oaktown, LocationName::OldKeep, LocationName::OldRun,
	LocationName::ReichParkeep, LocationName::Riverbridge2, LocationName::SailenVulgate,
	LocationName::SilgradTower, LocationName::SilnimDale, LocationName::Stonefalls,
	LocationName::Stoneforest, LocationName::Tear, LocationName::VerarchenHall } },

	// Skyrim.
	{ ProvinceName::Skyrim, { LocationName::AmberGuard, LocationName::Amol,
	LocationName::BlackMoor, LocationName::Dawnstar, LocationName::DragonBridge,
	LocationName::DragonWood, LocationName::DunparWall, LocationName::DunstadGrove,
	LocationName::Falcrenth, LocationName::Granitehall, LocationName::Greenwall,
	LocationName::HelarchenCreek, LocationName::KarthwastenHall, LocationName::Lainalten,
	LocationName::LaintarDale, LocationName::MarkarthSide, LocationName::NeugradWatch,
	LocationName::NimaltenCity, LocationName::NorthKeep, LocationName::Oakwood,
	LocationName::PargranVillage, LocationName::ReichCorigate, LocationName::Riften,
	LocationName::Riverwood, LocationName::Snowhawk, LocationName::Solitude,
	LocationName::Stonehills, LocationName::Sunguard, LocationName::VernimWood,
	LocationName::Whiterun, LocationName::Windhelm, LocationName::Winterhold } },

	// Summerset Isle.
	{ ProvinceName::SummersetIsle, { LocationName::Alinor, LocationName::ArchenGrangrove,
	LocationName::BelportRun, LocationName::Cloudrest, LocationName::CorgradWastes,
	LocationName::Dusk, LocationName::EbonStadmont, LocationName::FirstHold,
	LocationName::Glenview, LocationName::GraddunSpring, LocationName::HollyFalls,
	LocationName::KarndarWatch, LocationName::KarnwastenMoor, LocationName::KingsHaven,
	LocationName::Lillandril, LocationName::MarbrukBrook, LocationName::MarnorKeep,
	LocationName::OldFalls, LocationName::Riverfield, LocationName::Riverwatch,
	LocationName::Rosefield, LocationName::SeaKeep, LocationName::Shimmerene,
	LocationName::SilsailenPoint, LocationName::SilverWood, LocationName::Skywatch,
	LocationName::Sunhold, LocationName::ThorheimGuard, LocationName::VulkhelGuard,
	LocationName::WastenCoridale, LocationName::WestGuard, LocationName::WhiteGuard } },

	// Valenwood.
	{ ProvinceName::Valenwood, { LocationName::ArchenCormount, LocationName::Arenthia,
	LocationName::BlackPark, LocationName::CoriSilmoor, LocationName::CormeirSpring,
	LocationName::EagleVale, LocationName::EbonRo, LocationName::Eldenroot,
	LocationName::EmperorsRun, LocationName::Falinesti, LocationName::Glenpoint2,
	LocationName::GreenHall, LocationName::Greenheart, LocationName::Haven,
	LocationName::HeimdarCity, LocationName::KarthdarSquare, LocationName::Longhaven,
	LocationName::Longvale, LocationName::LynpanMarch, LocationName::MarbrukField,
	LocationName::MeadowRun, LocationName::Moonmont, LocationName::Silvenar,
	LocationName::Southpoint, LocationName::StoneFell, LocationName::Stonesquare,
	LocationName::TarlainHeights, LocationName::ThormarKeep, LocationName::VulkwastenWood,
	LocationName::VullainHaven, LocationName::WastenBrukbrook, LocationName::Woodhearth } }
};

const auto ProvinceMainQuestDungeons = std::map<ProvinceName, std::vector<LocationName>>
{
	// Black Marsh.
	{ ProvinceName::BlackMarsh, { LocationName::VaultsOfGemin, LocationName::Murkwood } },

	// Elsweyr.
	{ ProvinceName::Elsweyr, { LocationName::TempleOfAgamanus, LocationName::HallsOfColossus } },

	// Hammerfell.
	{ ProvinceName::Hammerfell, { LocationName::Stonekeep, LocationName::FangLair } },

	// High Rock.
	{ ProvinceName::HighRock, { LocationName::MinesOfKhuras, LocationName::CryptOfHearts } },

	// Imperial Province. The dungeons are the starting area, and the palace is the end game.
	{ ProvinceName::ImperialProvince, { LocationName::ImperialDungeons, LocationName::ImperialPalace } },

	// Morrowind.
	{ ProvinceName::Morrowind, { LocationName::BlackGate, LocationName::DagothUr } },

	// Skyrim.
	{ ProvinceName::Skyrim, { LocationName::FortressOfIce, LocationName::Labyrinthian } },

	// Summerset Isle.
	{ ProvinceName::SummersetIsle, { LocationName::TempleOfMadGod, LocationName::CrystalTower } },

	// Valenwood.
	{ ProvinceName::Valenwood, { LocationName::SelenesWeb, LocationName::EldenGrove } }
};

const auto ProvinceDisplayNames = std::map<ProvinceName, std::string>
{
	{ ProvinceName::BlackMarsh, "Black Marsh" },
	{ ProvinceName::Elsweyr, "Elsweyr" },
	{ ProvinceName::Hammerfell, "Hammerfell" },
	{ ProvinceName::HighRock, "High Rock" },
	{ ProvinceName::ImperialProvince, "Imperial Province" },
	{ ProvinceName::Morrowind, "Morrowind" },
	{ ProvinceName::Skyrim, "Skyrim" },
	{ ProvinceName::SummersetIsle, "Summerset Isle" },
	{ ProvinceName::Valenwood, "Valenwood" }
};

const auto ProvinceSingularRaceNames = std::map<ProvinceName, std::string>
{
	{ ProvinceName::BlackMarsh, "Argonian" },
	{ ProvinceName::Elsweyr, "Khajiit" },
	{ ProvinceName::Hammerfell, "Redguard" },
	{ ProvinceName::HighRock, "Breton" },
	{ ProvinceName::ImperialProvince, "Imperial" },
	{ ProvinceName::Morrowind, "Dark Elf" },
	{ ProvinceName::Skyrim, "Nord" },
	{ ProvinceName::SummersetIsle, "High Elf" },
	{ ProvinceName::Valenwood, "Wood Elf" }
};

const auto ProvincePluralRaceNames = std::map<ProvinceName, std::string>
{
	{ ProvinceName::BlackMarsh, "Argonians" },
	{ ProvinceName::Elsweyr, "Khajiit" },
	{ ProvinceName::Hammerfell, "Redguards" },
	{ ProvinceName::HighRock, "Bretons" },
	{ ProvinceName::ImperialProvince, "Imperials" },
	{ ProvinceName::Morrowind, "Dark Elves" },
	{ ProvinceName::Skyrim, "Nords" },
	{ ProvinceName::SummersetIsle, "High Elves" },
	{ ProvinceName::Valenwood, "Wood Elves" }
};

Province::Province(ProvinceName provinceName)
{
	this->provinceName = provinceName;
}

Province::~Province()
{

}

std::vector<ProvinceName> Province::getAllProvinceNames()
{
	auto provinces = std::vector<ProvinceName>();
	for (auto &item : ProvinceCivilizations)
	{
		provinces.push_back(item.first);
	}

	assert(provinces.size() > 0);
	return provinces;
}

const ProvinceName &Province::getProvinceName() const
{
	return this->provinceName;
}

std::string Province::toString() const
{
	auto displayName = ProvinceDisplayNames.at(this->getProvinceName());
	assert(displayName.size() > 0);
	return displayName;
}

std::string Province::getRaceName(bool plural) const
{
	auto raceName = plural ?
		ProvincePluralRaceNames.at(this->getProvinceName()) :
		ProvinceSingularRaceNames.at(this->getProvinceName());
	assert(raceName.size() > 0);
	return raceName;
}

std::vector<LocationName> Province::getCivilizations() const
{
	auto civilizations = ProvinceCivilizations.at(this->getProvinceName());
	assert(civilizations.size() > 0);
	return civilizations;
}

std::vector<LocationName> Province::getMainQuestDungeons() const
{
	auto dungeons = ProvinceMainQuestDungeons.at(this->getProvinceName());
	assert(dungeons.size() > 0);
	return dungeons;
}
