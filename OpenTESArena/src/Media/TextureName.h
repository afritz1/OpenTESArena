#ifndef TEXTURE_NAME_H
#define TEXTURE_NAME_H

// A unique identifier for every non-bulk image file; non-bulk meaning not wall 
// or sprite textures. There are hundreds of bulk textures and using an enum for 
// each would be prohibitive.

// Bulk textures should use some kind of filename indexing method that is loaded
// from a text file, associating each wall or sprite with a set of filenames.

enum class TextureName
{
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
	CharacterCreation, // Starry night with house.
	CharacterEquipment, // Character sheet equipment.
	CharacterStats, // Character sheet attributes, stats, etc..
	CompassFrame, // Compass border with "gargoyle-like" object.
	CompassSlider, // Actual compass headings.
	GameWorldInterface, // Portrait, stat bars, buttons in game world.
	Icon, // Custom window icon for the program.
	IntroTitle, // Arena copyright, etc..
	IntroQuote, // "The best techniques...".
	MainMenu, // Load, New, Exit.
	ParchmentBig, // Fullscreen parchment with scrolls on top and bottom.
	ParchmentPopup, // For text boxes.
	PauseBackground, // Arena logo with sound/music/detail and buttons
	PopUp, // For class list.
	PopUp11, // For items, etc..
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
	BlackMarshMap,
	ElsweyrMap,
	HammerfellMap,
	HighRockMap,
	ImperialProvinceMap,
	MorrowindMap,
	SkyrimMap,
	SummersetIsleMap,
	ValenwoodMap,
	WorldMap,
};

#endif
