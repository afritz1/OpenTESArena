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
	// Cursors.
	ArrowCursors,
	QuillCursor,
	SwordCursor,

	// Equipment overlays.
	FemaleEquipment,
	FemaleHelmet, // Unused in original.
	FemaleNonMagicShirt,
	FemaleMagicShirt,
	FemaleDemoTop, // Used in Arena demo image.
	FemalePants,
	MaleEquipment,
	MaleHelmet, // Unused in original.
	MaleNonMagicShirt,
	MaleMagicShirt,
	MalePants,

	// Interface.
	AcceptReject, // Accept/Reject for bartering.
	AcceptCounterReject, // Accept/Counter/Reject for bartering.
	AddJobStatusCancel, // Blacksmith repair buttons.
	BarterBackground, // Stone-looking background for bartering.
	BonusPointsText, // Texture shown in character stats on level-up.
	Brass, // Tiles for a brass texture somewhere.
	Brass2, // Tiles for a brass texture somewhere.
	CharacterCreation, // Starry night with house.
	CharacterEquipment, // Character sheet equipment. "NEWEQUIP.IMG" switches Drop and Exit.
	CharacterStats, // Character sheet attributes, stats, etc..
	CompassFrame, // Compass border with "gargoyle-like" object.
	CompassSlider, // Actual compass headings.
	GameWorldInterface, // Portrait, stat bars, buttons in game world.
	IntroTitle, // Arena copyright, etc..
	IntroQuote, // "The best techniques...".
	LoadSave, // Slots for loading and saving.
	Logbook, // Logbook background.
	MainMenu, // Load, New, Exit.
	Marble, // Tiles for a marble texture somewhere.
	Marble2, // Tiles for a marble texture somewhere.
	NextPage, // Next page button in character stats.
	NoExit, // Covers "Exit" in race select.
	NoSpell, // Darkened spell icon for non-spellcasters.
	Parchment, // Nine tiles for parchment generation.
	ParchmentBig, // Fullscreen parchment with scrolls on top and bottom.
	PauseBackground, // Arena logo with sound/music/detail and buttons
	PopUp,
	PopUp2, // For character creation classes.
	PopUp11, // For items, etc..
	RaceSelect, // World map with location dots and no exit text.
	Scroll, // Nine tiles, similar to parchment tiles, but with a different palette.
	SpellbookText, // Text that covers up "Equipment" in player's inventory.
	StatusGradients, // Colored gradients behind player portrait.
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

	// Maps (province maps are accessed by ID, not texture name).
	Automap,
	WorldMap, // World map without location dots and with exit text.

	// Map icons.
	CityStateIcon,
	DungeonIcon,
	TownIcon,
	VillageIcon,

	// Map text.
	ProvinceNames,

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
	SpellMakerBackground
};

#endif
