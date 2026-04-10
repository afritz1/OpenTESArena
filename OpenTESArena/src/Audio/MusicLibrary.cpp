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

#define MAKE_NAME_TYPE_PAIR(type, name) { #name, type::name }

namespace
{
	constexpr std::pair<const char*, MusicType> MusicTypes[] =
	{
		MAKE_NAME_TYPE_PAIR(MusicType, CharacterCreation),
		MAKE_NAME_TYPE_PAIR(MusicType, Cinematic),
		MAKE_NAME_TYPE_PAIR(MusicType, Interior),
		MAKE_NAME_TYPE_PAIR(MusicType, Jingle),
		MAKE_NAME_TYPE_PAIR(MusicType, MainMenu),
		MAKE_NAME_TYPE_PAIR(MusicType, Night),
		MAKE_NAME_TYPE_PAIR(MusicType, Swimming),
		MAKE_NAME_TYPE_PAIR(MusicType, Weather)
	};

	constexpr std::pair<const char*, CinematicMusicType> CinematicMusicTypes[] =
	{
		MAKE_NAME_TYPE_PAIR(CinematicMusicType, Intro),
		MAKE_NAME_TYPE_PAIR(CinematicMusicType, DreamGood),
		MAKE_NAME_TYPE_PAIR(CinematicMusicType, DreamBad),
		MAKE_NAME_TYPE_PAIR(CinematicMusicType, Ending)
	};

	constexpr std::pair<const char*, InteriorMusicType> InteriorMusicTypes[] =
	{
		MAKE_NAME_TYPE_PAIR(InteriorMusicType, Dungeon),
		MAKE_NAME_TYPE_PAIR(InteriorMusicType, Equipment),
		MAKE_NAME_TYPE_PAIR(InteriorMusicType, House),
		MAKE_NAME_TYPE_PAIR(InteriorMusicType, MagesGuild),
		MAKE_NAME_TYPE_PAIR(InteriorMusicType, Palace),
		MAKE_NAME_TYPE_PAIR(InteriorMusicType, Tavern),
		MAKE_NAME_TYPE_PAIR(InteriorMusicType, Temple)
	};

	constexpr std::pair<const char*, ArenaCityType> JingleCityTypes[] =
	{
		MAKE_NAME_TYPE_PAIR(ArenaCityType, CityState),
		MAKE_NAME_TYPE_PAIR(ArenaCityType, Town),
		MAKE_NAME_TYPE_PAIR(ArenaCityType, Village)
	};

	constexpr std::pair<const char*, ArenaClimateType> JingleClimateTypes[] =
	{
		MAKE_NAME_TYPE_PAIR(ArenaClimateType, Temperate),
		MAKE_NAME_TYPE_PAIR(ArenaClimateType, Desert),
		MAKE_NAME_TYPE_PAIR(ArenaClimateType, Mountain)
	};

	constexpr std::pair<const char*, WeatherType> WeatherTypes[] =
	{
		MAKE_NAME_TYPE_PAIR(WeatherType, Clear),
		MAKE_NAME_TYPE_PAIR(WeatherType, Overcast),
		MAKE_NAME_TYPE_PAIR(WeatherType, Rain),
		MAKE_NAME_TYPE_PAIR(WeatherType, Snow)
	};

	template<typename T>
	bool TryParseTypeInternal(const std::string_view str, Span<const std::pair<const char*, T>> types, T *outType)
	{
		const auto iter = std::find_if(types.begin(), types.end(),
			[str](const std::pair<const char*, T> &pair)
		{
			return StringView::caseInsensitiveEquals(pair.first, str);
		});

		if (iter == types.end())
		{
			return false;
		}

		*outType = iter->second;
		return true;
	}

	bool TryParseMusicType(const std::string_view str, MusicType *outType)
	{
		if (!TryParseTypeInternal<MusicType>(str, MusicTypes, outType))
		{
			DebugLogWarningFormat("Unrecognized music type \"%s\".", std::string(str).c_str());
			return false;
		}

		return true;
	}

	bool TryParseCinematicType(const std::string_view str, CinematicMusicType *outCinematicType)
	{
		if (!TryParseTypeInternal<CinematicMusicType>(str, CinematicMusicTypes, outCinematicType))
		{
			DebugLogWarningFormat("Unrecognized cinematic music type \"%s\".", std::string(str).c_str());
			return false;
		}

		return true;
	}

	bool TryParseInteriorType(const std::string_view str, InteriorMusicType *outInteriorType)
	{
		if (!TryParseTypeInternal<InteriorMusicType>(str, InteriorMusicTypes, outInteriorType))
		{
			DebugLogWarningFormat("Unrecognized interior music type \"%s\".", std::string(str).c_str());
			return false;
		}

		return true;
	}

	bool TryParseJingleCityType(const std::string_view str, ArenaCityType *outCityType)
	{
		if (!TryParseTypeInternal<ArenaCityType>(str, JingleCityTypes, outCityType))
		{
			DebugLogWarningFormat("Unrecognized city type \"%s\".", std::string(str).c_str());
			return false;
		}

		return true;
	}

	bool TryParseJingleClimateType(const std::string_view str, ArenaClimateType *outClimateType)
	{
		if (!TryParseTypeInternal<ArenaClimateType>(str, JingleClimateTypes, outClimateType))
		{
			DebugLogWarningFormat("Unrecognized climate type \"%s\".", std::string(str).c_str());
			return false;
		}

		return true;
	}

	bool TryParseWeatherType(const std::string_view str, WeatherType *outWeatherType)
	{
		if (!TryParseTypeInternal<WeatherType>(str, WeatherTypes, outWeatherType))
		{
			DebugLogWarningFormat("Unrecognized weather type \"%s\".", std::string(str).c_str());
			return false;
		}

		return true;
	}

	bool TryParseWeatherBoolArg(const std::string_view str, bool *outValue)
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
			DebugLogWarningFormat("Unrecognized weather argument \"%s\".", std::string(str).c_str());
			return false;
		}

		return true;
	}
}

bool MusicLibrary::tryParseValue(const std::string_view valueStr, MusicType type, MusicDefinition *outDefinition)
{
	constexpr char VALUE_SEPARATOR = ',';
	const Buffer<std::string_view> strs = StringView::split(valueStr, VALUE_SEPARATOR);
	if (strs.getCount() == 0)
	{
		DebugLogWarningFormat("No music definition in string \"%s\".", std::string(valueStr).c_str());
		return false;
	}

	const std::string musicFilename(strs[0]);

	if (type == MusicType::CharacterCreation)
	{
		if (strs.getCount() != 1)
		{
			DebugLogErrorFormat("Expected 1 token in character creation music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		outDefinition->initCharacterCreation(musicFilename);
	}
	else if (type == MusicType::Cinematic)
	{
		if (strs.getCount() != 2)
		{
			DebugLogErrorFormat("Expected 2 tokens in cinematic music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		CinematicMusicType cinematicType;
		if (!TryParseCinematicType(strs[1], &cinematicType))
		{
			DebugLogWarningFormat("Couldn't parse type in cinematic music definition \"%s\".", std::string(valueStr).c_str());
			return false;
		}

		outDefinition->initCinematic(musicFilename, cinematicType);
	}
	else if (type == MusicType::Interior)
	{
		if (strs.getCount() != 2)
		{
			DebugLogErrorFormat("Expected 2 tokens in interior music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		InteriorMusicType interiorType;
		if (!TryParseInteriorType(strs[1], &interiorType))
		{
			DebugLogWarningFormat("Couldn't parse type in interior music definition \"%s\".", std::string(valueStr).c_str());
			return false;
		}

		outDefinition->initInterior(musicFilename, interiorType);
	}
	else if (type == MusicType::Jingle)
	{
		if (strs.getCount() != 3)
		{
			DebugLogErrorFormat("Expected 3 tokens in jingle music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		ArenaCityType cityType;
		if (!TryParseJingleCityType(strs[1], &cityType))
		{
			DebugLogWarningFormat("Couldn't parse city type in jingle music definition \"%s\".", std::string(valueStr).c_str());
			return false;
		}

		ArenaClimateType climateType;
		if (!TryParseJingleClimateType(strs[2], &climateType))
		{
			DebugLogWarningFormat("Couldn't parse climate type in jingle music definition \"%s\".", std::string(valueStr).c_str());
			return false;
		}

		outDefinition->initJingle(musicFilename, cityType, climateType);
	}
	else if (type == MusicType::MainMenu)
	{
		if (strs.getCount() != 1)
		{
			DebugLogErrorFormat("Expected 1 token in main menu music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		outDefinition->initMainMenu(musicFilename);
	}
	else if (type == MusicType::Night)
	{
		if (strs.getCount() != 1)
		{
			DebugLogErrorFormat("Expected 1 token in night music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		outDefinition->initNight(musicFilename);
	}
	else if (type == MusicType::Swimming)
	{
		if (strs.getCount() != 1)
		{
			DebugLogErrorFormat("Expected 1 token in swimming music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		outDefinition->initSwimming(musicFilename);
	}
	else if (type == MusicType::Weather)
	{
		// Variable arguments depending on the weather type.
		if (strs.getCount() < 2)
		{
			DebugLogErrorFormat("Expected at least 2 tokens in weather music definition \"%s\" (got %d).", std::string(valueStr).c_str(), strs.getCount());
			return false;
		}

		WeatherType weatherType;
		if (!TryParseWeatherType(strs[1], &weatherType))
		{
			DebugLogWarningFormat("Couldn't parse weather type in weather music definition \"%s\".", std::string(valueStr).c_str());
			return false;
		}

		WeatherDefinition weatherDef;
		if (weatherType == WeatherType::Clear)
		{
			if (strs.getCount() != 2)
			{
				DebugLogWarningFormat("Incorrect argument count for clear weather music definition \"%s\".", std::string(valueStr).c_str());
				return false;
			}

			weatherDef.initClear();
		}
		else if (weatherType == WeatherType::Overcast)
		{
			if (strs.getCount() != 3)
			{
				DebugLogWarningFormat("Incorrect argument count for overcast weather music definition \"%s\".", std::string(valueStr).c_str());
				return false;
			}

			bool heavyFog;
			if (!TryParseWeatherBoolArg(strs[2], &heavyFog))
			{
				DebugLogWarningFormat("Couldn't parse argument in overcast weather music definition \"%s\".", std::string(valueStr).c_str());
				return false;
			}

			weatherDef.initOvercast(heavyFog);
		}
		else if (weatherType == WeatherType::Rain)
		{
			if (strs.getCount() != 3)
			{
				DebugLogWarningFormat("Incorrect argument count for rain weather music definition \"%s\".", std::string(valueStr).c_str());
				return false;
			}

			bool thunderstorm;
			if (!TryParseWeatherBoolArg(strs[2], &thunderstorm))
			{
				DebugLogWarningFormat("Couldn't parse argument in rain weather music definition \"%s\".", std::string(valueStr).c_str());
				return false;
			}

			weatherDef.initRain(thunderstorm);
		}
		else if (weatherType == WeatherType::Snow)
		{
			if (strs.getCount() != 4)
			{
				DebugLogWarningFormat("Incorrect argument count for snow weather music definition \"%s\".", std::string(valueStr).c_str());
				return false;
			}

			bool overcast;
			if (!TryParseWeatherBoolArg(strs[2], &overcast))
			{
				DebugLogWarningFormat("Couldn't parse first argument in snow weather music definition \"%s\".", std::string(valueStr).c_str());
				return false;
			}

			bool heavyFog;
			if (!TryParseWeatherBoolArg(strs[3], &heavyFog))
			{
				DebugLogWarningFormat("Couldn't parse second argument in snow weather music definition \"%s\".", std::string(valueStr).c_str());
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
		DebugLogErrorFormat("Couldn't init KeyValueFile \"%s\".", filename);
		return false;
	}

	for (int i = 0; i < keyValueFile.getSectionCount(); i++)
	{
		const KeyValueFileSection &section = keyValueFile.getSection(i);

		MusicType sectionType;
		if (!TryParseMusicType(section.getName(), &sectionType))
		{
			DebugLogWarningFormat("Couldn't parse section type \"%s\".", section.getName().c_str());
			continue;
		}

		auto definitionsIter = this->definitions.find(sectionType);
		if (definitionsIter == this->definitions.end())
		{
			definitionsIter = this->definitions.emplace(sectionType, std::vector<MusicDefinition>()).first;
		}

		for (int j = 0; j < section.getPairCount(); j++)
		{
			const std::pair<std::string, std::string> &pair = section.getPair(j);

			MusicDefinition musicDefinition;
			if (!MusicLibrary::tryParseValue(pair.second, sectionType, &musicDefinition))
			{
				DebugLogWarningFormat("Couldn't parse value on music line \"%s\" in section \"%s\".", pair.first.c_str(), section.getName().c_str());
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
	if (iter == this->definitions.end())
	{
		return 0;
	}

	return static_cast<int>(iter->second.size());
}

const MusicDefinition *MusicLibrary::getMusicDefinition(MusicType type, int index) const
{
	const auto iter = this->definitions.find(type);
	if (iter == this->definitions.end())
	{
		return nullptr;
	}

	Span<const MusicDefinition> defs = iter->second;
	return &defs[index];
}

const MusicDefinition *MusicLibrary::getFirstMusicDefinition(MusicType type) const
{
	if (this->getMusicDefinitionCount(type) == 0)
	{
		return nullptr;
	}

	return this->getMusicDefinition(type, 0);
}

const MusicDefinition *MusicLibrary::getRandomMusicDefinition(MusicType type, Random &random) const
{
	const int count = this->getMusicDefinitionCount(type);
	if (count == 0)
	{
		return nullptr;
	}

	const int index = random.next(count);
	return this->getMusicDefinition(type, index);
}

const MusicDefinition *MusicLibrary::getRandomMusicDefinitionIf(MusicType type, Random &random, const MusicDefinitionPredicate &predicate) const
{
	Buffer<int> musicDefIndices(this->getMusicDefinitionCount(type));
	std::iota(musicDefIndices.begin(), musicDefIndices.end(), 0);
	RandomUtils::shuffle<int>(musicDefIndices, random);

	for (const int index : musicDefIndices)
	{
		const MusicDefinition *musicDef = this->getMusicDefinition(type, index);
		if (predicate(*musicDef))
		{
			return musicDef;
		}
	}

	return nullptr;
}
