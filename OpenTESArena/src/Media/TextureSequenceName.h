#ifndef TEXTURE_SEQUENCE_NAME_H
#define TEXTURE_SEQUENCE_NAME_H

// A unique identifier for each FLC/CEL video, such as cinematics. This exists 
// to cut down on the number of TextureNames that would otherwise be used 
// for those cinematics, and it is not intended for walls and sprites.

// For all intents and purposes, a texture sequence is a video.

// Each texture sequence will have an associated count hidden in the TextureFile 
// class that determines how many images are in the sequence, as well as part of 
// the filenames of each.

enum class TextureSequenceName
{
	IntroBook, // Zoom in on book
	OpeningScroll, // Opening scroll for both intro and new game story
	Silmane, // Ria Silmane looping cinematic
};

#endif
