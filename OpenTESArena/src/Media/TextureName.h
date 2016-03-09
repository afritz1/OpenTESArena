#ifndef TEXTURE_NAME_H
#define TEXTURE_NAME_H

// A unique identifier for every concrete image file.

// Walls should be combined into the group format for better organization.

// This enum isn't going to work well for thousands of textures. Hmm. What do people
// normally do when they want to have a unique reference for thousands of things?
// I'll probably store these in a text file somewhere anyway at some point.

enum class TextureName
{
	// Interface.
	SwordCursor,
	QuillCursor,

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


};

#endif