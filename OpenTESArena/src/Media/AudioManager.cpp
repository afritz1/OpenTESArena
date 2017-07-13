#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "al.h"
#include "alc.h"

#include "AudioManager.h"

#include "WildMidi.hpp"
#include "../Game/Options.h"
#include "../Utilities/Debug.h"

std::unique_ptr<MidiDevice> MidiDevice::sInstance;

class OpenALStream;

class AudioManagerImpl {
public:
	float mMusicVolume;
	float mSfxVolume;

	/* Currently active song and playback stream. */
	MidiSongPtr mCurrentSong;
	std::unique_ptr<OpenALStream> mSongStream;

	/* A deque of available sources to play sounds and streams with. */
	std::deque<ALuint> mFreeSources;

	AudioManagerImpl();
	~AudioManagerImpl();

	void init(const Options &options);

	void playMusic(const std::string &filename);
	void playSound(const std::string &filename);

	void stopMusic();
	void stopSound();

	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
};


class OpenALStream {
	AudioManagerImpl *mManager;
	MidiSong *mSong;

	/* Background thread and control. */
	std::atomic<bool> mQuit;
	std::thread mThread;

	/* Playback source and buffer queue. */
	static const int sBufferFrames = 16384;
	ALuint mSource;
	std::array<ALuint, 4> mBuffers;
	ALuint mBufferIdx;

	/* Stream format. */
	ALenum mFormat;
	ALuint mSampleRate;
	ALuint mFrameSize;

	/* Read samples from the song and fill the given OpenAL buffer ID (buffer
	 * vector is for temporary storage). Returns true if the buffer was filled.
	 */
	bool fillBuffer(ALuint bufid, std::vector<char> &buffer)
	{
		size_t totalSize = 0;
		while (totalSize < buffer.size())
		{
			size_t toget = (buffer.size() - totalSize) / mFrameSize;
			size_t got = mSong->read(buffer.data() + totalSize, toget);
			if (got < toget)
			{
				/* End of song, rewind to loop. */
				if (!mSong->seek(0))
					break;
			}
			totalSize += got*mFrameSize;
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
			++queued;
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
					--processed;
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
				--processed;
			}
		}
	}

public:
	OpenALStream(AudioManagerImpl *manager, MidiSong *song)
		: mManager(manager), mSong(song), mQuit(false), mSource(0)
		, mBufferIdx(0), mSampleRate(0)
	{
		// Using std::fill for mBuffers since VS2013 doesn't support mBuffers{0}.
		std::fill(mBuffers.begin(), mBuffers.end(), 0);
	}

	~OpenALStream()
	{
		if (mThread.get_id() != std::thread::id())
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

	void play()
	{
		/* If the source is already playing (thread exists and isn't stopped),
		 * don't do anything.
		 */
		if (mThread.get_id() != std::thread::id())
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
		if (mThread.get_id() != std::thread::id())
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
		assert(mSource != 0);
		alSourcef(mSource, AL_GAIN, volume);
	}

	bool init(ALuint source, float volume)
	{
		assert(mSource == 0);

		/* Clear existing errors */
		alGetError();

		alGenBuffers(static_cast<ALsizei>(mBuffers.size()), mBuffers.data());
		if (alGetError() != AL_NO_ERROR)
		{
			std::fill(mBuffers.begin(), mBuffers.end(), 0);
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
		return true;
	}
};

// Audio Manager Impl

AudioManagerImpl::AudioManagerImpl()
	: mMusicVolume(1.0f), mSfxVolume(1.0f)
{

}

AudioManagerImpl::~AudioManagerImpl()
{
	stopMusic();

	MidiDevice::shutdown();

	ALCcontext *context = alcGetCurrentContext();
	if (!context) return;


	for (ALuint source : mFreeSources)
		alDeleteSources(1, &source);
	mFreeSources.clear();

	ALCdevice *device = alcGetContextsDevice(context);
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void AudioManagerImpl::init(const Options &options)
{
	DebugMention("Initializing.");

#ifdef HAVE_WILDMIDI
	WildMidiDevice::init(options.getSoundfont());
#endif

	// Start initializing the OpenAL device.
	ALCdevice *device = alcOpenDevice(nullptr);
	DebugAssert(device != nullptr, "alcOpenDevice");

	// Create an OpenAL context.
	ALCcontext *context = alcCreateContext(device, nullptr);
	DebugAssert(context != nullptr, "alcCreateContext");

	ALCboolean success = alcMakeContextCurrent(context);
	DebugAssert(success == AL_TRUE, "alcMakeContextCurrent");

	double musicVolume = options.getMusicVolume();
	double soundVolume = options.getSoundVolume();
	int maxChannels = options.getSoundChannelCount();

	this->setMusicVolume(musicVolume);
	this->setSoundVolume(soundVolume);

	for (size_t i = 0; i < maxChannels; ++i)
	{
		ALuint source;
		alGenSources(1, &source);
		if (alGetError() != AL_NO_ERROR)
			break;
		mFreeSources.push_back(source);
	}
}

void AudioManagerImpl::playMusic(const std::string &filename)
{
	stopMusic();

	if (!mFreeSources.empty())
	{
		if (MidiDevice::isInited())
			mCurrentSong = MidiDevice::get().open(filename);
		if (!mCurrentSong)
		{
			DebugMention("Failed to play " + filename + ".");
			return;
		}

		mSongStream.reset(new OpenALStream(this, mCurrentSong.get()));
		if (mSongStream->init(mFreeSources.front(), mMusicVolume))
		{
			mFreeSources.pop_front();
			mSongStream->play();
			DebugMention("Playing music " + filename + ".");
		}
		else
		{
			DebugMention("Failed to init song stream.");
		}
	}
}

void AudioManagerImpl::playSound(const std::string &filename)
{
	// To do: sound support.
	static_cast<void>(filename);
}

void AudioManagerImpl::stopMusic()
{
	if (mSongStream)
		mSongStream->stop();
	mSongStream = nullptr;
	mCurrentSong = nullptr;
}

void AudioManagerImpl::stopSound()
{
	// To do: stop all sounds.
}

void AudioManagerImpl::setMusicVolume(double percent)
{
	if (mSongStream)
		mSongStream->setVolume(static_cast<float>(percent));
	mMusicVolume = static_cast<float>(percent);
}

void AudioManagerImpl::setSoundVolume(double percent)
{
	mSfxVolume = static_cast<float>(percent);
}

// Audio Manager

const double AudioManager::MIN_VOLUME = 0.0;
const double AudioManager::MAX_VOLUME = 1.0;

AudioManager::AudioManager()
	: pImpl(new AudioManagerImpl())
{

}

AudioManager::~AudioManager()
{

}

void AudioManager::init(const Options &options)
{
	pImpl->init(options);
}

void AudioManager::playMusic(const std::string &filename)
{
	pImpl->playMusic(filename);
}

void AudioManager::playSound(const std::string &filename)
{
	pImpl->playSound(filename);
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
