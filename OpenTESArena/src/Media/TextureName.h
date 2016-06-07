#ifndef TEXTURE_NAME_H
#define TEXTURE_NAME_H

// A unique identifier for every non-bulk image file; non-bulk meaning not wall 
// or sprite textures. There are hundreds of bulk textures and using an enum for 
// each would be prohibitive, and also wouldn't allow for new walls or sprites.

// Bulk textures should use some kind of filename indexing method that is loaded
// from a text file, associating each wall or sprite with a set of filenames.

enum class TextureName
{
	// Interface.
	CharacterCreation, // Starry night with house
	CompassFrame,
	CompassSlider,
	Icon, // Window icon for the program
	IntroTitle, // Arena copyright, etc.
	IntroQuote, // "The best techniques..."
	MainMenu, // Load, New, Exit
	ParchmentPopup, // For text boxes
	QuillCursor,
	SwordCursor,
	UpDown, // Scroll arrows
	WorldMap, // Tamriel map with helmet
	
	// Fonts. I don't think any of these are duplicates.
	FontA,
	FontArena,
	FontB,
	FontC,
	FontChar,
	FontD,
	FontFour,
	FontS,
	FontTeeny,
};

#endif
