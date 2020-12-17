#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "al.h"
#include "alc.h"

#include "alext.h" // Using local copy (+ "efx.h") to guarantee existence on system.
#include "AudioManager.h"
#include "MusicDefinition.h"
#include "WildMidi.h"
#include "../Assets/VOCFile.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"

#include "components/debug/Debug.h"
#include "components/vfs/manager.hpp"

std::unique_ptr<MidiDevice> MidiDevice::sInstance;

class OpenALStream;

class AudioManagerImpl
{
private:
	static constexpr ALint UNSUPPORTED_EXTENSION = -1;

	ALint mResampler;
	bool mIs3D;	
	std::string mNextSong;

	// Sounds which are allowed only one active instance at a time, otherwise they would
	// sound a bit obnoxious. This functionality is added here because the original game
	// can only play one sound at a time, so it doesn't have this problem.
	std::vector<std::string> mSingleInstanceSounds;

	// Use this when resetting sound sources back to their default resampling. This uses
	// whatever setting is the default within OpenAL.
	static ALint getDefaultResampler();

	// Gets the resampling index to use, given some resampling option. The two values are not
	// necessarily identical (depending on the resampling implementation). Causes an error
	// if the resampling extension is unsupported.
	static ALint getResamplingIndex(int value);
public:
	float mMusicVolume;
	float mSfxVolume;
	bool mHasResamplerExtension; // Whether AL_SOFT_source_resampler is supported.

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

	AudioManagerImpl();
	~AudioManagerImpl();

	void init(double musicVolume, double soundVolume, int maxChannels, int resamplingOption,
		bool is3D, const std::string &midiConfig);

	bool hasNextMusic() const;

	// Returns whether the given sound is currently playing.
	bool soundIsPlaying(const std::string &filename) const;

	bool soundExists(const std::string &filename) const;

	void playMusic(const std::string &filename, bool loop);
	void playSound(const std::string &filename, const std::optional<Double3> &position);

	void stopMusic();
	void stopSound();

	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
	void setResamplingOption(int value);
	void set3D(bool is3D);
	void addSingleInstanceSound(std::string &&filename);
	void clearSingleInstanceSounds();
	void setListenerPosition(const Double3 &position);
	void setListenerOrientation(const Double3 &direction);
	void setNextMusic(std::string &&filename);

	void update(double dt, const AudioManager::ListenerData *listenerData);
};

AudioManager::ListenerData::ListenerData(const Double3 &position, const Double3 &direction)
	: position(position), direction(direction) { }

const Double3 &AudioManager::ListenerData::getPosition() const
{
	return this->position;
}

const Double3 &AudioManager::ListenerData::getDirection() const
{
	return this->direction;
}

class OpenALStream
{
private:
	AudioManagerImpl *mManager;
	MidiSong *mSong;
	bool mLoop;

	/* Background thread and control. */
	std::atomic<bool> mQuit;
	std::thread mThread;

	/* Playback source and buffer queue. */
	static constexpr int sBufferFrames = 16384;
	ALuint mSource;
	std::array<ALuint, 4> mBuffers;
	ALuint mBufferIdx;

	/* Stream format. */
	ALenum mFormat;
	ALuint mSampleRate;
	ALuint mFrameSize;

	bool threadIsValid() const
	{
		return mThread.get_id() != std::thread::id();
	}

	/* Read samples from the song and fill the given OpenAL buffer ID (buffer
	 * vector is for temporary storage). Returns true if the buffer was filled.
	 */
	bool fillBuffer(ALuint bufid, std::vector<char> &buffer)
	{
		size_t totalSize = 0;
		while (totalSize < buffer.size())
		{
			const size_t framesToGet = (buffer.size() - totalSize) / mFrameSize;
			const size_t framesReceived = mSong->read(buffer.data() + totalSize, framesToGet);

			bool shouldBreak = false;
			if (framesReceived < framesToGet)
			{
				if (mLoop)
				{
					/* End of song, rewind to loop. */
					const size_t beginOffset = 0;
					const bool success = mSong->seek(beginOffset);
					if (!success)
					{
						break;
					}
				}
				else
				{
					/* Don't receive more song data. */
					shouldBreak = true;
				}
			}

			totalSize += framesReceived * mFrameSize;

			if (shouldBreak)
			{
				break;
			}
		}

		if (totalSize == 0)
			return false;

		std::fill(buffer.begin() + totalSize, buffer.end(), 0);
		alBufferData(bufid, mFormat, buffer.data(),
			static_cast<ALsizei>(buffer.size()), mSampleRate);
		return true;
	}

	/* Fill buffers to fill up the source queue. Returns the number of buffers
	 * queued.
	 */
	ALint fillBufferQueue(std::vector<char> &buffer)
	{
		ALint queued;
		alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
		while (queued < mBuffers.size())
		{
			ALuint bufid = mBuffers[mBufferIdx];
			if (!fillBuffer(bufid, buffer))
				break;
			mBufferIdx = (mBufferIdx + 1) % mBuffers.size();
			alSourceQueueBuffers(mSource, 1, &bufid);
			queued++;
		}
		return queued;
	}

	/* A method run in a backround thread, to keep filling the queue with new
	 * audio over time.
	 */
	void backgroundProc()
	{
		/* Temporary storage to read samples into, before passing to OpenAL.
		 * Kept here to avoid reallocating it during playback.
		 */
		std::vector<char> buffer(sBufferFrames * mFrameSize);

		while (!mQuit.load())
		{
			/* First, make sure the buffer queue is filled. */
			fillBufferQueue(buffer);

			ALint state;
			alGetSourcei(mSource, AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING && state != AL_PAUSED)
			{
				/* If the source is not playing or paused, it either underrun
				 * or hasn't started at all yet. So remove any buffers that
				 * have been played (will be 0 when first starting).
				 */
				ALint processed;
				alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
				while (processed > 0)
				{
					ALuint bufid;
					alSourceUnqueueBuffers(mSource, 1, &bufid);
					processed--;
				}

				/* Make sure the buffer queue is still filled, in case another
				 * buffer had finished before checking the state and after the
				 * last fill. If the queue is empty, playback is over.
				 */
				if (fillBufferQueue(buffer) == 0)
				{
					mQuit.store(true);
					return;
				}

				/* Now start the sound source. */
				alSourcePlay(mSource);
			}

			ALint processed;
			alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
			if (processed == 0)
			{
				/* Wait until a buffer in the queue has been processed. */
				do {
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					if (mQuit.load()) break;
					alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
				} while (processed == 0);
			}
			/* Remove processed buffers, then restart the loop to keep the
			 * queue filled.
			 */
			while (processed > 0)
			{
				ALuint bufid;
				alSourceUnqueueBuffers(mSource, 1, &bufid);
				processed--;
			}
		}
	}

public:
	OpenALStream(AudioManagerImpl *manager, MidiSong *song)
		: mBuffers{ 0 }, mQuit(false)
	{
		mManager = manager;
		mSong = song;
		mSource = 0;
		mBufferIdx = 0;
		mSampleRate = 0;
		mLoop = false;
	}

	~OpenALStream()
	{
		if (threadIsValid())
		{
			/* Tell the thread to quit and wait for it to stop. */
			mQuit.store(true);
			mThread.join();
		}
		if (mSource)
		{
			/* Stop the source, remove the buffers, then put it back so it can
			 * be used again.
			 */
			alSourceRewind(mSource);
			alSourcei(mSource, AL_BUFFER, 0);
			mManager->mFreeSources.push_front(mSource);
		}
		/* Delete the buffers used for the queue. */
		alDeleteBuffers(static_cast<ALsizei>(mBuffers.size()), mBuffers.data());
	}

	bool isPlaying() const
	{
		return threadIsValid() && !mQuit.load();
	}

	void play()
	{
		/* If the source is already playing (thread exists and isn't stopped),
		 * don't do anything.
		 */
		if (threadIsValid())
		{
			if (!mQuit.load())
				return;
			mThread.join();
		}

		/* Reset the source and clear any buffers that may be on it. */
		alSourceRewind(mSource);
		alSourcei(mSource, AL_BUFFER, 0);
		mBufferIdx = 0;
		mQuit.store(false);

		/* Start the background thread processing. */
		mThread = std::thread(std::mem_fn(&OpenALStream::backgroundProc), this);
	}

	void stop()
	{
		if (threadIsValid())
		{
			mQuit.store(true);
			mThread.join();
		}

		alSourceRewind(mSource);
		alSourcei(mSource, AL_BUFFER, 0);
		mBufferIdx = 0;
	}

	void setVolume(float volume)
	{
		DebugAssert(mSource != 0);
		alSourcef(mSource, AL_GAIN, volume);
	}

	bool init(ALuint source, float volume, bool loop)
	{
		DebugAssert(mSource == 0);

		/* Clear existing errors */
		alGetError();

		alGenBuffers(static_cast<ALsizei>(mBuffers.size()), mBuffers.data());
		if (alGetError() != AL_NO_ERROR)
		{
			mBuffers.fill(0);
			return false;
		}

		/* Set the default properties for localized playback */
		alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
		alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSourcef(source, AL_GAIN, volume);
		alSourcef(source, AL_PITCH, 1.0f);
		alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);
		alSourcef(source, AL_SEC_OFFSET, 0.0f);
		alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcei(source, AL_LOOPING, AL_FALSE);

		if (alGetError() != AL_NO_ERROR)
			return false;

		int srate;
		mSong->getFormat(&srate);

		/* Currently hard-coded to 16-bit stereo. */
		mFormat = AL_FORMAT_STEREO16;
		mFrameSize = 4;
		mSampleRate = srate;

		mSource = source;
		mLoop = loop;
		return true;
	}
};

// Audio Manager Impl

AudioManagerImpl::AudioManagerImpl()
{
	mMusicVolume = 1.0f;
	mSfxVolume = 1.0f;
	mHasResamplerExtension = false;
}

AudioManagerImpl::~AudioManagerImpl()
{
	this->stopMusic();
	this->stopSound();

	MidiDevice::shutdown();

	ALCcontext *context = alcGetCurrentContext();
	if (context == nullptr)
	{
		return;
	}

	for (ALuint source : mFreeSources)
	{
		alDeleteSources(1, &source);
	}

	mFreeSources.clear();

	for (auto &pair : mSoundBuffers)
	{
		ALuint buffer = pair.second;
		alDeleteBuffers(1, &buffer);
	}

	mSoundBuffers.clear();

	ALCdevice *device = alcGetContextsDevice(context);
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

ALint AudioManagerImpl::getDefaultResampler()
{
	const ALint defaultResampler = alGetInteger(AL_DEFAULT_RESAMPLER_SOFT);
	return defaultResampler;
}

ALint AudioManagerImpl::getResamplingIndex(int resamplingOption)
{
	const ALint resamplerCount = alGetInteger(AL_NUM_RESAMPLERS_SOFT);
	const ALint defaultResampler = AudioManagerImpl::getDefaultResampler();

	if (resamplingOption == 0)
	{
		// Default.
		return defaultResampler;
	}
	else if (resamplingOption == 1)
	{
		// Fastest.
		return 0;
	}
	else if (resamplingOption == 2)
	{
		// Medium.
		return std::min(defaultResampler + 1, resamplerCount - 1);
	}
	else if (resamplingOption == 3)
	{
		// Best.
		return resamplerCount - 1;
	}
	else
	{
		DebugUnhandledReturnMsg(ALint, std::to_string(resamplingOption));
	}
}

void AudioManagerImpl::init(double musicVolume, double soundVolume, int maxChannels,
	int resamplingOption, bool is3D, const std::string &midiConfig)
{
	DebugLog("Initializing.");

#ifdef HAVE_WILDMIDI
	WildMidiDevice::init(midiConfig);
#endif

	// Initialize the OpenAL device and context.
	ALCdevice *device = alcOpenDevice(nullptr);
	if (device == nullptr)
	{
		DebugLogWarning("alcOpenDevice() error " + std::to_string(alGetError()) + ".");
	}

	ALCcontext *context = alcCreateContext(device, nullptr);
	if (context == nullptr)
	{
		DebugLogWarning("alcCreateContext() error " + std::to_string(alGetError()) + ".");
	}

	const ALCboolean success = alcMakeContextCurrent(context);
	if (success != AL_TRUE)
	{
		DebugLogWarning("alcMakeContextCurrent() error " + std::to_string(alGetError()) + ".");
	}

	// Check for sound resampling extension.
	mHasResamplerExtension = alIsExtensionPresent("AL_SOFT_source_resampler") != AL_FALSE;
	mResampler = mHasResamplerExtension ?
		AudioManagerImpl::getResamplingIndex(resamplingOption) :
		AudioManagerImpl::UNSUPPORTED_EXTENSION;

	// Set whether the audio manager should play in 2D or 3D mode.
	mIs3D = is3D;

	// Generate the sound sources.
	for (int i = 0; i < maxChannels; i++)
	{
		ALuint source;
		alGenSources(1, &source);

		const ALenum status = alGetError();
		if (status != AL_NO_ERROR)
		{
			DebugLogWarning("alGenSources() error " + std::to_string(status) + ".");
		}

		alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
		alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		alSourcef(source, AL_GAIN, mSfxVolume);
		alSourcef(source, AL_PITCH, 1.0f);
		alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);

		// Set resampling if the extension is supported.
		if (mHasResamplerExtension)
		{
			alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, mResampler);
		}

		mFreeSources.push_back(source);
	}

	this->clearSingleInstanceSounds();
	this->setMusicVolume(musicVolume);
	this->setSoundVolume(soundVolume);
	this->setListenerPosition(Double3::Zero);
	this->setListenerOrientation(Double3::UnitX);
}

bool AudioManagerImpl::hasNextMusic() const
{
	return !this->mNextSong.empty();
}

bool AudioManagerImpl::soundIsPlaying(const std::string &filename) const
{
	// Check through used sources' filenames.
	const auto iter = std::find_if(mUsedSources.begin(), mUsedSources.end(),
		[&filename](const std::pair<std::string, ALuint> &pair)
	{
		return pair.first == filename;
	});

	return iter != mUsedSources.end();
}

bool AudioManagerImpl::soundExists(const std::string &filename) const
{
	return VFS::Manager::get().open(filename.c_str()) != nullptr;
}

void AudioManagerImpl::playMusic(const std::string &filename, bool loop)
{
	stopMusic();

	if (!mFreeSources.empty())
	{
		if (MidiDevice::isInited())
			mCurrentSong = MidiDevice::get().open(filename);
		if (!mCurrentSong)
		{
			DebugLogWarning("Failed to play " + filename + ".");
			return;
		}

		mSongStream = std::make_unique<OpenALStream>(this, mCurrentSong.get());
		if (mSongStream->init(mFreeSources.front(), mMusicVolume, loop))
		{
			mFreeSources.pop_front();
			mSongStream->play();
			DebugLog("Playing music " + filename + ".");
		}
		else
		{
			DebugLogWarning("Failed to init " + filename + " stream.");
		}
	}
}

void AudioManagerImpl::playSound(const std::string &filename,
	const std::optional<Double3> &position)
{
	// Certain sounds should only have one live instance at a time. This is purely an arbitrary
	// rule to avoid having long sounds overlap each other which would be very annoying and/or
	// distracting for the player.
	const bool isSingleInstance = std::find(mSingleInstanceSounds.begin(),
		mSingleInstanceSounds.end(), filename) != mSingleInstanceSounds.end();
	const bool allowedToPlay = !isSingleInstance ||
		(isSingleInstance && !this->soundIsPlaying(filename));

	if (!mFreeSources.empty() && allowedToPlay)
	{
		auto vocIter = mSoundBuffers.find(filename);

		if (vocIter == mSoundBuffers.end())
		{
			// Load the .VOC file and give its PCM data to a new OpenAL buffer.
			VOCFile voc;
			if (!voc.init(filename.c_str()))
			{
				DebugCrash("Could not init .VOC file \"" + filename + "\".");
			}

			// Clear OpenAL error.
			alGetError();

			ALuint bufferID;
			alGenBuffers(1, &bufferID);

			const ALenum status = alGetError();
			if (status != AL_NO_ERROR)
			{
				DebugLogWarning("alGenBuffers() error " + std::to_string(status) + ".");
			}

			const std::vector<uint8_t> &audioData = voc.getAudioData();

			alBufferData(bufferID, AL_FORMAT_MONO8,
				static_cast<const ALvoid*>(audioData.data()),
				static_cast<ALsizei>(audioData.size()),
				static_cast<ALsizei>(voc.getSampleRate()));

			vocIter = mSoundBuffers.insert(std::make_pair(filename, bufferID)).first;
		}

		// Set up the sound source.
		const ALuint source = mFreeSources.front();
		alSourcei(source, AL_BUFFER, vocIter->second);

		// Play the sound in 3D if it has a position and we are set to 3D mode.
		// Otherwise, play it in 2D centered on the listener.
		if (position.has_value() && mIs3D)
		{
			alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
			const Double3 &positionValue = *position;
			const ALfloat posX = static_cast<ALfloat>(positionValue.x);
			const ALfloat posY = static_cast<ALfloat>(positionValue.y);
			const ALfloat posZ = static_cast<ALfloat>(positionValue.z);
			alSource3f(source, AL_POSITION, posX, posY, posZ);
		}
		else
		{
			alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
			alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		}

		// Set resampling if the extension is supported.
		if (mHasResamplerExtension)
		{
			alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, mResampler);
		}

		// Play the sound.
		alSourcePlay(source);

		mUsedSources.push_front(std::make_pair(filename, source));
		mFreeSources.pop_front();
	}
}

void AudioManagerImpl::stopMusic()
{
	if (mSongStream != nullptr)
	{
		mSongStream->stop();
	}

	mSongStream = nullptr;
	mCurrentSong = nullptr;
}

void AudioManagerImpl::stopSound()
{
	// Reset all used sources and return them to the free sources.
	for (const auto &pair : mUsedSources)
	{
		const ALuint source = pair.second;
		alSourceStop(source);
		alSourceRewind(source);
		alSourcei(source, AL_BUFFER, 0);

		if (mHasResamplerExtension)
		{
			const ALint defaultResampler = AudioManagerImpl::getDefaultResampler();
			alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, defaultResampler);
		}

		mFreeSources.push_front(source);
	}

	mUsedSources.clear();
}

void AudioManagerImpl::setMusicVolume(double percent)
{
	mMusicVolume = static_cast<float>(percent);

	if (mSongStream != nullptr)
	{
		mSongStream->setVolume(mMusicVolume);
	}
}

void AudioManagerImpl::setSoundVolume(double percent)
{
	mSfxVolume = static_cast<float>(percent);

	// Set volumes of free and used sound channels.
	for (const ALuint source : mFreeSources)
	{
		alSourcef(source, AL_GAIN, mSfxVolume);
	}

	for (const auto &pair : mUsedSources)
	{
		const ALuint source = pair.second;
		alSourcef(source, AL_GAIN, mSfxVolume);
	}
}

void AudioManagerImpl::setResamplingOption(int resamplingOption)
{
	// Do not call if AL_SOFT_source_resampler is unsupported.
	DebugAssert(mHasResamplerExtension);

	// Determine which resampling index to use.
	mResampler = AudioManagerImpl::getResamplingIndex(resamplingOption);

	// Set resampling options for free and used sources.
	for (const ALuint source : mFreeSources)
	{
		alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, mResampler);
	}

	for (const auto &pair : mUsedSources)
	{
		const ALuint source = pair.second;
		alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, mResampler);
	}
}

void AudioManagerImpl::set3D(bool is3D)
{
	// Any future game world sounds will base their playback on this value.
	mIs3D = is3D;
}

void AudioManagerImpl::addSingleInstanceSound(std::string &&filename)
{
	mSingleInstanceSounds.emplace_back(std::move(filename));
}

void AudioManagerImpl::clearSingleInstanceSounds()
{
	mSingleInstanceSounds.clear();
}

void AudioManagerImpl::setListenerPosition(const Double3 &position)
{
	const ALfloat posX = static_cast<ALfloat>(position.x);
	const ALfloat posY = static_cast<ALfloat>(position.y);
	const ALfloat posZ = static_cast<ALfloat>(position.z);
	alListener3f(AL_POSITION, posX, posY, posZ);
}

void AudioManagerImpl::setListenerOrientation(const Double3 &direction)
{
	DebugAssert(direction.isNormalized());
	const Double4 right = Matrix4d::yRotation(Constants::HalfPi) *
		Double4(direction.x, direction.y, direction.z, 1.0);
	const Double3 up = Double3(right.x, right.y, right.z).cross(direction).normalized();

	const std::array<ALfloat, 6> orientation =
	{
		static_cast<ALfloat>(direction.x),
		static_cast<ALfloat>(direction.y),
		static_cast<ALfloat>(direction.z),
		static_cast<ALfloat>(up.x),
		static_cast<ALfloat>(up.y),
		static_cast<ALfloat>(up.z)
	};

	alListenerfv(AL_ORIENTATION, orientation.data());
}

void AudioManagerImpl::setNextMusic(std::string &&filename)
{
	mNextSong = std::move(filename);
}

void AudioManagerImpl::update(double dt, const AudioManager::ListenerData *listenerData)
{
	// Update listener values if there is a listener currently active.
	if (listenerData != nullptr)
	{
		this->setListenerPosition(listenerData->getPosition());
		this->setListenerOrientation(listenerData->getDirection());
	}

	// If a sound source is done, reset it and return the ID to the free sources.
	for (size_t i = 0; i < mUsedSources.size(); i++)
	{
		const ALuint source = mUsedSources[i].second;

		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);

		if (state == AL_STOPPED)
		{
			alSourceRewind(source);
			alSourcei(source, AL_BUFFER, 0);

			if (mHasResamplerExtension)
			{
				const ALint defaultResampler = AudioManagerImpl::getDefaultResampler();
				alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, defaultResampler);
			}

			mFreeSources.push_front(source);
			mUsedSources.erase(mUsedSources.begin() + i);
		}
	}

	// Check if another music is staged and should start when the current one is done.
	if (this->hasNextMusic())
	{
		const bool canChangeToNextMusic = (mSongStream == nullptr) || !mSongStream->isPlaying();
		if (canChangeToNextMusic)
		{
			// Assume that the next music always loops.
			const bool loop = true;
			this->playMusic(mNextSong, loop);
			mNextSong.clear();
		}
	}
}

// Audio Manager

AudioManager::AudioManager()
	: pImpl(std::make_unique<AudioManagerImpl>()) { }

AudioManager::~AudioManager()
{

}

void AudioManager::init(double musicVolume, double soundVolume, int maxChannels,
	int resamplingOption, bool is3D, const std::string &midiConfig)
{
	pImpl->init(musicVolume, soundVolume, maxChannels, resamplingOption, is3D, midiConfig);
}

double AudioManager::getMusicVolume() const
{
	return static_cast<double>(pImpl->mMusicVolume);
}

double AudioManager::getSoundVolume() const
{
	return static_cast<double>(pImpl->mSfxVolume);
}

bool AudioManager::hasResamplerExtension() const
{
	return pImpl->mHasResamplerExtension;
}

bool AudioManager::isPlayingSound(const std::string &filename) const
{
	return pImpl->soundIsPlaying(filename);
}

bool AudioManager::soundExists(const std::string &filename) const
{
	return pImpl->soundExists(filename);
}

void AudioManager::playSound(const std::string &filename, const std::optional<Double3> &position)
{
	pImpl->playSound(filename, position);
}

void AudioManager::setMusic(const MusicDefinition *musicDef, const MusicDefinition *optMusicDef)
{
	if (optMusicDef != nullptr)
	{
		// Play optional music first and set the main music as the next music.
		const std::string &optFilename = optMusicDef->getFilename();
		const bool loop = false;
		pImpl->playMusic(optFilename, loop);

		DebugAssert(musicDef != nullptr);
		const std::string &nextFilename = musicDef->getFilename();
		pImpl->setNextMusic(std::string(nextFilename));
	}
	else if (musicDef != nullptr)
	{
		// Play main music immediately.
		const std::string &filename = musicDef->getFilename();
		const bool loop = true;
		pImpl->playMusic(filename, loop);
	}
	else
	{
		// No music to play.
		this->stopMusic();
	}
}

void AudioManager::stopMusic()
{
	pImpl->stopMusic();
}

void AudioManager::stopSound()
{
	pImpl->stopSound();
}

void AudioManager::setMusicVolume(double percent)
{
	pImpl->setMusicVolume(percent);
}

void AudioManager::setSoundVolume(double percent)
{
	pImpl->setSoundVolume(percent);
}

void AudioManager::setResamplingOption(int resamplingOption)
{
	pImpl->setResamplingOption(resamplingOption);
}

void AudioManager::set3D(bool is3D)
{
	pImpl->set3D(is3D);
}

void AudioManager::addSingleInstanceSound(std::string &&filename)
{
	pImpl->addSingleInstanceSound(std::move(filename));
}

void AudioManager::clearSingleInstanceSounds()
{
	pImpl->clearSingleInstanceSounds();
}

void AudioManager::update(double dt, const ListenerData *listenerData)
{
	pImpl->update(dt, listenerData);
}
