#ifndef MUSIC_FORMAT_H
#define MUSIC_FORMAT_H

// A unique identifier for each format of music. This should be visible to the
// MusicManager and any "options" settings.
enum class MusicFormat
{
	MIDI,
	Mp3,
	Ogg
};

#endif