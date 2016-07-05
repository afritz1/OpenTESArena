#ifndef PALETTE_NAME_H
#define PALETTE_NAME_H

// A unique identifier for each palette.

// The "built-in" palette name indicates that the loaded IMG should use its
// own palette, and not the active one owned by the texture manager. If the
// build-in enum is used with an IMG that doesn't have a palette, it should
// be an error.

enum class PaletteName
{
	BuiltIn,
	CharSheet,
	Daytime,
	Default,
	Dreary
};

#endif
