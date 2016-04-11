#ifndef SOUND_FORMAT_H
#define SOUND_FORMAT_H

// A unique identifier for each format of sound file.

// Maybe sound formats won't matter as much as music formats because there won't
// be an option for sound format in the options menu. That would require having
// an Ogg copy and WAV copy of every sound.

enum class SoundFormat
{
	Ogg,
	WAV
};

#endif
