#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <memory>
#include <optional>
#include <string>

#include "../Math/Vector3.h"

// This class manages what sounds and music are played by OpenAL Soft.

class AudioManagerImpl;
class MusicDefinition;
class Options;

class AudioManager
{
public:
	// Contains data for defining the state of an audio listener.
	class ListenerData
	{
	private:
		Double3 position;
		Double3 direction;
	public:
		ListenerData(const Double3 &position, const Double3 &direction);

		const Double3 &getPosition() const;
		const Double3 &getDirection() const;
	};
private:
	std::unique_ptr<AudioManagerImpl> pImpl;
public:
	AudioManager();
	~AudioManager(); // Required for pImpl to stay in .cpp file.

    void init(double musicVolume, double soundVolume, int maxChannels, int resamplingOption,
		bool is3D, const std::string &midiConfig);

	static constexpr double MIN_VOLUME = 0.0;
	static constexpr double MAX_VOLUME = 1.0;

	double getMusicVolume() const;
	double getSoundVolume() const;

	// Returns whether the implementation supports resampling options.
	bool hasResamplerExtension() const;

	// Returns whether the given filename is playing in any sound handle.
	bool isPlayingSound(const std::string &filename) const;

	// Returns whether the given filename references an actual sound.
	bool soundExists(const std::string &filename) const;

	// Plays a sound file. All sounds should play once. If 'position' is empty then the sound
	// is played globally.
	void playSound(const std::string &filename,
		const std::optional<Double3> &position = std::nullopt);

	// Sets the music to the given music definition, with an optional music to play first as a
	// lead-in to the actual music. If no music definition is given, the current music is stopped.
	void setMusic(const MusicDefinition *musicDef, const MusicDefinition *optMusicDef = nullptr);

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

	// Sets whether game world audio should be played in 2D (global) or 3D (with a listener).
	// The 2D option is provided for parity with the original engine.
	void set3D(bool is3D);

	// Adds a sound filename to the single-instance sounds list. These sounds can only have one
	// live instance at a time.
	void addSingleInstanceSound(std::string &&filename);
	void clearSingleInstanceSounds();

	// Updates any state not handled by a background thread, such as resetting
	// the sources of finished sounds, and updating listener values (if any).
	void update(double dt, const ListenerData *listenerData);
};

#endif
