#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <memory>

// OpenAL Soft will be used with WildMIDI and/or FluidSynth.

// Music is for looping background music. Sound is for short to medium duration 
// sounds and speech.

class AudioManagerImpl;
class Options;

enum class MusicName;
enum class SoundName;

class AudioManager
{
	std::unique_ptr<AudioManagerImpl> pImpl;
public:
	AudioManager();
	~AudioManager();

    void init(Options *options);

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
