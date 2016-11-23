#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <memory>

// This class manages what sounds and music are played.

// OpenAL Soft is used with WildMIDI for music.

class AudioManagerImpl;
class Options;

class AudioManager
{
	std::unique_ptr<AudioManagerImpl> pImpl;
public:
	AudioManager();
	~AudioManager();

    void init(const Options &options);

	static const double MIN_VOLUME;
	static const double MAX_VOLUME;

	// Plays a music file. All music should loop until changed.
	void playMusic(const std::string &filename);

	// Plays a sound file. All sounds should play once.
	void playSound(const std::string &filename);

	// Stops the music.
	void stopMusic();

	// Stops all sounds.
	void stopSound();

	// Sets the music volume. Percent must be between 0.0 and 1.0.
	void setMusicVolume(double percent);

	// Sets the sound volume. Percent must be between 0.0 and 1.0.
	void setSoundVolume(double percent);
};

#endif
