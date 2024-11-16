#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "al.h"

#include "Midi.h"

#include "../Math/Vector3.h"

class OpenALStream;
class Options;

struct MusicDefinition;

// Contains data for defining the state of an audio listener.
struct AudioListenerData
{
	Double3 position;
	Double3 direction;

	AudioListenerData(const Double3 &position, const Double3 &direction);
};

// Manages what sounds and music are played by OpenAL Soft.
class AudioManager
{
private:
	static constexpr ALint UNSUPPORTED_EXTENSION = -1;

	float mMusicVolume;
	float mSfxVolume;
	bool mHasResamplerExtension; // Whether AL_SOFT_source_resampler is supported.

	ALint mResampler;
	bool mIs3D;
	std::string mNextSong;

	// Sounds which are allowed only one active instance at a time, otherwise they would
	// sound a bit obnoxious. This functionality is added here because the original game
	// can only play one sound at a time, so it doesn't have this problem.
	std::vector<std::string> mSingleInstanceSounds;

	// Currently active song and playback stream.
	MidiSongPtr mCurrentSong;
	std::unique_ptr<OpenALStream> mSongStream;

	// Loaded sound buffers from .VOC files.
	std::unordered_map<std::string, ALuint> mSoundBuffers;

	// A deque of available sources to play sounds and streams with.
	std::deque<ALuint> mFreeSources;

	// A deque of currently used sources for sounds (the music source is owned
	// by OpenALStream). The string is the filename and the integer is the ID.
	// The filename is required for some sounds that can only have one instance
	// active at a time.
	std::deque<std::pair<std::string, ALuint>> mUsedSources;

	// Use this when resetting sound sources back to their default resampling. This uses
	// whatever setting is the default within OpenAL.
	static ALint getDefaultResampler();

	// Gets the resampling index to use, given some resampling option. The two values are not
	// necessarily identical (depending on the resampling implementation). Causes an error
	// if the resampling extension is unsupported.
	static ALint getResamplingIndex(int value);

	// Whether there is a music queued after the current one.
	bool hasNextMusic() const;

	void setListenerPosition(const Double3 &position);
	void setListenerOrientation(const Double3 &direction);

	void playMusic(const std::string &filename, bool loop);
public:
	AudioManager();
	~AudioManager();

    void init(double musicVolume, double soundVolume, int maxChannels, int resamplingOption,
		bool is3D, const std::string &midiConfig, const std::string &audioDataPath);

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

	// Updates state not handled by a background thread, such as resetting finished sources.
	void updateSources();

	// Updates the position of the 3D listener.
	void updateListener(const AudioListenerData &listenerData);
};

#endif
