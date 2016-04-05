#ifndef TEXTURE_SEQUENCE_NAME_H
#define TEXTURE_SEQUENCE_NAME_H

// A unique identifier for each sequence of textures, such as cinematics. This 
// exists to cut down on the number of TextureNames used for bulk texture sets.

// For all intents and purposes, a texture sequence is a video.

// Each texture sequence will have an associated count hidden in the
// TextureManager class that determines how many images are in the sequence,
// as well as part of the filenames of each.

enum class TextureSequenceName
{
	// Interface
	IntroBook, // zoom in on book
	IntroStory, // "for centuries..."
	OpeningScroll, // opening scroll for both intro and new game story
	NewGameStory, // Uriel story
	Silmane, // Ria Silmane looping cinematic
};

#endif
