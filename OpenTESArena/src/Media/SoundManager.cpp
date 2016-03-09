#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>

#include "FMOD\fmod.h"

#include "SoundManager.h"
#include "SoundFormat.h"
#include "SoundName.h"

const auto SoundFilenames = std::map<SoundName, std::string>
{

};

// There does need to be a mapping of sound names to sound formats because the
// extension for each filename is not given with the filename itself.
const auto SoundNameFormats = std::map<SoundName, SoundFormat>
{

};

const auto SoundFormatExtensions = std::map<SoundFormat, std::string>
{
	{ SoundFormat::Ogg, ".ogg" },
	{ SoundFormat::WAV, ".wav" }
};

const std::string SoundManager::PATH = "data/sounds/";
const double SoundManager::MIN_VOLUME = 0.0;
const double SoundManager::MAX_VOLUME = 1.0;

SoundManager::SoundManager()
{
	// This may not work if two FMOD instances aren't allowed (with MusicManager).
	
	// Default state.
	this->system = nullptr;
	this->channel = nullptr;
	this->sounds = std::map<SoundName, FMOD_SOUND*>();

	// Create sound system.
	FMOD_RESULT result = FMOD_System_Create(&this->system);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_Create");

	// Now initialize the sound system.
	result = FMOD_System_Init(this->system, 2, FMOD_INIT_NORMAL, 0);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_Init");

	// Set initial volume to max.
	this->setVolume(SoundManager::MAX_VOLUME);
	
	// Load all sounds.
	for (const auto &item : SoundFilenames)
	{
		// Make a blank mapping for the current sound name.
		auto soundName = item.first;
		this->sounds.insert(std::pair<SoundName, FMOD_SOUND*>(soundName, nullptr));

		// Get the full path to the file.
		auto filename = SoundFilenames.at(soundName);
		auto extension = SoundFormatExtensions.at(SoundNameFormats.at(soundName));
		auto path = SoundManager::PATH + filename + extension;

		// Use the blank mapping as the place to create the sound at.
		FMOD_RESULT result = FMOD_System_CreateStream(this->system, path.c_str(),
			FMOD_SOFTWARE, nullptr, &this->sounds.at(soundName));
		this->checkSuccess(result == FMOD_OK, "FMOD_System_CreateStream");
	}

	assert(this->system != nullptr);
	assert(this->channel != nullptr);
}

SoundManager::~SoundManager()
{
	// Release all stored musics.
	for (auto &item : this->sounds)
	{
		auto *sound = this->sounds.at(item.first);
		if (this->soundIsLoaded(sound))
		{
			this->releaseSound(sound);
		}
	}

	FMOD_RESULT result = FMOD_System_Close(this->system);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_Close");
}

double SoundManager::getVolume() const
{
	float volume;
	FMOD_RESULT result = FMOD_Channel_GetVolume(this->channel, &volume);
	this->checkSuccess(result == FMOD_OK, "FMOD_Channel_GetVolume");

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	return static_cast<double>(volume);
}

void SoundManager::checkSuccess(bool success, const std::string &message) const
{
	if (!success)
	{
		std::cout << "Sound Manager error: " << message << "." << "\n";
		std::getchar();
		exit(EXIT_FAILURE);
	}
}

bool SoundManager::soundIsLoaded(FMOD_SOUND *sound) const
{
	return sound != nullptr;
}

void SoundManager::releaseSound(FMOD_SOUND *sound)
{
	FMOD_RESULT result = FMOD_Sound_Release(sound);
	this->checkSuccess(result == FMOD_OK, "FMOD_Sound_Release");
	sound = nullptr;

	assert(sound == nullptr);
}

void SoundManager::play(SoundName sound)
{
	// This hasn't been tested, so it might only play one sound per channel. Who knows?!
	// It might even go out of bounds in the channel, expecting it to be an array!

	// All sounds should already be loaded.
	assert(this->sounds.find(sound) != this->sounds.cend());

	FMOD_RESULT result = FMOD_System_PlaySound(this->system, FMOD_CHANNEL_FREE,
		this->sounds.at(sound), false, &this->channel);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_PlaySound");
}

void SoundManager::setVolume(double percent)
{
	float volume = static_cast<float>(std::max(SoundManager::MIN_VOLUME,
		std::min(SoundManager::MAX_VOLUME, percent)));

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	FMOD_Channel_SetVolume(this->channel, volume);
}