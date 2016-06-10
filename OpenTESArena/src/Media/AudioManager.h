#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <map>
#include <string>
#include <vector>

#include "MusicFormat.h"
#include "SoundFormat.h"

// Making the switch from FMOD Ex to OpenAL Soft soon.

// MIDI will come from WildMIDI and/or FluidSynth.
// Ogg will come from libogg (or Ogg FLAC? Or libsndfile?).

// ----------------------------------

// Music is for looping background music (MIDI, MP3, or Ogg). Sound is for short 
// to medium duration sounds and speech (Ogg or WAV).

// The default format for music will be MIDI, and sound will be Ogg.

enum class MusicName;
enum class SoundName;


class AudioManager
{
private:
	static const std::string MUSIC_PATH;
	static const std::string SOUNDS_PATH;

	std::map<std::string, std::uint32_t> objects;
	MusicFormat musicFormat;
	SoundFormat soundFormat;

	void loadMusic(const std::string &filename);
	void loadSound(const std::string &filename);
public:
	AudioManager(MusicFormat musicFormat, SoundFormat soundFormat, 
		double musicVolume, double soundVolume, int maxChannels);
	~AudioManager();

	static const double MIN_VOLUME;
	static const double MAX_VOLUME;

	double getMusicVolume() const;
	double getSoundVolume() const;
	bool musicIsPlaying() const;

	// All music will continue to loop until changed by an outside force.
	void playMusic(const std::string &filename);
	void playMusic(MusicName musicName);
	void playSound(const std::string &filename);
	void playSound(SoundName soundName);

	void toggleMusic();
	void stopMusic();
	void stopSound();

	// Percent is [0.0, 1.0].
	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
};

#endif
