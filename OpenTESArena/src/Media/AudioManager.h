#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <memory>
#include <string>
#include <optional>

#include "../Math/Vector3.h"

// This class manages what sounds and music are played by OpenAL Soft.

class AudioManagerImpl;
class Options;

class AudioManager
{
private:
	std::unique_ptr<AudioManagerImpl> pImpl;
public:
	AudioManager();
	~AudioManager(); // Required for pImpl to stay in .cpp file.

    void init(double musicVolume, double soundVolume, int maxChannels,
		int resamplingOption, const std::string &midiConfig);

	static const double MIN_VOLUME;
	static const double MAX_VOLUME;

	double getMusicVolume() const;
	double getSoundVolume() const;

	// Returns whether the implementation supports resampling options.
	bool hasResamplerExtension() const;

	// Plays a music file. All music should loop until changed.
	void playMusic(const std::string &filename);

	// Plays a sound file. All sounds should play once. If 'position' is empty then the sound
	// is played globally.
	void playSound(const std::string &filename,
		const std::optional<Double3> &position = std::nullopt);

	// Stops the music.
	void stopMusic();

	// Stops all sounds.
	void stopSound();

	// Sets the music volume. Percent must be between 0.0 and 1.0.
	void setMusicVolume(double percent);

	// Sets the sound volume. Percent must be between 0.0 and 1.0.
	void setSoundVolume(double percent);

	// Sets the resampling option used by all sources. Note that the given index does not
	// necessarily map to a specific index in the resampling list. Causes an error if
	// resampling options are not supported.
	void setResamplingOption(int resamplingOption);

	// Sets the global listener's position/orientation (in world coordinates).
	void setListenerPosition(const Double3 &position);
	void setListenerOrientation(const Double3 &direction);

	// Updates any state not handled by a background thread, such as resetting
	// the sources of finished sounds.
	void update();
};

#endif
