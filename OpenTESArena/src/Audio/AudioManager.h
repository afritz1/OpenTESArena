#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "al.h"

#include "Midi.h"
#include "../Math/Vector3.h"

#include "components/utilities/KeyValuePool.h"

class OpenALStream;
class Options;

struct MusicDefinition;

using SoundInstanceID = int;

struct SoundInstance
{
	std::string filename;
	bool isOneShot;
	bool is3D;

	ALuint source; // Taken from free sources when playing.

	SoundInstance();

	void init(const std::string &filename, bool isOneShot, bool is3D);
};

// Contains data for defining the state of an audio listener.
struct AudioListenerState
{
	Double3 position;
	Double3 forward;
	Double3 up;

	AudioListenerState(const Double3 &position, const Double3 &forward, const Double3 &up);
};

struct VocRepairSpan
{
	int startIndex;
	int count;
	uint8_t replacementSample;

	VocRepairSpan();
};

// For spot-fixing bad samples in .VOC files, eventually to be done by a mod.
struct VocRepairEntry
{
	std::string filename;
	std::vector<VocRepairSpan> spans;
};

// Manages sounds and music played by OpenAL Soft.
class AudioManager
{
private:
	static constexpr ALint UNSUPPORTED_EXTENSION = -1;

	float mMusicVolume;
	float mSfxVolume;
	bool mHasResamplerExtension; // Whether AL_SOFT_source_resampler is supported.

	ALint mResampler;
	bool mIs3D;
	std::string mCurrentSong, mNextSong;

	// Sounds which are allowed only one active instance to avoid overcrowding. This new functionality is important
	// because the original game can only play one sound at a time, so it doesn't have this problem.
	std::vector<std::string> mSingleInstanceSounds;

	// The engine can overwrite .VOC file samples with revised data to fix annoying pops.
	std::vector<VocRepairEntry> mVocRepairEntries;

	// Currently active song and playback stream.
	MidiSongPtr mCurrentMidiSong;
	std::unique_ptr<OpenALStream> mSongStream;

	// Loaded sound buffers from .VOC files.
	std::unordered_map<std::string, ALuint> mSoundBuffers;

	// Available sources to play sounds and streams with.
	std::deque<ALuint> mFreeSources;

	KeyValuePool<SoundInstanceID, SoundInstance> soundInstancesPool;

	// Whether there is a music queued after the current one.
	bool hasNextMusic() const;

	void setListenerPosition(const Double3 &position);
	void setListenerOrientation(const Double3 &forward, const Double3 &up);

	void playMusic(const std::string &filename, bool loop);

	void resetSource(ALuint source);
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

	// Returns whether the given filename references an actual sound.
	bool soundExists(const std::string &filename) const;

	// Returns whether the given filename is playing in any sound instance.
	bool anyPlayingSounds(const std::string &filename) const;

	// Sets the music to the given music definition, with an optional music to play first as a
	// lead-in to the actual music. If no music definition is given, the current music is stopped.
	void setMusic(const MusicDefinition *musicDef, const MusicDefinition *optMusicDef = nullptr);

	void stopMusic();
	void stopSounds();

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
	void updateListener(const AudioListenerState &listenerState);

	SoundInstanceID allocateSound(const std::string &filename, bool isOneShot, bool is3D);
	SoundInstanceID allocateSound(const std::string &filename);
	void freeSound(SoundInstanceID instID);

	double getSoundTotalSeconds(const std::string &filename) const;
	double getSoundTotalSeconds(SoundInstanceID instID) const;
	double getSoundCurrentSeconds(SoundInstanceID instID) const;
	bool isSoundPlaying(SoundInstanceID instID) const;
	void setSoundPosition(SoundInstanceID instID, const Double3 &position);
	void playSound(SoundInstanceID instID);
	void pauseSound(SoundInstanceID instID);
	void stopSound(SoundInstanceID instID);

	// Manages creating and destroying the sound instance. This does nothing if not enough sound channels are available.
	void playSoundOneShot(const std::string &filename, const Double3 &position);
	void playSoundOneShot(const std::string &filename);
};
