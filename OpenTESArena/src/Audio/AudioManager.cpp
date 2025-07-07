#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
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
#include "components/utilities/Span.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"
#include "components/utilities/TextLinesFile.h"
#include "components/vfs/manager.hpp"

namespace
{
	bool ProcessVocRepairLine(const std::string_view text, std::string *outFilename, VocRepairSpan *outSpan)
	{
		constexpr int expectedTokenCount = 4;
		std::string_view tokens[expectedTokenCount];
		if (!StringView::splitExpected<expectedTokenCount>(text, ',', tokens))
		{
			DebugLogErrorFormat("Invalid .VOC repair format, skipping: \"%s\"", text);
			return false;
		}

		*outFilename = tokens[0];

		const std::string_view startIndexView = tokens[1];
		std::from_chars_result result = std::from_chars(startIndexView.data(), startIndexView.data() + startIndexView.size(), outSpan->startIndex);
		if (result.ec != std::errc())
		{
			DebugLogError("Couldn't parse .VOC repair startIndex \"" + std::string(startIndexView) + "\".");
			return false;
		}

		const std::string_view countView = tokens[2];
		result = std::from_chars(countView.data(), countView.data() + countView.size(), outSpan->count);
		if (result.ec != std::errc())
		{
			DebugLogError("Couldn't parse .VOC repair count \"" + std::string(countView) + "\".");
			return false;
		}

		const std::string_view replacementByteView = tokens[3];
		result = std::from_chars(replacementByteView.data(), replacementByteView.data() + replacementByteView.size(), outSpan->replacementSample);
		if (result.ec != std::errc())
		{
			DebugLogError("Couldn't parse .VOC repair replacementByte \"" + std::string(replacementByteView) + "\".");
			return false;
		}

		return true;
	}
}

std::unique_ptr<MidiDevice> MidiDevice::sInstance;

class OpenALStream
{
private:
	std::deque<ALuint> *mFreeSourcesPtr; // Free sources owned by audio manager.
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
	bool fillBuffer(ALuint bufid, Span<char> buffer)
	{
		size_t totalSize = 0;
		while (totalSize < buffer.getCount())
		{
			const size_t framesToGet = (buffer.getCount() - totalSize) / mFrameSize;
			const size_t framesReceived = mSong->read(buffer.begin() + totalSize, framesToGet);

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
		alBufferData(bufid, mFormat, buffer.begin(), static_cast<ALsizei>(buffer.getCount()), mSampleRate);
		return true;
	}

	/* Fill buffers to fill up the source queue. Returns the number of buffers
	 * queued.
	 */
	ALint fillBufferQueue(Span<char> buffer)
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
		Buffer<char> buffer(sBufferFrames * mFrameSize);

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
	OpenALStream(std::deque<ALuint> *freeSources, MidiSong *song)
		: mQuit(false)
	{
		mFreeSourcesPtr = freeSources;
		mSong = song;
		mSource = 0;
		mBuffers.fill(0);
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
			mFreeSourcesPtr->push_front(mSource);
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

AudioListenerState::AudioListenerState(const Double3 &position, const Double3 &forward, const Double3 &up)
	: position(position), forward(forward), up(up) { }

VocRepairSpan::VocRepairSpan()
{
	this->startIndex = -1;
	this->count = 0;
	this->replacementSample = 0;
}

AudioManager::AudioManager()
{
	mMusicVolume = 0.0f;
	mSfxVolume = 0.0f;
	mHasResamplerExtension = false;
	mResampler = -1;
	mIs3D = false;
}

AudioManager::~AudioManager()
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

void AudioManager::init(double musicVolume, double soundVolume, int maxChannels, int resamplingOption,
	bool is3D, const std::string &midiConfig, const std::string &audioDataPath)
{
	DebugLog("Initializing.");

#ifdef HAVE_WILDMIDI
	WildMidiDevice::init(midiConfig);
#endif

	// Initialize OpenAL device and context.
	ALCdevice *device = alcOpenDevice(nullptr);
	if (device == nullptr)
	{
		DebugLogWarning("alcOpenDevice() error 0x" + String::toHexString(alGetError()) + ".");
		return;
	}

	ALCcontext *context = alcCreateContext(device, nullptr);
	if (context == nullptr)
	{
		DebugLogWarning("alcCreateContext() error 0x" + String::toHexString(alGetError()) + ".");
		return;
	}

	const ALCboolean success = alcMakeContextCurrent(context);
	if (success != AL_TRUE)
	{
		DebugLogWarning("alcMakeContextCurrent() error 0x" + String::toHexString(alGetError()) + ".");
		return;
	}

	// Check for sound resampling extension.
	mHasResamplerExtension = alIsExtensionPresent("AL_SOFT_source_resampler") != AL_FALSE;
	mResampler = mHasResamplerExtension ? AudioManager::getResamplingIndex(resamplingOption) : AudioManager::UNSUPPORTED_EXTENSION;

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
			DebugLogWarning("alGenSources() error 0x" + String::toHexString(status) + ".");
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

		mFreeSources.emplace_back(source);
	}

	this->setMusicVolume(musicVolume);
	this->setSoundVolume(soundVolume);
	this->setListenerPosition(Double3::Zero);
	this->setListenerOrientation(Double3::UnitX, Double3::UnitY);

	// Load single-instance sounds file, a new feature with this engine since the one-sound-at-a-time limit
	// no longer exists.
	TextLinesFile singleInstanceSoundsFile;
	const std::string singleInstanceSoundsFilename = "SingleInstanceSounds.txt";
	const std::string singleInstanceSoundsPath = audioDataPath + singleInstanceSoundsFilename;
	if (!singleInstanceSoundsFile.init(singleInstanceSoundsPath.c_str()))
	{
		DebugLogWarningFormat("Missing %s in \"%s\".", singleInstanceSoundsFilename.c_str(), audioDataPath.c_str());
	}

	for (int i = 0; i < singleInstanceSoundsFile.getLineCount(); i++)
	{
		std::string soundFilename = singleInstanceSoundsFile.getLine(i);
		mSingleInstanceSounds.emplace_back(std::move(soundFilename));
	}

	// Load .VOC repair file, a temporary fix for annoying pops until a proper mod is available.
	TextLinesFile vocRepairFile;
	const std::string vocRepairFilename = "VocRepair.txt";
	const std::string vocRepairPath = audioDataPath + vocRepairFilename;
	if (!vocRepairFile.init(vocRepairPath.c_str()))
	{
		DebugLogWarningFormat("Missing %s in \"%s\".", vocRepairFilename.c_str(), audioDataPath.c_str());
	}

	for (int i = 0; i < vocRepairFile.getLineCount(); i++)
	{
		const std::string &entry = vocRepairFile.getLine(i);

		std::string vocFilename;
		VocRepairSpan vocRepairSpan;
		if (ProcessVocRepairLine(entry, &vocFilename, &vocRepairSpan))
		{
			const auto existingIter = std::find_if(this->mVocRepairEntries.begin(), this->mVocRepairEntries.end(),
				[&vocFilename](const VocRepairEntry &entry)
			{
				return entry.filename == vocFilename;
			});

			if (existingIter == this->mVocRepairEntries.end())
			{
				VocRepairEntry newEntry;
				newEntry.filename = vocFilename;
				newEntry.spans.emplace_back(vocRepairSpan);
				this->mVocRepairEntries.emplace_back(std::move(newEntry));
			}
			else
			{
				existingIter->spans.emplace_back(vocRepairSpan);
			}
		}
	}	
}

double AudioManager::getMusicVolume() const
{
	return static_cast<double>(mMusicVolume);
}

double AudioManager::getSoundVolume() const
{
	return static_cast<double>(mSfxVolume);
}

bool AudioManager::hasResamplerExtension() const
{
	return mHasResamplerExtension;
}

bool AudioManager::isPlayingSound(const std::string &filename) const
{
	// Check through used sources' filenames.
	const auto iter = std::find_if(mUsedSources.begin(), mUsedSources.end(),
		[&filename](const std::pair<std::string, ALuint> &pair)
	{
		return pair.first == filename;
	});

	return iter != mUsedSources.end();
}

bool AudioManager::soundExists(const std::string &filename) const
{
	return VFS::Manager::get().open(filename.c_str()) != nullptr;
}

ALint AudioManager::getDefaultResampler()
{
	const ALint defaultResampler = alGetInteger(AL_DEFAULT_RESAMPLER_SOFT);
	return defaultResampler;
}

ALint AudioManager::getResamplingIndex(int resamplingOption)
{
	const ALint resamplerCount = alGetInteger(AL_NUM_RESAMPLERS_SOFT);
	const ALint defaultResampler = AudioManager::getDefaultResampler();

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

bool AudioManager::hasNextMusic() const
{
	return !this->mNextSong.empty();
}

void AudioManager::setListenerPosition(const Double3 &position)
{
	const ALfloat posX = static_cast<ALfloat>(position.x);
	const ALfloat posY = static_cast<ALfloat>(position.y);
	const ALfloat posZ = static_cast<ALfloat>(position.z);
	alListener3f(AL_POSITION, posX, posY, posZ);
}

void AudioManager::setListenerOrientation(const Double3 &forward, const Double3 &up)
{
	DebugAssert(forward.isNormalized());
	DebugAssert(up.isNormalized());

	const ALfloat orientation[] =
	{
		static_cast<ALfloat>(forward.x),
		static_cast<ALfloat>(forward.y),
		static_cast<ALfloat>(forward.z),
		static_cast<ALfloat>(up.x),
		static_cast<ALfloat>(up.y),
		static_cast<ALfloat>(up.z)
	};

	alListenerfv(AL_ORIENTATION, orientation);
}

void AudioManager::playSound(const char *filename, const std::optional<Double3> &position)
{
	// Certain sounds should only have one live instance at a time. This is purely an arbitrary
	// rule to avoid having long sounds overlap each other which would be very annoying or
	// distracting for the player.
	const bool isSingleInstance = std::find(mSingleInstanceSounds.begin(), mSingleInstanceSounds.end(), std::string(filename)) != mSingleInstanceSounds.end();
	const bool allowedToPlay = !isSingleInstance || (isSingleInstance && !this->isPlayingSound(filename));

	if (!mFreeSources.empty() && allowedToPlay)
	{
		auto vocIter = mSoundBuffers.find(filename);
		if (vocIter == mSoundBuffers.end())
		{
			// Load the .VOC file and give its PCM data to a new OpenAL buffer.
			VOCFile voc;
			if (!voc.init(filename))
			{
				DebugCrash("Could not init .VOC file \"" + std::string(filename) + "\".");
			}

			// Clear OpenAL error.
			alGetError();

			ALuint bufferID;
			alGenBuffers(1, &bufferID);

			const ALenum status = alGetError();
			if (status != AL_NO_ERROR)
			{
				DebugLogWarning("alGenBuffers() error 0x" + String::toHexString(status));
			}

			Span<uint8_t> audioData = voc.getAudioData();

			// Find and repair any bad samples we know of. A mod should eventually do this.
			const auto repairIter = std::find_if(this->mVocRepairEntries.begin(), this->mVocRepairEntries.end(),
				[filename](const VocRepairEntry &entry)
			{
				return StringView::equals(entry.filename, filename);
			});

			if (repairIter != this->mVocRepairEntries.end())
			{
				const Span<const VocRepairSpan> repairSpans = repairIter->spans;
				for (const VocRepairSpan span : repairSpans)
				{
					const auto spanBegin = audioData.begin() + span.startIndex;
					const auto spanEnd = spanBegin + span.count;
					DebugAssert(spanEnd <= audioData.end());
					std::fill(spanBegin, spanEnd, span.replacementSample);
				}
			}

			alBufferData(bufferID, AL_FORMAT_MONO8,
				static_cast<const ALvoid*>(audioData.begin()),
				static_cast<ALsizei>(audioData.getCount()),
				static_cast<ALsizei>(voc.getSampleRate()));

			vocIter = mSoundBuffers.emplace(filename, bufferID).first;
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

		alSourcePlay(source);

		mUsedSources.push_front(std::make_pair(filename, source));
		mFreeSources.pop_front();
	}
}

void AudioManager::playMusic(const std::string &filename, bool loop)
{
	if (mCurrentSong == filename)
	{
		return;
	}

	stopMusic();

	if (!mFreeSources.empty())
	{
		if (MidiDevice::isInited())
		{
			mCurrentMidiSong = MidiDevice::get().open(filename);
		}

		if (!mCurrentMidiSong)
		{
			DebugLogWarning("Failed to play music " + filename + ".");
			return;
		}

		mSongStream = std::make_unique<OpenALStream>(&mFreeSources, mCurrentMidiSong.get());
		if (mSongStream->init(mFreeSources.front(), mMusicVolume, loop))
		{
			mFreeSources.pop_front();
			mSongStream->play();
			mCurrentSong = filename;
			DebugLog("Playing music " + filename + ".");
		}
		else
		{
			DebugLogWarning("Failed to init music stream " + filename + ".");
		}
	}
}

void AudioManager::setMusic(const MusicDefinition *musicDef, const MusicDefinition *optMusicDef)
{
	if (optMusicDef != nullptr)
	{
		// Play optional music first and set the main music as the next music.
		const std::string &optFilename = optMusicDef->filename;
		const bool loop = false;
		this->playMusic(optFilename, loop);

		DebugAssert(musicDef != nullptr);
		mNextSong = musicDef->filename;
	}
	else if (musicDef != nullptr)
	{
		// Play main music immediately.
		const std::string &filename = musicDef->filename;
		const bool loop = true;
		this->playMusic(filename, loop);
	}
	else
	{
		// No music to play.
		this->stopMusic();
	}
}

void AudioManager::stopMusic()
{
	if (mSongStream != nullptr)
	{
		mSongStream->stop();
	}

	mCurrentMidiSong = nullptr;
	mSongStream = nullptr;
}

void AudioManager::stopSound()
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
			const ALint defaultResampler = AudioManager::getDefaultResampler();
			alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, defaultResampler);
		}

		mFreeSources.push_front(source);
	}

	mUsedSources.clear();
}

void AudioManager::setMusicVolume(double percent)
{
	mMusicVolume = static_cast<float>(percent);

	if (mSongStream != nullptr)
	{
		mSongStream->setVolume(mMusicVolume);
	}
}

void AudioManager::setSoundVolume(double percent)
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

void AudioManager::setResamplingOption(int resamplingOption)
{
	// Do not call if AL_SOFT_source_resampler is unsupported.
	DebugAssert(mHasResamplerExtension);

	// Determine which resampling index to use.
	mResampler = AudioManager::getResamplingIndex(resamplingOption);

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

void AudioManager::set3D(bool is3D)
{
	// Any future game world sounds will base their playback on this value.
	mIs3D = is3D;
}

void AudioManager::updateSources()
{
	for (size_t i = 0; i < mUsedSources.size(); i++)
	{
		const ALuint source = mUsedSources[i].second;

		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);

		// If a sound source is done, reset it and return the ID to the free sources.
		if (state == AL_STOPPED)
		{
			alSourceRewind(source);
			alSourcei(source, AL_BUFFER, 0);

			if (mHasResamplerExtension)
			{
				const ALint defaultResampler = AudioManager::getDefaultResampler();
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

void AudioManager::updateListener(const AudioListenerState &listenerState)
{
	this->setListenerPosition(listenerState.position);
	this->setListenerOrientation(listenerState.forward, listenerState.up);
}
