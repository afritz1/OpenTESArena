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
	constexpr ALuint INVALID_SOURCE = 0;

	ALint GetDefaultAudioResampler()
	{
		return alGetInteger(AL_DEFAULT_RESAMPLER_SOFT);
	}

	ALint GetAudioResamplingIndex(int resamplingOption)
	{
		const ALint resamplerCount = alGetInteger(AL_NUM_RESAMPLERS_SOFT);
		const ALint defaultResampler = GetDefaultAudioResampler();

		if (resamplingOption == 0)
		{
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

SoundInstance::SoundInstance()
{
	this->isOneShot = false;
	this->is3D = false;
	this->source = INVALID_SOURCE;
}

void SoundInstance::init(const std::string &filename, bool isOneShot, bool is3D)
{
	DebugAssert(this->source == INVALID_SOURCE);
	this->filename = filename;
	this->isOneShot = isOneShot;
	this->is3D = is3D;
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
		mFormat = 0;
		mSampleRate = 0;
		mFrameSize = 0;
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
	this->stopSounds();
	this->soundInstancesPool.clear();

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

	mHasResamplerExtension = alIsExtensionPresent("AL_SOFT_source_resampler") != AL_FALSE;
	mResampler = mHasResamplerExtension ? GetAudioResamplingIndex(resamplingOption) : AudioManager::UNSUPPORTED_EXTENSION;
	mIs3D = is3D;

	// Generate sound sources.
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

	// Load single-instance sounds file, a new feature with this engine since the one-sound-at-a-time limit no longer exists.
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
			const auto existingIter = std::find_if(mVocRepairEntries.begin(), mVocRepairEntries.end(),
				[&vocFilename](const VocRepairEntry &entry)
			{
				return entry.filename == vocFilename;
			});

			if (existingIter == mVocRepairEntries.end())
			{
				VocRepairEntry newEntry;
				newEntry.filename = vocFilename;
				newEntry.spans.emplace_back(vocRepairSpan);
				mVocRepairEntries.emplace_back(std::move(newEntry));
			}
			else
			{
				existingIter->spans.emplace_back(vocRepairSpan);
			}
		}
	}

	// Load all sounds into memory.
	const std::vector<std::string> soundFilenames = VFS::Manager::get().list("*.voc");
	const std::vector<std::string> voiceFilenames = VFS::Manager::get().list("SPEECH/*.voc");
	std::vector<std::string> totalSoundFilenames;
	totalSoundFilenames.reserve(soundFilenames.size() + voiceFilenames.size());
	std::copy(soundFilenames.begin(), soundFilenames.end(), std::back_inserter(totalSoundFilenames));
	std::copy(voiceFilenames.begin(), voiceFilenames.end(), std::back_inserter(totalSoundFilenames));

	for (const std::string &soundFilename : totalSoundFilenames)
	{
		VOCFile voc;
		if (!voc.init(soundFilename.c_str()))
		{
			DebugLogErrorFormat("Couldn't init .VOC file \"%s\".", soundFilename.c_str());
			continue;
		}

		// Clear OpenAL error.
		alGetError();

		ALuint bufferID;
		alGenBuffers(1, &bufferID);

		const ALenum status = alGetError();
		if (status != AL_NO_ERROR)
		{
			DebugLogWarningFormat("alGenBuffers() error 0x%x.", status);
		}

		Span<uint8_t> pcmSamples = voc.getAudioData();

		// Find and repair any pre-defined bad samples. A mod should eventually do this.
		const auto repairIter = std::find_if(mVocRepairEntries.begin(), mVocRepairEntries.end(),
			[&soundFilename](const VocRepairEntry &entry)
		{
			return StringView::equals(entry.filename, soundFilename);
		});

		if (repairIter != mVocRepairEntries.end())
		{
			const Span<const VocRepairSpan> repairSpans = repairIter->spans;
			for (const VocRepairSpan span : repairSpans)
			{
				uint8_t *spanBegin = pcmSamples.begin() + span.startIndex;
				uint8_t *spanEnd = spanBegin + span.count;
				DebugAssert(spanEnd <= pcmSamples.end());
				std::fill(spanBegin, spanEnd, span.replacementSample);
			}
		}

		alBufferData(bufferID, AL_FORMAT_MONO8, static_cast<const ALvoid*>(pcmSamples.begin()), static_cast<ALsizei>(pcmSamples.getCount()), static_cast<ALsizei>(voc.getSampleRate()));
		mSoundBuffers.emplace(soundFilename, bufferID);
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

bool AudioManager::soundExists(const std::string &filename) const
{
	return VFS::Manager::get().open(filename.c_str()) != nullptr;
}

bool AudioManager::anyPlayingSounds(const std::string &filename) const
{
	for (const SoundInstanceID soundInstID : this->soundInstancesPool.keys)
	{
		const SoundInstance &soundInst = this->soundInstancesPool.get(soundInstID);
		if (soundInst.filename != filename)
		{
			continue;
		}

		if (this->isSoundPlaying(soundInstID))
		{
			return true;
		}
	}

	return false;
}

int AudioManager::getTotalPlayingSoundCount() const
{
	int count = 0;

	for (const SoundInstance &soundInst : this->soundInstancesPool.values)
	{
		if (alIsSource(soundInst.source) != AL_TRUE)
		{
			continue;
		}

		ALint sourceState;
		alGetSourcei(soundInst.source, AL_SOURCE_STATE, &sourceState);
		if (sourceState != AL_PLAYING)
		{
			continue;
		}

		count++;
	}

	return count;
}

bool AudioManager::hasNextMusic() const
{
	return !mNextSong.empty();
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

void AudioManager::resetSource(ALuint source)
{
	alSourceRewind(source);
	alSourcei(source, AL_BUFFER, 0);

	if (mHasResamplerExtension)
	{
		const ALint defaultResampler = GetDefaultAudioResampler();
		alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, defaultResampler);
	}
}

void AudioManager::playMusic(const std::string &filename, bool loop)
{
	if (mCurrentSong == filename)
	{
		return;
	}

	this->stopMusic();

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

void AudioManager::stopSounds()
{
	for (SoundInstance &soundInst : this->soundInstancesPool.values)
	{
		if (alIsSource(soundInst.source) == AL_TRUE)
		{
			this->resetSource(soundInst.source);
			mFreeSources.push_front(soundInst.source);
			soundInst.source = INVALID_SOURCE;
		}
	}
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

	for (const ALuint source : mFreeSources)
	{
		alSourcef(source, AL_GAIN, mSfxVolume);
	}

	for (const SoundInstance &soundInst : this->soundInstancesPool.values)
	{
		if (alIsSource(soundInst.source) == AL_TRUE)
		{
			alSourcef(soundInst.source, AL_GAIN, mSfxVolume);
		}
	}
}

void AudioManager::setResamplingOption(int resamplingOption)
{
	DebugAssert(mHasResamplerExtension);
	mResampler = GetAudioResamplingIndex(resamplingOption);

	for (const ALuint source : mFreeSources)
	{
		alSourcei(source, AL_SOURCE_RESAMPLER_SOFT, mResampler);
	}

	for (const SoundInstance &soundInst : this->soundInstancesPool.values)
	{
		if (alIsSource(soundInst.source) == AL_TRUE)
		{
			alSourcei(soundInst.source, AL_SOURCE_RESAMPLER_SOFT, mResampler);
		}
	}
}

void AudioManager::set3D(bool is3D)
{
	// Any future game world sounds will base their playback on this value.
	mIs3D = is3D;
}

void AudioManager::updateSources()
{
	// Recycle sources of finished sound instances, destroying the instance if they are one-shot.
	std::vector<SoundInstanceID> soundInstsToDestroy;
	for (const SoundInstanceID soundInstID : this->soundInstancesPool.keys)
	{
		SoundInstance &soundInst = this->soundInstancesPool.get(soundInstID);
		if (alIsSource(soundInst.source) != AL_TRUE)
		{
			continue;
		}

		ALint sourceState;
		alGetSourcei(soundInst.source, AL_SOURCE_STATE, &sourceState);
		if (sourceState != AL_STOPPED)
		{
			continue;
		}

		this->resetSource(soundInst.source);
		mFreeSources.push_front(soundInst.source);
		soundInst.source = INVALID_SOURCE;

		if (soundInst.isOneShot)
		{
			soundInstsToDestroy.emplace_back(soundInstID);
		}
	}

	for (const SoundInstanceID soundInstID : soundInstsToDestroy)
	{
		this->soundInstancesPool.free(soundInstID);
	}

	// Check for staged music.
	if (this->hasNextMusic())
	{
		const bool canChangeToNextMusic = (mSongStream == nullptr) || !mSongStream->isPlaying();
		if (canChangeToNextMusic)
		{
			// Assume that the next music always loops.
			constexpr bool loop = true;
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

SoundInstanceID AudioManager::allocateSound(const std::string &filename, bool isOneShot, bool is3D)
{
	const SoundInstanceID instID = this->soundInstancesPool.alloc();
	if (instID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate sound instance ID for \"%s\".", filename.c_str());
		return -1;
	}

	SoundInstance &inst = this->soundInstancesPool.get(instID);
	inst.init(filename, isOneShot, is3D);
	return instID;
}

SoundInstanceID AudioManager::allocateSound(const std::string &filename)
{
	constexpr bool isOneShot = false;
	constexpr bool is3D = false;
	return this->allocateSound(filename, isOneShot, is3D);
}

void AudioManager::freeSound(SoundInstanceID instID)
{
	SoundInstance &inst = this->soundInstancesPool.get(instID);

	if (alIsSource(inst.source) == AL_TRUE)
	{
		this->resetSource(inst.source);		
		mFreeSources.push_front(inst.source);
		inst.source = INVALID_SOURCE;
	}

	this->soundInstancesPool.free(instID);
}

double AudioManager::getSoundTotalSeconds(const std::string &filename) const
{
	if (alIsExtensionPresent("AL_SOFT_source_length") == AL_FALSE)
	{
		DebugLogWarningFormat("Missing extension for sound duration lookup.", filename.c_str());
		return 0.0;
	}

	const auto iter = mSoundBuffers.find(filename);
	if (iter == mSoundBuffers.end())
	{
		DebugLogWarningFormat("Missing sound \"%s\" for duration lookup.", filename.c_str());
		return 0.0;
	}

	const ALuint soundSource = iter->second;

	ALfloat seconds;
	alGetSourcef(soundSource, AL_SEC_LENGTH_SOFT, &seconds);
	return static_cast<double>(seconds);
}

double AudioManager::getSoundTotalSeconds(SoundInstanceID instID) const
{
	const SoundInstance &soundInst = this->soundInstancesPool.get(instID);

	ALfloat seconds;
	alGetSourcef(soundInst.source, AL_SEC_LENGTH_SOFT, &seconds);
	return static_cast<double>(seconds);
}

double AudioManager::getSoundCurrentSeconds(SoundInstanceID instID) const
{
	const SoundInstance &soundInst = this->soundInstancesPool.get(instID);

	ALfloat seconds;
	alGetSourcef(soundInst.source, AL_SEC_OFFSET, &seconds);
	return static_cast<double>(seconds);
}

bool AudioManager::isSoundPlaying(SoundInstanceID instID) const
{
	const SoundInstance &soundInst = this->soundInstancesPool.get(instID);

	ALint state;
	alGetSourcei(soundInst.source, AL_SOURCE_STATE, &state);
	return state == AL_PLAYING;
}

void AudioManager::setSoundPosition(SoundInstanceID instID, const Double3 &position)
{
	const SoundInstance &soundInst = this->soundInstancesPool.get(instID);
	const ALfloat posX = static_cast<ALfloat>(position.x);
	const ALfloat posY = static_cast<ALfloat>(position.y);
	const ALfloat posZ = static_cast<ALfloat>(position.z);
	alSource3f(soundInst.source, AL_POSITION, posX, posY, posZ);
}

void AudioManager::playSound(SoundInstanceID instID)
{
	SoundInstance &soundInst = this->soundInstancesPool.get(instID);
	const std::string &soundFilename = soundInst.filename;
	
	if (alIsSource(soundInst.source) != AL_TRUE)
	{
		if (mFreeSources.empty())
		{
			DebugLogWarningFormat("No free sources to play sound \"%s\".", soundFilename.c_str());
			return;
		}

		const bool isSingleInstance = std::find(mSingleInstanceSounds.begin(), mSingleInstanceSounds.end(), soundFilename) != mSingleInstanceSounds.end();
		const bool isAllowedToPlay = !isSingleInstance || !this->anyPlayingSounds(soundFilename);
		if (!isAllowedToPlay)
		{
			return;
		}

		soundInst.source = mFreeSources.back();
		mFreeSources.pop_back();

		const auto vocIter = mSoundBuffers.find(soundFilename);
		if (vocIter == mSoundBuffers.end())
		{
			DebugLogErrorFormat("Expected .VOC file \"%s\" to be loaded.", soundFilename.c_str());
			return;
		}

		const ALuint samplesBuffer = vocIter->second;
		alSourcei(soundInst.source, AL_BUFFER, samplesBuffer);

		const ALboolean isSourceRelative = soundInst.is3D ? AL_FALSE : AL_TRUE;
		alSourcei(soundInst.source, AL_SOURCE_RELATIVE, isSourceRelative);
		alSource3f(soundInst.source, AL_POSITION, 0.0f, 0.0f, 0.0f);

		if (mHasResamplerExtension)
		{
			alSourcei(soundInst.source, AL_SOURCE_RESAMPLER_SOFT, mResampler);
		}
	}
	else
	{
		alSourceRewind(soundInst.source);
	}

	alSourcePlay(soundInst.source);
}

void AudioManager::pauseSound(SoundInstanceID instID)
{
	const SoundInstance &soundInst = this->soundInstancesPool.get(instID);
	alSourcePause(soundInst.source);
}

void AudioManager::stopSound(SoundInstanceID instID)
{
	const SoundInstance &soundInst = this->soundInstancesPool.get(instID);
	alSourceStop(soundInst.source);
}

void AudioManager::playSoundOneShot(const std::string &filename, const Double3 &position)
{
	constexpr bool isOneShot = true;
	constexpr bool is3D = true;
	const SoundInstanceID instID = this->allocateSound(filename, isOneShot, is3D);
	if (instID < 0)
	{
		DebugLogWarningFormat("Couldn't play one-shot sound \"%s\".", filename.c_str());
		return;
	}

	this->playSound(instID);
	this->setSoundPosition(instID, position);
}

void AudioManager::playSoundOneShot(const std::string &filename)
{
	constexpr bool isOneShot = true;
	constexpr bool is3D = false;
	const SoundInstanceID instID = this->allocateSound(filename, isOneShot, is3D);
	if (instID < 0)
	{
		DebugLogWarningFormat("Couldn't play one-shot sound \"%s\".", filename.c_str());
		return;
	}

	this->playSound(instID);
}
