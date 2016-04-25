#ifndef TEXTURE_NAME_H
#define TEXTURE_NAME_H

// A unique identifier for every concrete image file.

// Should walls be combined into the group format for better organization?

enum class TextureName
{
	// Interface.
	CharacterCreation, // starry night with house
	CompassFrame,
	Icon, // window icon
	IntroTitle, // Arena copyright, etc.
	IntroQuote, // "The best techniques..."
	MainMenu,
	ParchmentPopup, // for text boxes
	QuillCursor,
	SwordCursor,
	WorldMap, // Tamriel map with helmet
	
	// Fonts.
	FontA, // big
	FontArena,
	FontB,
	FontC, // big
	FontChar,
	FontD,
	FontFour,
	FontS,
	FontTeeny,
};

#endif
