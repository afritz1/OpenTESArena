#include <algorithm>
#include <numeric>
#include <sstream>

#include "MusicLibrary.h"
#include "../Math/Random.h"
#include "../Math/RandomUtils.h"
#include "../World/ClimateType.h"
#include "../World/LocationDefinition.h"
#include "../World/WeatherType.h"

#include "components/debug/Debug.h"
#include "components/utilities/File.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/StringView.h"

#define MAKE_MUSIC_DEFINITION_PAIR(name) { #name, MusicDefinition::Type::name }

namespace
{
	const std::array<std::pair<std::string, MusicDefinition::Type>, 9> MusicDefinitionTypes =
	{
		{
			MAKE_MUSIC_DEFINITION_PAIR(CharacterCreation),
			MAKE_MUSIC_DEFINITION_PAIR(Cinematic),
			MAKE_MUSIC_DEFINITION_PAIR(Dungeon),
			MAKE_MUSIC_DEFINITION_PAIR(Interior),
			MAKE_MUSIC_DEFINITION_PAIR(Jingle),
			MAKE_MUSIC_DEFINITION_PAIR(MainMenu),
			MAKE_MUSIC_DEFINITION_PAIR(Night),
			MAKE_MUSIC_DEFINITION_PAIR(Swimming),
			MAKE_MUSIC_DEFINITION_PAIR(Weather)
		}
	};
}

bool MusicLibrary::tryParseType(const std::string_view &typeStr, MusicDefinition::Type *outType)
{
	const auto iter = std::find_if(MusicDefinitionTypes.begin(), MusicDefinitionTypes.end(),
		[&typeStr](const auto &pair)
	{
		return pair.first == typeStr;
	});

	if (iter != MusicDefinitionTypes.end())
	{
		*outType = iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

bool MusicLibrary::tryParseValue(const std::string_view &valueStr, MusicDefinition::Type type,
	MusicDefinition *outDefinition)
{
	auto tryParseCinematicType = [](const std::string_view &str,
		MusicDefinition::CinematicMusicDefinition::Type *outCinematicType)
	{
		if (str == "Intro")
		{
			*outCinematicType = MusicDefinition::CinematicMusicDefinition::Type::Intro;
		}
		else if (str == "DreamGood")
		{
			*outCinematicType = MusicDefinition::CinematicMusicDefinition::Type::DreamGood;
		}
		else if (str == "DreamBad")
		{
			*outCinematicType = MusicDefinition::CinematicMusicDefinition::Type::DreamBad;
		}
		else if (str == "Ending")
		{
			*outCinematicType = MusicDefinition::CinematicMusicDefinition::Type::Ending;
		}
		else
		{
			DebugLogWarning("Unrecognized cinematic music type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseInteriorType = [](const std::string_view &str,
		MusicDefinition::InteriorMusicDefinition::Type *outInteriorType)
	{
		if (str == "Equipment")
		{
			*outInteriorType = MusicDefinition::InteriorMusicDefinition::Type::Equipment;
		}
		else if (str == "House")
		{
			*outInteriorType = MusicDefinition::InteriorMusicDefinition::Type::House;
		}
		else if (str == "MagesGuild")
		{
			*outInteriorType = MusicDefinition::InteriorMusicDefinition::Type::MagesGuild;
		}
		else if (str == "Palace")
		{
			*outInteriorType = MusicDefinition::InteriorMusicDefinition::Type::Palace;
		}
		else if (str == "Tavern")
		{
			*outInteriorType = MusicDefinition::InteriorMusicDefinition::Type::Tavern;
		}
		else if (str == "Temple")
		{
			*outInteriorType = MusicDefinition::InteriorMusicDefinition::Type::Temple;
		}
		else
		{
			DebugLogWarning("Unrecognized interior music type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseJingleCityType = [](const std::string_view &str, ArenaTypes::CityType *outCityType)
	{
		if (str == "CityState")
		{
			*outCityType = ArenaTypes::CityType::CityState;
		}
		else if (str == "Town")
		{
			*outCityType = ArenaTypes::CityType::Town;
		}
		else if (str == "Village")
		{
			*outCityType = ArenaTypes::CityType::Village;
		}
		else
		{
			DebugLogWarning("Unrecognized city type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseJingleClimateType = [](const std::string_view &str, ClimateType *outClimateType)
	{
		if (str == "Temperate")
		{
			*outClimateType = ClimateType::Temperate;
		}
		else if (str == "Desert")
		{
			*outClimateType = ClimateType::Desert;
		}
		else if (str == "Mountain")
		{
			*outClimateType = ClimateType::Mountain;
		}
		else
		{
			DebugLogWarning("Unrecognized climate type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseWeatherType = [](const std::string_view &str, WeatherType *outWeatherType)
	{
		if (str == "Clear")
		{
			*outWeatherType = WeatherType::Clear;
		}
		else if (str == "Overcast")
		{
			*outWeatherType = WeatherType::Overcast;
		}
		else if (str == "Rain")
		{
			*outWeatherType = WeatherType::Rain;
		}
		else if (str == "Snow")
		{
			*outWeatherType = WeatherType::Snow;
		}
		else if (str == "SnowOvercast")
		{
			*outWeatherType = WeatherType::SnowOvercast;
		}
		else if (str == "Overcast2")
		{
			*outWeatherType = WeatherType::Overcast2;
		}
		else if (str == "Rain2")
		{
			*outWeatherType = WeatherType::Rain2;
		}
		else if (str == "SnowOvercast2")
		{
			*outWeatherType = WeatherType::SnowOvercast2;
		}
		else
		{
			DebugLogWarning("Unrecognized weather type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	constexpr char VALUE_SEPARATOR = ',';
	const std::vector<std::string_view> strs = StringView::split(valueStr, VALUE_SEPARATOR);
	if (strs.size() == 0)
	{
		DebugLogWarning("No music definition in string \"" + std::string(valueStr) + "\".");
		return false;
	}

	std::string musicFilename(strs[0]);

	if (type == MusicDefinition::Type::CharacterCreation)
	{
		DebugAssert(strs.size() == 1);
		outDefinition->initCharacterCreation(std::move(musicFilename));
	}
	else if (type == MusicDefinition::Type::Cinematic)
	{
		DebugAssert(strs.size() == 2);

		MusicDefinition::CinematicMusicDefinition::Type cinematicType;
		if (!tryParseCinematicType(strs[1], &cinematicType))
		{
			DebugLogWarning("Couldn't parse type in cinematic music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		outDefinition->initCinematic(std::move(musicFilename), cinematicType);
	}
	else if (type == MusicDefinition::Type::Dungeon)
	{
		DebugAssert(strs.size() == 1);
		outDefinition->initDungeon(std::move(musicFilename));
	}
	else if (type == MusicDefinition::Type::Interior)
	{
		DebugAssert(strs.size() == 2);

		MusicDefinition::InteriorMusicDefinition::Type interiorType;
		if (!tryParseInteriorType(strs[1], &interiorType))
		{
			DebugLogWarning("Couldn't parse type in interior music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		outDefinition->initInterior(std::move(musicFilename), interiorType);
	}
	else if (type == MusicDefinition::Type::Jingle)
	{
		DebugAssert(strs.size() == 3);

		ArenaTypes::CityType cityType;
		if (!tryParseJingleCityType(strs[1], &cityType))
		{
			DebugLogWarning("Couldn't parse city type in jingle music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		ClimateType climateType;
		if (!tryParseJingleClimateType(strs[2], &climateType))
		{
			DebugLogWarning("Couldn't parse climate type in jingle music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		outDefinition->initJingle(std::move(musicFilename), cityType, climateType);
	}
	else if (type == MusicDefinition::Type::MainMenu)
	{
		DebugAssert(strs.size() == 1);
		outDefinition->initMainMenu(std::move(musicFilename));
	}
	else if (type == MusicDefinition::Type::Night)
	{
		DebugAssert(strs.size() == 1);
		outDefinition->initNight(std::move(musicFilename));
	}
	else if (type == MusicDefinition::Type::Swimming)
	{
		DebugAssert(strs.size() == 1);
		outDefinition->initSwimming(std::move(musicFilename));
	}
	else if (type == MusicDefinition::Type::Weather)
	{
		DebugAssert(strs.size() == 2);

		WeatherType weatherType;
		if (!tryParseWeatherType(strs[1], &weatherType))
		{
			DebugLogWarning("Couldn't parse weather type in weather music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		outDefinition->initWeather(std::move(musicFilename), weatherType);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
		return false;
	}

	return true;
}

bool MusicLibrary::init(const char *filename)
{
	KeyValueFile keyValueFile;
	if (!keyValueFile.init(filename))
	{
		DebugLogError("Couldn't init KeyValueFile \"" + std::string(filename) + "\".");
		return false;
	}

	for (int i = 0; i < keyValueFile.getSectionCount(); i++)
	{
		const KeyValueFile::Section &section = keyValueFile.getSection(i);

		MusicDefinition::Type sectionType;
		if (!MusicLibrary::tryParseType(section.getName(), &sectionType))
		{
			DebugLogWarning("Couldn't parse section type \"" + section.getName() + "\".");
			continue;
		}

		auto definitionsIter = this->definitions.find(sectionType);
		if (definitionsIter == this->definitions.end())
		{
			definitionsIter = this->definitions.emplace(
				std::make_pair(sectionType, std::vector<MusicDefinition>())).first;
		}

		for (int j = 0; j < section.getPairCount(); j++)
		{
			const auto &pair = section.getPair(j);

			MusicDefinition musicDefinition;
			if (!MusicLibrary::tryParseValue(pair.second, sectionType, &musicDefinition))
			{
				DebugLogWarning("Couldn't parse value on music line \"" + pair.first +
					"\" in section \"" + section.getName() + "\".");
				continue;
			}

			definitionsIter->second.push_back(std::move(musicDefinition));
		}
	}

	return true;
}

int MusicLibrary::getMusicDefinitionCount(MusicDefinition::Type type) const
{
	const auto iter = this->definitions.find(type);
	return (iter != this->definitions.end()) ? static_cast<int>(iter->second.size()) : 0;
}

const MusicDefinition *MusicLibrary::getMusicDefinition(MusicDefinition::Type type, int index) const
{
	const auto iter = this->definitions.find(type);
	if (iter != this->definitions.end())
	{
		const std::vector<MusicDefinition> &defs = iter->second;
		DebugAssertIndex(defs, index);
		return &defs[index];
	}
	else
	{
		return nullptr;
	}
}

const MusicDefinition *MusicLibrary::getFirstMusicDefinition(MusicDefinition::Type type) const
{
	return (this->getMusicDefinitionCount(type) > 0) ? this->getMusicDefinition(type, 0) : nullptr;
}

const MusicDefinition *MusicLibrary::getRandomMusicDefinition(MusicDefinition::Type type, Random &random) const
{
	const int count = this->getMusicDefinitionCount(type);
	if (count > 0)
	{
		const int index = random.next(count);
		return this->getMusicDefinition(type, index);
	}
	else
	{
		return nullptr;
	}
}

const MusicDefinition *MusicLibrary::getRandomMusicDefinitionIf(MusicDefinition::Type type,
	Random &random, const Predicate &predicate) const
{
	std::vector<int> musicDefIndices(this->getMusicDefinitionCount(type));
	std::iota(musicDefIndices.begin(), musicDefIndices.end(), 0);
	RandomUtils::shuffle(musicDefIndices.data(), static_cast<int>(musicDefIndices.size()), random);

	for (const int index : musicDefIndices)
	{
		const MusicDefinition &musicDef = *this->getMusicDefinition(type, index);
		if (predicate(musicDef))
		{
			return &musicDef;
		}
	}

	return nullptr;
}
