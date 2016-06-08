#include <cassert>

#include "OptionsParser.h"

#include "Options.h"
#include "../Media/MusicFormat.h"
#include "../Media/SoundFormat.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/String.h"

const auto OptionsParserMusicFormats = std::map<std::string, MusicFormat>
{
	{ "MIDI", MusicFormat::MIDI },
	{ "MP3", MusicFormat::MP3 },
	{ "Ogg", MusicFormat::Ogg }
};

// Technically, "WAV" is "wave", but oh well.
const auto OptionsParserSoundFormats = std::map<std::string, SoundFormat>
{
	{ "Ogg", SoundFormat::Ogg },
	{ "WAV", SoundFormat::WAV }
};

const auto OptionsParserBooleans = std::map<std::string, bool>
{
	{ "True", true },
	{ "False", false }
};

const std::string OptionsParser::PATH = "options/";
const std::string OptionsParser::FILENAME = "options.txt";

const std::string OptionsParser::SCREEN_WIDTH_KEY = "ScreenWidth";
const std::string OptionsParser::SCREEN_HEIGHT_KEY = "ScreenHeight";
const std::string OptionsParser::FULLSCREEN_KEY = "Fullscreen";
const std::string OptionsParser::VERTICAL_FOV_KEY = "VerticalFieldOfView";
const std::string OptionsParser::H_SENSITIVITY_KEY = "HorizontalSensitivity";
const std::string OptionsParser::V_SENSITIVITY_KEY = "VerticalSensitivity";
const std::string OptionsParser::MUSIC_VOLUME_KEY = "MusicVolume";
const std::string OptionsParser::SOUND_VOLUME_KEY = "SoundVolume";
const std::string OptionsParser::SOUND_CHANNELS_KEY = "SoundChannels";
const std::string OptionsParser::MUSIC_FORMAT_KEY = "MusicFormat";
const std::string OptionsParser::SOUND_FORMAT_KEY = "SoundFormat";
const std::string OptionsParser::SKIP_INTRO_KEY = "SkipIntro";

std::map<std::string, std::string> OptionsParser::getPairs(const std::string &text)
{
	auto pairs = std::map<std::string, std::string>();

	std::istringstream iss(text);

	auto line = std::string();

	// Relevant parsing symbols.
	const char comment = '#';

	// Check each line in "text" for valid key/value pairs.
	while (std::getline(iss, line))
	{
		// Ignore comments and blank lines.
		if ((line.at(0) == comment) || (line.at(0) == '\r') || (line.at(0) == '\n'))
		{
			continue;
		}

		auto tokens = String::split(line, '=');
		Debug::check(tokens.size() == 2, "Options Parser", "Invalid entry \"" + line + "\".");

		// The strings could be trimmed of whitespace also, but I want the parser to be strict.
		auto key = tokens.at(0);
		auto value = String::trimLines(tokens.at(1));

		pairs.insert(std::pair<std::string, std::string>(key, value));
	}

	return pairs;
}

std::string OptionsParser::getValue(const std::map<std::string, std::string> &pairs,
	const std::string &key)
{
	Debug::check(pairs.find(key) != pairs.end(), "Options Parser",
		"Key \"" + key + "\" not found.");
	auto value = pairs.at(key);
	return value;
}

int OptionsParser::getInteger(const std::map<std::string, std::string> &pairs,
	const std::string &key)
{
	auto value = OptionsParser::getValue(pairs, key);
	return std::stoi(value);
}

double OptionsParser::getDouble(const std::map<std::string, std::string> &pairs,
	const std::string &key)
{
	auto value = OptionsParser::getValue(pairs, key);
	return std::stod(value);
}

bool OptionsParser::getBoolean(const std::map<std::string, std::string> &pairs,
	const std::string &key)
{
	auto value = OptionsParser::getValue(pairs, key);
	Debug::check(OptionsParserBooleans.find(value) != OptionsParserBooleans.end(),
		"Options Parser", "Invalid boolean value \"" + value + "\".");
	return OptionsParserBooleans.at(value);
}

MusicFormat OptionsParser::getMusicFormat(const std::map<std::string, std::string> &pairs)
{
	auto value = OptionsParser::getValue(pairs, OptionsParser::MUSIC_FORMAT_KEY);
	Debug::check(OptionsParserMusicFormats.find(value) != OptionsParserMusicFormats.end(),
		"Options Parser", "Invalid music format \"" + value + "\".");
	return OptionsParserMusicFormats.at(value);
}

SoundFormat OptionsParser::getSoundFormat(const std::map<std::string, std::string> &pairs)
{
	auto value = OptionsParser::getValue(pairs, OptionsParser::SOUND_FORMAT_KEY);
	Debug::check(OptionsParserSoundFormats.find(value) != OptionsParserSoundFormats.end(),
		"Options Parser", "Invalid sound format \"" + value + "\".");
	return OptionsParserSoundFormats.at(value);
}

std::unique_ptr<Options> OptionsParser::parse()
{
	// This parser is very simple right now. All text must have the exact amount
	// of spacing and commas, and there must be a new line at the end of the file.
	// Comment lines must have the comment symbol in the first column.

	auto fullPath = OptionsParser::PATH + OptionsParser::FILENAME;

	// Read the options file into a string.
	auto text = File::toString(fullPath);

	// Obtain all the key value pairs from the text.
	auto pairs = OptionsParser::getPairs(text);

	// Graphics.
	int screenWidth = OptionsParser::getInteger(pairs, OptionsParser::SCREEN_WIDTH_KEY);
	int screenHeight = OptionsParser::getInteger(pairs, OptionsParser::SCREEN_HEIGHT_KEY);
	bool fullscreen = OptionsParser::getBoolean(pairs, OptionsParser::FULLSCREEN_KEY);
	double verticalFOV = OptionsParser::getDouble(pairs, OptionsParser::VERTICAL_FOV_KEY);

	// Input.
	double hSensitivity = OptionsParser::getDouble(pairs, OptionsParser::H_SENSITIVITY_KEY);
	double vSensitivity = OptionsParser::getDouble(pairs, OptionsParser::V_SENSITIVITY_KEY);

	// Sound.
	auto musicVolume = OptionsParser::getDouble(pairs, OptionsParser::MUSIC_VOLUME_KEY);
	auto soundVolume = OptionsParser::getDouble(pairs, OptionsParser::SOUND_VOLUME_KEY);
	int soundChannels = OptionsParser::getInteger(pairs, OptionsParser::SOUND_CHANNELS_KEY);
	auto musicFormat = OptionsParser::getMusicFormat(pairs);
	auto soundFormat = OptionsParser::getSoundFormat(pairs);

	// Miscellaneous.
	bool skipIntro = OptionsParser::getBoolean(pairs, OptionsParser::SKIP_INTRO_KEY);

	auto options = std::unique_ptr<Options>(new Options(
		screenWidth, screenHeight, fullscreen, verticalFOV, hSensitivity, vSensitivity,
		musicVolume, soundVolume, soundChannels, musicFormat, soundFormat, skipIntro));

	return options;
}

void OptionsParser::save(const Options &options)
{
	// not done!
}
