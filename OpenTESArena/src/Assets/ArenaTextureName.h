#ifndef ARENA_TEXTURE_NAME_H
#define ARENA_TEXTURE_NAME_H

#include <string>

namespace ArenaTextureName
{
	// Cursors.
	const std::string ArrowCursors = "ARROWS.CIF";
	const std::string QuillCursor = "POINTER.IMG";
	const std::string SwordCursor = "ARENARW.IMG";

	// Equipment overlays.
	const std::string FemaleEquipment = "1EQUIP.CIF";
	const std::string FemaleHelmet = "1ARGHELM.IMG";
	const std::string FemaleNonMagicShirt = "FSSHIRT.IMG";
	const std::string FemaleMagicShirt = "FRSHIRT.IMG";
	const std::string FemaleDemoTop = "TOP.IMG";
	const std::string FemalePants = "FPANTS.IMG";
	const std::string MaleEquipment = "0EQUIP.CIF";
	const std::string MaleHelmet = "0ARGHELM.IMG";
	const std::string MaleNonMagicShirt = "MSSHIRT.IMG";
	const std::string MaleMagicShirt = "MRSHIRT.IMG";
	const std::string MalePants = "MPANTS.IMG";

	// Interface.
	const std::string AcceptReject = "ACCPREJT.IMG";
	const std::string AcceptCounterReject = "NEGOTBUT.IMG";
	const std::string AddJobStatusCancel = "NEWOLD.IMG";
	const std::string BarterBackground = "MENUSCRN.IMG";
	const std::string BonusPointsText = "BONUS.IMG";
	const std::string Brass = "BRASS.CIF";
	const std::string Brass2 = "BRASS2.CIF";
	const std::string CharacterCreation = "STARTGAM.MNU";
	const std::string CharacterEquipment = "EQUIP.IMG";
	const std::string CharacterStats = "CHARSTAT.IMG";
	const std::string CompassFrame = "COMPASS.IMG";
	const std::string CompassSlider = "SLIDER.IMG";
	const std::string FastTravel = "HORSE.DFA";
	const std::string GameWorldInterface = "P1.IMG";
	const std::string IntroTitle = "TITLE.IMG";
	const std::string IntroQuote = "QUOTE.IMG";
	const std::string LoadSave = "LOADSAVE.IMG";
	const std::string Logbook = "LOGBOOK.IMG";
	const std::string MainMenu = "MENU.IMG";
	const std::string Marble = "MARBLE.CIF";
	const std::string Marble2 = "MARBLE2.CIF";
	const std::string NextPage = "PAGE2.IMG";
	const std::string NoExit = "NOEXIT.IMG";
	const std::string NoSpell = "NOSPELL.IMG";
	const std::string Parchment = "PARCH.CIF";
	const std::string ParchmentBig = "PARCH.IMG";
	const std::string PauseBackground = "OP.IMG";
	const std::string PopUp = "POPUP.IMG";
	const std::string PopUp2 = "POPUP2.IMG";
	const std::string PopUp8 = "POPUP8.IMG";
	const std::string PopUp11 = "POPUP11.IMG";
	const std::string RaceSelect = "TAMRIEL.MNU";
	const std::string Scroll = "SCROLL.CIF";
	const std::string SpellbookText = "SPELLBK.IMG";
	const std::string StatusGradients = "STATUS.CIF";
	const std::string UpDown = "UPDOWN.IMG";
	const std::string YesNoCancel = "YESNO.IMG";

	// Maps (province maps are stored in a separate container).
	const std::string Automap = "AUTOMAP.IMG";
	const std::string WorldMap = "TAMRIEL.MNU";

	// Map icons.
	const std::string CityStateIcon = "CITY.IMG";
	const std::string DungeonIcon = "DUNGEON.IMG";
	const std::string TownIcon = "TOWN.IMG";
	const std::string VillageIcon = "VILLAGE.IMG";
	const std::string StaffDungeonIcons = "STAFDUNG.CIF";
	const std::string MapIconOutlines = "MAPOUT.CIF";
	const std::string MapIconOutlinesBlinking = "MAPBLINK.CIF";

	// Map text.
	const std::string ProvinceNames = "OUTPROV.CIF";

	// Spellbook and spellmaker.
	const std::string BuySpellBackground = "BUYSPELL.IMG";
	const std::string Form1 = "FORM1.IMG";
	const std::string Form2 = "FORM2.IMG";
	const std::string Form3 = "FORM3.IMG";
	const std::string Form4 = "FORM4.IMG";
	const std::string Form4A = "FORM4A.IMG";
	const std::string Form5 = "FORM5.IMG";
	const std::string Form6 = "FORM6.IMG";
	const std::string Form6A = "FORM6A.IMG";
	const std::string Form7 = "FORM7.IMG";
	const std::string Form8 = "FORM8.IMG";
	const std::string Form9 = "FORM9.IMG";
	const std::string Form10 = "FORM10.IMG";
	const std::string Form11 = "FORM11.IMG";
	const std::string Form12 = "FORM12.IMG";
	const std::string Form13 = "FORM13.IMG";
	const std::string Form14 = "FORM14.IMG";
	const std::string Form15 = "FORM15.IMG";
	const std::string SpellMakerBackground = "SPELLMKR.IMG";
}

// Cinematics in the CD version:
// - AFLC2 (AFLC2.FLC)
// - IntroBook (INTRO.FLC)
// - Mage (MAGE.FLC, duplicate of MAGE.CEL)
// - NewJagarDeath (NUJAGDTH.FLC)
// - NewKing (NUKING.FLC)
// - Rogue (ROGUE.FLC, duplicate of ROGUE.CEL)
// - Staff (STAFF.FLC)
// - Walk (WALK.FLC)
// - Warrior (WARRIOR.FLC, duplicate of WARRIOR.CEL)

namespace ArenaTextureSequenceName
{
	const std::string AFLC2 = "AFLC2.FLC";
	const std::string ChaosVision = "CHAOSVSN.FLC";
	const std::string End01 = "END01.FLC";
	const std::string End02 = "END02.FLC";
	const std::string IntroBook = "INTRO.FLC";
	const std::string Jagar = "JAGAR.FLC";
	const std::string JagarDeath = "JAGARDTH.FLC";
	const std::string JagarShield = "JAGRSHLD.FLC";
	const std::string King = "KING.FLC";
	const std::string Mage = "MAGE.CEL";
	const std::string Morph = "MORPH.FLC";
	const std::string NewJagarDeath = "NUJAGDTH.FLC";
	const std::string NewKing = "NUKING.FLC";
	const std::string OpeningScroll = "SCROLL.FLC";
	const std::string Rogue = "ROGUE.CEL";
	const std::string Silmane = "VISION.FLC";
	const std::string Staff = "STAFF.FLC";
	const std::string Walk = "WALK.FLC";
	const std::string Warhaft = "WARHAFT.FLC";
	const std::string Warrior = "WARRIOR.CEL";
}

#endif
