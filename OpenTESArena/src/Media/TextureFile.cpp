#include <cassert>
#include <map>

#include "TextureFile.h"

#include "TextureName.h"
#include "TextureSequenceName.h"

// The filename of each TextureName, by type.
const std::map<TextureName, std::string> TextureFilenames =
{
	// Character backgrounds (without clothes).
	{ TextureName::FemaleArgonianBackground, "CHRBKF07.IMG" },
	{ TextureName::FemaleBretonBackground, "CHRBKF00.IMG" },
	{ TextureName::FemaleDarkElfBackground, "CHRBKF03.IMG" },
	{ TextureName::FemaleHighElfBackground, "CHRBKF04.IMG" },
	{ TextureName::FemaleKhajiitBackground, "CHRBKF06.IMG" },
	{ TextureName::FemaleNordBackground, "CHRBKF02.IMG" },
	{ TextureName::FemaleRedguardBackground, "CHRBKF01.IMG" },
	{ TextureName::FemaleWoodElfBackground, "CHRBKF05.IMG" },
	{ TextureName::MaleArgonianBackground, "CHARBK07.IMG" },
	{ TextureName::MaleBretonBackground, "CHARBK00.IMG" },
	{ TextureName::MaleDarkElfBackground, "CHARBK03.IMG" },
	{ TextureName::MaleHighElfBackground, "CHARBK04.IMG" },
	{ TextureName::MaleKhajiitBackground, "CHARBK06.IMG" },
	{ TextureName::MaleNordBackground, "CHARBK02.IMG" },
	{ TextureName::MaleRedguardBackground, "CHARBK01.IMG" },
	{ TextureName::MaleWoodElfBackground, "CHARBK05.IMG" },

	// Cursors.
	{ TextureName::QuillCursor, "POINTER.IMG" },
	{ TextureName::SwordCursor, "ARENARW.IMG" },

	// Equipment overlays. "TOP.IMG" was used in a demo image, I think.
	// 0ARGHELM.IMG and 1ARGHELM.IMG seem to be helmets.
	{ TextureName::FemaleNonMagicShirt, "FSSHIRT.IMG" },
	{ TextureName::FemaleMagicShirt, "FRSHIRT.IMG" },
	{ TextureName::FemalePants, "FPANTS.IMG" },
	{ TextureName::MaleNonMagicShirt, "MSSHIRT.IMG" },
	{ TextureName::MaleMagicShirt, "MRSHIRT.IMG" },
	{ TextureName::MalePants, "MPANTS.IMG" },

	// Interface.
	{ TextureName::AcceptReject, "ACCPREJT.IMG" },
	{ TextureName::AcceptCounterReject, "NEGOTBUT.IMG" },
	{ TextureName::AddJobStatusCancel, "NEWOLD.IMG" },
	{ TextureName::BarterBackground, "MENUSCRN.IMG" },
	{ TextureName::BonusPointsText, "BONUS.IMG" },
	{ TextureName::CharacterCreation, "STARTGAM.MNU" },
	{ TextureName::CharacterEquipment, "EQUIP.IMG" },
	{ TextureName::CharacterStats, "CHARSTAT.IMG" },
	{ TextureName::CompassFrame, "COMPASS.IMG" },
	{ TextureName::CompassSlider, "SLIDER.IMG" },
	{ TextureName::GameWorldInterface, "P1.IMG" }, // The portrait gradients might be in STATUS.CIF.
	{ TextureName::Icon, "interface/icon" },
	{ TextureName::IntroTitle, "TITLE.IMG" },
	{ TextureName::IntroQuote, "QUOTE.IMG" },
	{ TextureName::LoadSave, "LOADSAVE.IMG" },
	{ TextureName::Logbook, "LOGBOOK.IMG" },
	{ TextureName::MainMenu, "MENU.IMG" },
	{ TextureName::NextPage, "PAGE2.IMG" },
	{ TextureName::ParchmentBig, "PARCH.IMG" },
	{ TextureName::ParchmentPopup, "interface/parchment/parchment_popup" },
	{ TextureName::PauseBackground, "OP.IMG" },
	{ TextureName::PopUp, "POPUP.IMG" },
	{ TextureName::PopUp2, "POPUP2.IMG" },
	{ TextureName::PopUp11, "POPUP11.IMG" },
	{ TextureName::RaceSelect, "TAMRIEL.IMG" },
	{ TextureName::SpellbookText, "SPELLBK.IMG" },
	{ TextureName::UpDown, "UPDOWN.IMG" },
	{ TextureName::YesNoCancel, "YESNO.IMG" },

	// Main quest dungeon splash screens.
	{ TextureName::CryptOfHeartsSplash, "CRYPT.IMG" },
	{ TextureName::CrystalTowerSplash, "TOWER.IMG" },
	{ TextureName::DagothUrSplash, "DAGOTHUR.IMG" },
	{ TextureName::EldenGroveSplash, "GROVE.IMG" },
	{ TextureName::FangLairSplash, "FANGLAIR.IMG" },
	{ TextureName::HallsOfColossusSplash, "COLOSSUS.IMG" },
	{ TextureName::LabyrinthianSplash, "LABRINTH.IMG" },
	{ TextureName::MurkwoodSplash, "MIRKWOOD.IMG" },

	// Maps.
	{ TextureName::Automap, "AUTOMAP.IMG" },
	{ TextureName::BlackMarshMap, "BLAKMRSH.IMG" },
	{ TextureName::ElsweyrMap, "ELSWEYR.IMG" },
	{ TextureName::HammerfellMap, "HAMERFEL.IMG" },
	{ TextureName::HighRockMap, "HIGHROCK.IMG" },
	{ TextureName::ImperialProvinceMap, "IMPERIAL.IMG" },
	{ TextureName::MorrowindMap, "MOROWIND.IMG" },
	{ TextureName::SkyrimMap, "SKYRIM.IMG" },
	{ TextureName::SummersetIsleMap, "SUMERSET.IMG" },
	{ TextureName::ValenwoodMap, "VALNWOOD.IMG" },
	{ TextureName::WorldMap, "TAMRIEL.MNU" },

	// Map icons.
	{ TextureName::CityStateIcon, "CITY.IMG" },
	{ TextureName::DungeonIcon, "DUNGEON.IMG" },
	{ TextureName::TownIcon, "TOWN.IMG" },
	{ TextureName::VillageIcon, "VILLAGE.IMG" },

	// Spellbook and spellmaker.
	{ TextureName::BuySpellBackground, "BUYSPELL.IMG" },
	{ TextureName::Form1, "FORM1.IMG" },
	{ TextureName::Form2, "FORM2.IMG" },
	{ TextureName::Form3, "FORM3.IMG" },
	{ TextureName::Form4, "FORM4.IMG" },
	{ TextureName::Form4A, "FORM4A.IMG" },
	{ TextureName::Form5, "FORM5.IMG" },
	{ TextureName::Form6, "FORM6.IMG" },
	{ TextureName::Form6A, "FORM6A.IMG" },
	{ TextureName::Form7, "FORM7.IMG" },
	{ TextureName::Form8, "FORM8.IMG" },
	{ TextureName::Form9, "FORM9.IMG" },
	{ TextureName::Form10, "FORM10.IMG" },
	{ TextureName::Form11, "FORM11.IMG" },
	{ TextureName::Form12, "FORM12.IMG" },
	{ TextureName::Form13, "FORM13.IMG" },
	{ TextureName::Form14, "FORM14.IMG" },
	{ TextureName::Form15, "FORM15.IMG" },
	{ TextureName::SpellMakerBackground, "SPELLMKR.IMG" },
};

// The filename prefix of each TextureSequenceName (with sub-folders). When looking
// for files in the sequence, "-#" will be appended to the name for the number of
// the file, with more numbers added as needed.
const std::map<TextureSequenceName, std::string> TextureSequenceFilenames =
{
	// Interface.
	{ TextureSequenceName::IntroBook, "interface/intro/intro" },
	{ TextureSequenceName::OpeningScroll, "interface/scroll/scroll" },
	{ TextureSequenceName::Silmane, "interface/silmane/silmane" },
};

// The number of images in each texture sequence. I think these numbers are necessary
// without doing some directory counting, and that sounds like a bad idea, with all
// the possible mix-ups with text files lying around and duplicates and such.
// These could be parsed in from a text file, but it's not necessary.
const std::map<TextureSequenceName, int> TextureSequenceCounts =
{
	// Interface.
	{ TextureSequenceName::IntroBook, 75 },
	{ TextureSequenceName::OpeningScroll, 49 },
	{ TextureSequenceName::Silmane, 19 }
};

std::vector<TextureSequenceName> TextureFile::getSequenceNames()
{
	std::vector<TextureSequenceName> names;
	for (const auto &pair : TextureSequenceFilenames)
	{
		names.push_back(pair.first);
	}

	return names;
}

const std::string &TextureFile::fromName(TextureName textureName)
{
	const auto &filename = TextureFilenames.at(textureName);
	return filename;
}

std::vector<std::string> TextureFile::fromName(TextureSequenceName sequenceName)
{
	const auto &filename = TextureSequenceFilenames.at(sequenceName);
	const int count = TextureSequenceCounts.at(sequenceName);

	std::vector<std::string> filenames;

	// Generate the list of filenames to load from file later.
	for (int i = 0; i < count; ++i)
	{
		auto countString = "-" + std::to_string(i + 1);
		filenames.push_back(filename + countString);
	}

	return filenames;
}
