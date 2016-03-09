#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "FMOD\fmod.h"

#include "MusicManager.h"
#include "MusicFormat.h"
#include "MusicName.h"
#include "MusicType.h"
#include "../Math/Random.h"

// Each MusicType corresponds to a list of MusicNames.
const auto MusicTypeNames = std::map<MusicType, std::vector<MusicName>>
{
	{ MusicType::ArabCityEnter, { MusicName::ArabCityEnter } },
	{ MusicType::ArabTownEnter, { MusicName::ArabTownEnter } },
	{ MusicType::ArabVillageEnter, { MusicName::ArabVillageEnter } },
	{ MusicType::CityEnter, { MusicName::CityEnter } },
	{ MusicType::Credits, { MusicName::Credits } },
	{ MusicType::Dungeon, { MusicName::Dungeon1, MusicName::Dungeon2, MusicName::Dungeon3,
	MusicName::Dungeon4 } },
	{ MusicType::Equipment, { MusicName::Equipment } },
	{ MusicType::Evil, { MusicName::Evil } },
	{ MusicType::EvilIntro, { MusicName::EvilIntro } },
	{ MusicType::Magic, { MusicName::Magic } },
	{ MusicType::Night, { MusicName::Night } },
	{ MusicType::Overcast, { MusicName::Overcast } },
	{ MusicType::Palace, { MusicName::Palace } },
	{ MusicType::PercIntro, { MusicName::PercIntro } },
	{ MusicType::Raining, { MusicName::Raining } },
	{ MusicType::Sheet, { MusicName::Sheet } },
	{ MusicType::Sneaking, { MusicName::Sneaking } },
	{ MusicType::Snowing, { MusicName::Snowing, MusicName::OverSnow } },
	{ MusicType::Sunny, { MusicName::SunnyDay } },
	{ MusicType::Swimming, { MusicName::Swimming } },
	{ MusicType::Tavern, { MusicName::Tavern, MusicName::Square } },
	{ MusicType::Temple, { MusicName::Temple } },
	{ MusicType::TownEnter, { MusicName::TownEnter } },
	{ MusicType::VillageEnter, { MusicName::VillageEnter } },
	{ MusicType::Vision, { MusicName::Vision } },
	{ MusicType::WinGame, { MusicName::WinGame } }
};

// Each MusicName has a corresponding filename.
const auto MusicFilenames = std::map<MusicName, std::string>
{
	{ MusicName::ArabCityEnter, "arab_city_enter" },
	{ MusicName::ArabTownEnter, "arab_town_enter" },
	{ MusicName::ArabVillageEnter, "arab_village_enter" },
	{ MusicName::CityEnter, "city_enter" },
	{ MusicName::Credits, "credits" },
	{ MusicName::Dungeon1, "dungeon_1" },
	{ MusicName::Dungeon2, "dungeon_2" },
	{ MusicName::Dungeon3, "dungeon_3" },
	{ MusicName::Dungeon4, "dungeon_4" },
	{ MusicName::Equipment, "equipment" },
	{ MusicName::Evil, "evil" },
	{ MusicName::EvilIntro, "evil_intro" },
	{ MusicName::Magic, "magic" },
	{ MusicName::Night, "night" },
	{ MusicName::Overcast, "overcast" },
	{ MusicName::OverSnow, "oversnow" },
	{ MusicName::Palace, "palace" },
	{ MusicName::PercIntro, "perc_intro" },
	{ MusicName::Raining, "raining" },
	{ MusicName::Sheet, "sheet" },
	{ MusicName::Sneaking, "sneaking" },
	{ MusicName::Snowing, "snowing" },
	{ MusicName::Square, "square" },
	{ MusicName::SunnyDay, "sunny_day" },
	{ MusicName::Swimming, "swimming" },
	{ MusicName::Tavern, "tavern" },
	{ MusicName::Temple, "temple" },
	{ MusicName::TownEnter, "town_enter" },
	{ MusicName::VillageEnter, "village_enter" },
	{ MusicName::Vision, "vision" },
	{ MusicName::WinGame, "win_game" }
};

const auto MusicFormatExtensions = std::map<MusicFormat, std::string>
{
	{ MusicFormat::MIDI, ".mid" },
	{ MusicFormat::Mp3, ".mp3" },
	{ MusicFormat::Ogg, ".ogg" }
};

const std::string MusicManager::PATH = "data/music/";
const double MusicManager::MIN_VOLUME = 0.0;
const double MusicManager::MAX_VOLUME = 1.0;

MusicManager::MusicManager(MusicFormat format)
{
	// Default state.
	this->system = nullptr;
	this->channel = nullptr;
	this->musics = std::map<MusicName, FMOD_SOUND*>();

	// Create sound system.
	FMOD_RESULT result = FMOD_System_Create(&this->system);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_Create");

	// Now initialize the sound system.
	result = FMOD_System_Init(this->system, 2, FMOD_INIT_NORMAL, 0);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_Init");

	// Set initial volume to max.
	this->setVolume(MusicManager::MAX_VOLUME);

	// Set the music to loop.
	FMOD_Channel_SetMode(this->channel, FMOD_LOOP_NORMAL);

	// Load all musics.
	for (const auto &item : MusicFilenames)
	{
		// Make a blank mapping for the current music name.
		auto musicName = item.first;
		this->musics.insert(std::pair<MusicName, FMOD_SOUND*>(musicName, nullptr));

		// Get the path to the file.
		auto filename = MusicFilenames.at(musicName);
		auto extension = MusicFormatExtensions.at(format);
		auto path = MusicManager::PATH + filename + extension;

		// Use the blank mapping as the place to create the music at.
		FMOD_RESULT result = FMOD_System_CreateStream(this->system, path.c_str(),
			FMOD_SOFTWARE, nullptr, &this->musics.at(musicName));
		this->checkSuccess(result == FMOD_OK, "FMOD_System_CreateStream");
	}

	assert(this->system != nullptr);
	assert(this->channel != nullptr);
}

MusicManager::~MusicManager()
{
	// Release all stored musics.
	for (auto &item : this->musics)
	{
		auto *music = this->musics.at(item.first);
		if (this->musicIsLoaded(music))
		{
			this->releaseMusic(music);
		}
	}

	FMOD_RESULT result = FMOD_System_Close(this->system);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_Close");
}

double MusicManager::getVolume() const
{
	float volume;
	FMOD_RESULT result = FMOD_Channel_GetVolume(this->channel, &volume);
	this->checkSuccess(result == FMOD_OK, "FMOD_Channel_GetVolume");

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	return static_cast<double>(volume);
}

bool MusicManager::isPlaying() const
{
	FMOD_BOOL playing;
	FMOD_RESULT result = FMOD_Channel_IsPlaying(this->channel, &playing);
	this->checkSuccess(result == FMOD_OK, "FMOD_Channel_IsPlaying");

	// Using NULL because "playing" is not a bool.
	return playing != NULL;
}

void MusicManager::checkSuccess(bool success, const std::string &message) const
{
	if (!success)
	{
		std::cout << "Music Manager error: " << message << "." << "\n";
		std::getchar();
		exit(EXIT_FAILURE);
	}
}

bool MusicManager::musicIsLoaded(FMOD_SOUND *music) const
{
	return music != nullptr;
}

void MusicManager::releaseMusic(FMOD_SOUND *music)
{
	FMOD_RESULT result = FMOD_Sound_Release(music);
	this->checkSuccess(result == FMOD_OK, "FMOD_Sound_Release");
	music = nullptr;

	assert(music == nullptr);
}

void MusicManager::play(MusicName musicName)
{
	// Stop any currently playing music.
	FMOD_RESULT result = FMOD_Channel_Stop(this->channel);
	this->checkSuccess(result == FMOD_OK, "FMOD_Channel_Stop");

	// All musics should already be loaded.
	assert(this->musics.find(musicName) != this->musics.cend());

	result = FMOD_System_PlaySound(this->system, FMOD_CHANNEL_FREE,
		this->musics.at(musicName), false, &this->channel);
	this->checkSuccess(result == FMOD_OK, "FMOD_System_PlaySound");
}

void MusicManager::play(MusicType musicType)
{
	auto random = Random();
	auto musicNames = MusicTypeNames.at(musicType);

	assert(musicNames.size() > 0);

	auto musicName = musicNames.at(random.next(static_cast<int>(musicNames.size())));
	this->play(musicName);
}

void MusicManager::togglePause()
{
	FMOD_BOOL p;
	FMOD_Channel_GetPaused(this->channel, &p);
	FMOD_Channel_SetPaused(this->channel, !p);
}

void MusicManager::setVolume(double percent)
{
	float volume = static_cast<float>(std::max(MusicManager::MIN_VOLUME,
		std::min(MusicManager::MAX_VOLUME, percent)));

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	FMOD_Channel_SetVolume(this->channel, volume);
}
