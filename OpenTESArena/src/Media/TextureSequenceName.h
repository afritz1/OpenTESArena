#ifndef TEXTURE_SEQUENCE_NAME_H
#define TEXTURE_SEQUENCE_NAME_H

// A unique identifier for each sequence of textures, such as cinematics. This 
// exists to cut down on the number of TextureNames that would otherwise be used 
// for those cinematics, and it is not intended for walls and sprites.

// For all intents and purposes, a texture sequence is a video.

// Each texture sequence will have an associated count hidden in the TextureFile 
// class that determines how many images are in the sequence, as well as part of 
// the filenames of each.

enum class TextureSequenceName
{
	// Interface
	IntroBook, // Zoom in on book
	IntroStory, // "For centuries..."
	OpeningScroll, // Opening scroll for both intro and new game story
	NewGameStory, // Uriel story (original is INTRO01.IMG, INTRO02.IMG, ...)
	Silmane, // Ria Silmane looping cinematic
};

#endif
