#ifndef SOUND_FORMAT_H
#define SOUND_FORMAT_H

// A unique identifier for each format of sound file.

// I plan on having an option for which sound format to use because using wave 
// format for each of the speech files adds up to several dozen, even hundreds,
// of megabytes! Ogg sounds like a much better format to use as the default in
// the final package.

enum class SoundFormat
{
	Ogg,
	WAV
};

#endif
