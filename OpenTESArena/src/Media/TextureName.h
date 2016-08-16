#ifndef TEXTURE_NAME_H
#define TEXTURE_NAME_H

// A unique identifier for every non-bulk image file; non-bulk meaning not wall 
// or sprite textures. There are hundreds of bulk textures and using an enum for 
// each would be prohibitive.

// Bulk textures should use some kind of filename indexing method that is loaded
// from a text file, associating each wall or sprite with a set of filenames.

// We don't want to have texture names for single-image sprites. Just include
// those with the bulk textures, too.

enum class TextureName
{
	// Character backgrounds (without clothes).
	FemaleArgonianBackground,
	FemaleBretonBackground,
	FemaleDarkElfBackground,
	FemaleHighElfBackground,
	FemaleKhajiitBackground,
	FemaleNordBackground,
	FemaleRedguardBackground,
	FemaleWoodElfBackground,
	MaleArgonianBackground,
	MaleBretonBackground,
	MaleDarkElfBackground,
	MaleHighElfBackground,
	MaleKhajiitBackground,
	MaleNordBackground,
	MaleRedguardBackground,
	MaleWoodElfBackground,

	// Cursors.
	QuillCursor,
	SwordCursor,

	// Equipment overlays.
	FemaleNonMagicShirt,
	FemaleMagicShirt,
	FemalePants,
	MaleNonMagicShirt,
	MaleMagicShirt,
	MalePants,

	// Fonts.
	FontA,
	FontArena,
	FontB,
	FontC,
	FontChar,
	FontD,
	FontFour,
	FontS,
	FontTeeny,

	// Interface.
	AcceptReject, // Accept/Reject for bartering.
	AcceptCounterReject, // Accept/Counter/Reject for bartering.
	AddJobStatusCancel, // Blacksmith repair buttons.
	BarterBackground, // Stone-looking background for bartering.
	BonusPointsText, // Texture shown in character stats on level-up.
	CharacterCreation, // Starry night with house.
	CharacterEquipment, // Character sheet equipment. "NEWEQUIP.IMG" switches Drop and Exit.
	CharacterStats, // Character sheet attributes, stats, etc..
	CompassFrame, // Compass border with "gargoyle-like" object.
	CompassSlider, // Actual compass headings.
	GameWorldInterface, // Portrait, stat bars, buttons in game world.
	Icon, // Custom window icon for the program.
	IntroTitle, // Arena copyright, etc..
	IntroQuote, // "The best techniques...".
	LoadSave, // Slots for loading and saving.
	Logbook, // Logbook background.
	MainMenu, // Load, New, Exit.
	NextPage, // Next page button in character stats.
	ParchmentBig, // Fullscreen parchment with scrolls on top and bottom.
	ParchmentPopup, // For text boxes.
	PauseBackground, // Arena logo with sound/music/detail and buttons
	PopUp,
	PopUp2, // For character creation classes.
	PopUp11, // For items, etc..
	RaceSelect, // World map with location dots and no exit text.
	SpellbookText, // Text that covers up "Equipment" in player's inventory.
	UpDown, // Scroll arrows.
	YesNoCancel, // Yes/No/Cancel texture for bartering.

	// Main quest dungeon splash screens.
	CryptOfHeartsSplash,
	CrystalTowerSplash,
	DagothUrSplash,
	EldenGroveSplash,
	FangLairSplash,
	HallsOfColossusSplash,
	LabyrinthianSplash,
	MurkwoodSplash,

	// Maps.
	Automap,
	BlackMarshMap,
	ElsweyrMap,
	HammerfellMap,
	HighRockMap,
	ImperialProvinceMap,
	MorrowindMap,
	SkyrimMap,
	SummersetIsleMap,
	ValenwoodMap,
	WorldMap, // World map without location dots and with exit text.

	// Map icons.
	CityStateIcon,
	DungeonIcon,
	TownIcon,
	VillageIcon,

	// Spellbook and spellmaker.
	BuySpellBackground,
	Form1,
	Form2,
	Form3,
	Form4,
	Form4A,
	Form5,
	Form6,
	Form6A,
	Form7,
	Form8,
	Form9,
	Form10,
	Form11,
	Form12,
	Form13,
	Form14,
	Form15,
	SpellMakerBackground,
};

#endif
