#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <map>
#include <string>
#include <vector>
#include <memory>

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


class AudioManagerImpl;

class AudioManager {
    std::unique_ptr<AudioManagerImpl> pImpl;

public:
	AudioManager();
	~AudioManager();

    void init(MusicFormat musicFormat, SoundFormat soundFormat,
        double musicVolume, double soundVolume, int maxChannels);

	static const double MIN_VOLUME;
	static const double MAX_VOLUME;

	bool musicIsPlaying() const;

	// All music will continue to loop until changed by an outside force.
	void playMusic(MusicName musicName);
	void playSound(SoundName soundName);

	void toggleMusic();
	void stopMusic();
	void stopSound();

	// Percent is [0.0, 1.0].
	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
};

#endif
