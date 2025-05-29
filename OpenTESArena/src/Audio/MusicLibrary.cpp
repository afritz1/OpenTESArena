#include <algorithm>
#include <numeric>
#include <sstream>

#include "MusicLibrary.h"
#include "../Assets/ArenaTypes.h"
#include "../Math/Random.h"
#include "../Math/RandomUtils.h"
#include "../Weather/WeatherDefinition.h"
#include "../WorldMap/LocationDefinition.h"

#include "components/debug/Debug.h"
#include "components/utilities/File.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/StringView.h"

#define MAKE_MUSIC_DEFINITION_PAIR(name) { #name, MusicType::name }

namespace
{
	const std::array<std::pair<std::string, MusicType>, 9> MusicDefinitionTypes =
	{
		{
			MAKE_MUSIC_DEFINITION_PAIR(CharacterCreation),
			MAKE_MUSIC_DEFINITION_PAIR(Cinematic),
			MAKE_MUSIC_DEFINITION_PAIR(Interior),
			MAKE_MUSIC_DEFINITION_PAIR(Jingle),
			MAKE_MUSIC_DEFINITION_PAIR(MainMenu),
			MAKE_MUSIC_DEFINITION_PAIR(Night),
			MAKE_MUSIC_DEFINITION_PAIR(Swimming),
			MAKE_MUSIC_DEFINITION_PAIR(Weather)
		}
	};
}

bool MusicLibrary::tryParseType(const std::string_view typeStr, MusicType *outType)
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

bool MusicLibrary::tryParseValue(const std::string_view valueStr, MusicType type, MusicDefinition *outDefinition)
{
	auto tryParseCinematicType = [](const std::string_view str, CinematicMusicType *outCinematicType)
	{
		if (str == "Intro")
		{
			*outCinematicType = CinematicMusicType::Intro;
		}
		else if (str == "DreamGood")
		{
			*outCinematicType = CinematicMusicType::DreamGood;
		}
		else if (str == "DreamBad")
		{
			*outCinematicType = CinematicMusicType::DreamBad;
		}
		else if (str == "Ending")
		{
			*outCinematicType = CinematicMusicType::Ending;
		}
		else
		{
			DebugLogWarning("Unrecognized cinematic music type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseInteriorType = [](const std::string_view str, InteriorMusicType *outInteriorType)
	{
		if (str == "Dungeon")
		{
			*outInteriorType = InteriorMusicType::Dungeon;
		}
		else if (str == "Equipment")
		{
			*outInteriorType = InteriorMusicType::Equipment;
		}
		else if (str == "House")
		{
			*outInteriorType = InteriorMusicType::House;
		}
		else if (str == "MagesGuild")
		{
			*outInteriorType = InteriorMusicType::MagesGuild;
		}
		else if (str == "Palace")
		{
			*outInteriorType = InteriorMusicType::Palace;
		}
		else if (str == "Tavern")
		{
			*outInteriorType = InteriorMusicType::Tavern;
		}
		else if (str == "Temple")
		{
			*outInteriorType = InteriorMusicType::Temple;
		}
		else
		{
			DebugLogWarning("Unrecognized interior music type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseJingleCityType = [](const std::string_view str, ArenaCityType *outCityType)
	{
		if (str == "CityState")
		{
			*outCityType = ArenaCityType::CityState;
		}
		else if (str == "Town")
		{
			*outCityType = ArenaCityType::Town;
		}
		else if (str == "Village")
		{
			*outCityType = ArenaCityType::Village;
		}
		else
		{
			DebugLogWarning("Unrecognized city type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseJingleClimateType = [](const std::string_view str, ArenaClimateType *outClimateType)
	{
		if (str == "Temperate")
		{
			*outClimateType = ArenaClimateType::Temperate;
		}
		else if (str == "Desert")
		{
			*outClimateType = ArenaClimateType::Desert;
		}
		else if (str == "Mountain")
		{
			*outClimateType = ArenaClimateType::Mountain;
		}
		else
		{
			DebugLogWarning("Unrecognized climate type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	auto tryParseWeatherType = [](const std::string_view str, WeatherType *outWeatherType)
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
		else
		{
			DebugLogWarning("Unrecognized weather type \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	// All weather arguments (heavy fog, etc.) are bools.
	auto tryParseWeatherBoolArg = [](const std::string_view str, bool *outValue)
	{
		if (StringView::caseInsensitiveEquals(str, "True"))
		{
			*outValue = true;
		}
		else if (StringView::caseInsensitiveEquals(str, "False"))
		{
			*outValue = false;
		}
		else
		{
			DebugLogWarning("Unrecognized weather argument \"" + std::string(str) + "\".");
			return false;
		}

		return true;
	};

	constexpr char VALUE_SEPARATOR = ',';
	const Buffer<std::string_view> strs = StringView::split(valueStr, VALUE_SEPARATOR);
	if (strs.getCount() == 0)
	{
		DebugLogWarning("No music definition in string \"" + std::string(valueStr) + "\".");
		return false;
	}

	std::string musicFilename(strs[0]);

	if (type == MusicType::CharacterCreation)
	{
		DebugAssert(strs.getCount() == 1);
		outDefinition->initCharacterCreation(musicFilename);
	}
	else if (type == MusicType::Cinematic)
	{
		DebugAssert(strs.getCount() == 2);

		CinematicMusicType cinematicType;
		if (!tryParseCinematicType(strs[1], &cinematicType))
		{
			DebugLogWarning("Couldn't parse type in cinematic music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		outDefinition->initCinematic(musicFilename, cinematicType);
	}
	else if (type == MusicType::Interior)
	{
		DebugAssert(strs.getCount() == 2);

		InteriorMusicType interiorType;
		if (!tryParseInteriorType(strs[1], &interiorType))
		{
			DebugLogWarning("Couldn't parse type in interior music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		outDefinition->initInterior(musicFilename, interiorType);
	}
	else if (type == MusicType::Jingle)
	{
		DebugAssert(strs.getCount() == 3);

		ArenaCityType cityType;
		if (!tryParseJingleCityType(strs[1], &cityType))
		{
			DebugLogWarning("Couldn't parse city type in jingle music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		ArenaClimateType climateType;
		if (!tryParseJingleClimateType(strs[2], &climateType))
		{
			DebugLogWarning("Couldn't parse climate type in jingle music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		outDefinition->initJingle(musicFilename, cityType, climateType);
	}
	else if (type == MusicType::MainMenu)
	{
		DebugAssert(strs.getCount() == 1);
		outDefinition->initMainMenu(musicFilename);
	}
	else if (type == MusicType::Night)
	{
		DebugAssert(strs.getCount() == 1);
		outDefinition->initNight(musicFilename);
	}
	else if (type == MusicType::Swimming)
	{
		DebugAssert(strs.getCount() == 1);
		outDefinition->initSwimming(musicFilename);
	}
	else if (type == MusicType::Weather)
	{
		// Variable arguments depending on the weather type.
		DebugAssert(strs.getCount() >= 2);

		WeatherType weatherType;
		if (!tryParseWeatherType(strs[1], &weatherType))
		{
			DebugLogWarning("Couldn't parse weather type in weather music definition \"" + std::string(valueStr) + "\".");
			return false;
		}

		WeatherDefinition weatherDef;
		if (weatherType == WeatherType::Clear)
		{
			if (strs.getCount() != 2)
			{
				DebugLogWarning("Incorrect argument count for clear weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			weatherDef.initClear();
		}
		else if (weatherType == WeatherType::Overcast)
		{
			if (strs.getCount() != 3)
			{
				DebugLogWarning("Incorrect argument count for overcast weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			bool heavyFog;
			if (!tryParseWeatherBoolArg(strs[2], &heavyFog))
			{
				DebugLogWarning("Couldn't parse argument in overcast weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			weatherDef.initOvercast(heavyFog);
		}
		else if (weatherType == WeatherType::Rain)
		{
			if (strs.getCount() != 3)
			{
				DebugLogWarning("Incorrect argument count for rain weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			bool thunderstorm;
			if (!tryParseWeatherBoolArg(strs[2], &thunderstorm))
			{
				DebugLogWarning("Couldn't parse argument in rain weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			weatherDef.initRain(thunderstorm);
		}
		else if (weatherType == WeatherType::Snow)
		{
			if (strs.getCount() != 4)
			{
				DebugLogWarning("Incorrect argument count for snow weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			bool overcast;
			if (!tryParseWeatherBoolArg(strs[2], &overcast))
			{
				DebugLogWarning("Couldn't parse first argument in snow weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			bool heavyFog;
			if (!tryParseWeatherBoolArg(strs[3], &heavyFog))
			{
				DebugLogWarning("Couldn't parse second argument in snow weather music definition \"" + std::string(valueStr) + "\".");
				return false;
			}

			weatherDef.initSnow(overcast, heavyFog);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(weatherType)));
		}

		outDefinition->initWeather(musicFilename, weatherDef);
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
		const KeyValueFileSection &section = keyValueFile.getSection(i);

		MusicType sectionType;
		if (!MusicLibrary::tryParseType(section.getName(), &sectionType))
		{
			DebugLogWarning("Couldn't parse section type \"" + section.getName() + "\".");
			continue;
		}

		auto definitionsIter = this->definitions.find(sectionType);
		if (definitionsIter == this->definitions.end())
		{
			definitionsIter = this->definitions.emplace(sectionType, std::vector<MusicDefinition>()).first;
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

			definitionsIter->second.emplace_back(std::move(musicDefinition));
		}
	}

	return true;
}

int MusicLibrary::getMusicDefinitionCount(MusicType type) const
{
	const auto iter = this->definitions.find(type);
	return (iter != this->definitions.end()) ? static_cast<int>(iter->second.size()) : 0;
}

const MusicDefinition *MusicLibrary::getMusicDefinition(MusicType type, int index) const
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

const MusicDefinition *MusicLibrary::getFirstMusicDefinition(MusicType type) const
{
	return (this->getMusicDefinitionCount(type) > 0) ? this->getMusicDefinition(type, 0) : nullptr;
}

const MusicDefinition *MusicLibrary::getRandomMusicDefinition(MusicType type, Random &random) const
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

const MusicDefinition *MusicLibrary::getRandomMusicDefinitionIf(MusicType type,
	Random &random, const Predicate &predicate) const
{
	Buffer<int> musicDefIndices(this->getMusicDefinitionCount(type));
	std::iota(musicDefIndices.begin(), musicDefIndices.end(), 0);
	RandomUtils::shuffle<int>(musicDefIndices, random);

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
