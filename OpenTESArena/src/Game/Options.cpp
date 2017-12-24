#include <cassert>
#include <fstream>

#include "Options.h"
#include "../Utilities/Debug.h"
#include "../Utilities/KeyValueMap.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

namespace
{
	// Supported value types by the parser.
	enum class OptionType { Bool, Int, Double, String };

	// Mappings of option names to key names and their associated type. The key names
	// are for the parser, and the types are to prevent type errors when accessing
	// elements.
	const std::unordered_map<std::string, std::pair<OptionName, OptionType>> OptionMappings =
	{
		{ "ScreenWidth", { OptionName::ScreenWidth, OptionType::Int } },
		{ "ScreenHeight", { OptionName::ScreenHeight, OptionType::Int } },
		{ "Fullscreen", { OptionName::Fullscreen, OptionType::Bool } },
		{ "TargetFPS", { OptionName::TargetFPS, OptionType::Int } },
		{ "ResolutionScale", { OptionName::ResolutionScale, OptionType::Double } },
		{ "VerticalFieldOfView", { OptionName::VerticalFOV, OptionType::Double } },
		{ "LetterboxAspect", { OptionName::LetterboxAspect, OptionType::Double } },
		{ "CursorScale", { OptionName::CursorScale, OptionType::Double } },
		{ "ModernInterface", { OptionName::ModernInterface, OptionType::Bool } },

		{ "HorizontalSensitivity", { OptionName::HorizontalSensitivity, OptionType::Double } },
		{ "VerticalSensitivity", { OptionName::VerticalSensitivity, OptionType::Double } },

		{ "MusicVolume", { OptionName::MusicVolume, OptionType::Double } },
		{ "SoundVolume", { OptionName::SoundVolume, OptionType::Double } },
		{ "MidiConfig", { OptionName::MidiConfig, OptionType::String } },
		{ "SoundChannels", { OptionName::SoundChannels, OptionType::Int } },

		{ "ArenaPath", { OptionName::ArenaPath, OptionType::String } },
		{ "Collision", { OptionName::Collision, OptionType::Bool } },
		{ "SkipIntro", { OptionName::SkipIntro, OptionType::Bool } },
		{ "ShowDebug", { OptionName::ShowDebug, OptionType::Bool } }
	};
}

// The "default" options file is shipped with releases, and it resides in the options 
// folder on the base path. The "changes" options file is copied to the user's prefs 
// folder (preferably by a wizard, otherwise by the user), and it contains changes to 
// settings in the default file.
const std::string Options::DEFAULT_FILENAME = "options-default.txt";
const std::string Options::CHANGES_FILENAME = "options-changes.txt";

const int Options::MIN_FPS = 15;
const double Options::MIN_RESOLUTION_SCALE = 0.10;
const double Options::MAX_RESOLUTION_SCALE = 1.0;
const double Options::MIN_VERTICAL_FOV = 40.0;
const double Options::MAX_VERTICAL_FOV = 150.0;
const double Options::MIN_CURSOR_SCALE = 0.50;
const double Options::MAX_CURSOR_SCALE = 8.0;
const double Options::MIN_LETTERBOX_ASPECT = 0.75;
const double Options::MAX_LETTERBOX_ASPECT = 3.0;
const double Options::MIN_HORIZONTAL_SENSITIVITY = 0.50;
const double Options::MAX_HORIZONTAL_SENSITIVITY = 50.0;
const double Options::MIN_VERTICAL_SENSITIVITY = 0.50;
const double Options::MAX_VERTICAL_SENSITIVITY = 50.0;
const double Options::MIN_VOLUME = 0.0;
const double Options::MAX_VOLUME = 1.0;

Options::Options()
{
	const std::string defaultPath(Platform::getBasePath() +
		"options/" + Options::DEFAULT_FILENAME);
	DebugMention("Reading \"" + defaultPath + "\".");

	// Read the key-value pairs from options-default.txt.
	const KeyValueMap keyValueMap(defaultPath);

	for (const auto &pair : keyValueMap.getAll())
	{
		const std::string &key = pair.first;

		// See if the key is recognized, and if so, see what type the value should be, 
		// convert it, and place it in the default map.
		const auto mapIter = OptionMappings.find(key);

		if (mapIter != OptionMappings.end())
		{
			const OptionName name = mapIter->second.first;
			const OptionType type = mapIter->second.second;

			// (Using KeyValueMap's getter code here for convenience, despite it doing an
			// unnecessary look-up).
			if (type == OptionType::Bool)
			{
				this->defaultBools.insert(std::make_pair(name, keyValueMap.getBoolean(key)));
			}
			else if (type == OptionType::Int)
			{
				this->defaultInts.insert(std::make_pair(name, keyValueMap.getInteger(key)));
			}
			else if (type == OptionType::Double)
			{
				this->defaultDoubles.insert(std::make_pair(name, keyValueMap.getDouble(key)));
			}
			else if (type == OptionType::String)
			{
				this->defaultStrings.insert(std::make_pair(name, keyValueMap.getString(key)));
			}
		}
		else
		{
			DebugMention("Key \"" + key + "\" not recognized in \"" + defaultPath + "\".");
		}
	}
}

Options::~Options()
{

}

const std::string &getOptionsNameString(OptionName key)
{
	// Find the key given the value.
	for (const auto &pair : OptionMappings)
	{
		const OptionName name = pair.second.first;

		if (name == key)
		{
			return pair.first;
		}
	}

	// Programmer error if this is reached (means OptionMappings is out-of-date).
	throw std::runtime_error("Key \"" +
		std::to_string(static_cast<int>(key)) + "\" not in OptionMappings.");
}

template <typename T>
const T &get(OptionName key, const std::unordered_map<OptionName, T> &defaultMap,
	const std::unordered_map<OptionName, T> &changedMap)
{
	// The "changed" map is accessed first, and if the value isn't in that, it must be
	// in the default map, otherwise error.
	const auto changedIter = changedMap.find(key);

	if (changedIter != changedMap.end())
	{
		return changedIter->second;
	}
	else
	{
		const auto defaultIter = defaultMap.find(key);

		if (defaultIter != defaultMap.end())
		{
			return defaultIter->second;
		}
		else
		{
			const std::string &keyString = getOptionsNameString(key);
			throw std::runtime_error("Key \"" + keyString + "\" missing from default options.");
		}
	}
}

bool Options::getBool(OptionName key) const
{
	return get(key, this->defaultBools, this->changedBools);
}

int Options::getInt(OptionName key) const
{
	return get(key, this->defaultInts, this->changedInts);
}

double Options::getDouble(OptionName key) const
{
	return get(key, this->defaultDoubles, this->changedDoubles);
}

const std::string &Options::getString(OptionName key) const
{
	return get(key, this->defaultStrings, this->changedStrings);
}

template <typename T>
void set(OptionName key, const T &value,
	std::unordered_map<OptionName, T> &changedMap)
{
	// All options changed at runtime go into the "changed" map which is saved separately 
	// from the default map.
	const auto iter = changedMap.find(key);

	if (iter != changedMap.end())
	{
		iter->second = value;
	}
	else
	{
		changedMap.insert(std::make_pair(key, value));
	}
}

void Options::setBool(OptionName key, bool value)
{
	set(key, value, this->changedBools);
}

void Options::setInt(OptionName key, int value)
{
	set(key, value, this->changedInts);
}

void Options::setDouble(OptionName key, double value)
{
	set(key, value, this->changedDoubles);
}

void Options::setString(OptionName key, const std::string &value)
{
	set(key, value, this->changedStrings);
}

void Options::checkScreenWidth(int value) const
{
	DebugAssert(value > 0, "Screen width must be positive.");
}

void Options::checkScreenHeight(int value) const
{
	DebugAssert(value > 0, "Screen height must be positive.");
}

void Options::checkTargetFPS(int value) const
{
	DebugAssert(value >= Options::MIN_FPS, "Target FPS cannot be less than " +
		std::to_string(Options::MIN_FPS) + ".");
}

void Options::checkResolutionScale(double value) const
{
	DebugAssert(value >= Options::MIN_RESOLUTION_SCALE,
		"Resolution scale cannot be less than " +
		String::fixedPrecision(Options::MIN_RESOLUTION_SCALE, 2) + ".");
	DebugAssert(value <= Options::MAX_RESOLUTION_SCALE,
		"Resolution scale cannot be greater than " +
		String::fixedPrecision(Options::MAX_RESOLUTION_SCALE, 2) + ".");
}

void Options::checkVerticalFOV(double value) const
{
	DebugAssert(value >= Options::MIN_VERTICAL_FOV, "Vertical FOV cannot be less than " +
		String::fixedPrecision(Options::MIN_VERTICAL_FOV, 1) + ".");
	DebugAssert(value <= Options::MAX_VERTICAL_FOV, "Vertical FOV cannot be greater than " +
		String::fixedPrecision(Options::MAX_VERTICAL_FOV, 1) + ".");
}

void Options::checkLetterboxAspect(double value) const
{
	DebugAssert(value >= Options::MIN_LETTERBOX_ASPECT,
		"Letterbox aspect cannot be less than " +
		String::fixedPrecision(Options::MIN_LETTERBOX_ASPECT, 2) + ".");
	DebugAssert(value <= Options::MAX_LETTERBOX_ASPECT,
		"Letterbox aspect cannot be greater than " +
		String::fixedPrecision(Options::MAX_LETTERBOX_ASPECT, 2) + ".");
}

void Options::checkCursorScale(double value) const
{
	DebugAssert(value >= Options::MIN_CURSOR_SCALE,
		"Cursor scale cannot be less than " +
		String::fixedPrecision(Options::MIN_CURSOR_SCALE, 1) + ".");
	DebugAssert(value <= Options::MAX_CURSOR_SCALE,
		"Cursor scale cannot be greater than " +
		String::fixedPrecision(Options::MAX_CURSOR_SCALE, 1) + ".");
}

void Options::checkHorizontalSensitivity(double value) const
{
	DebugAssert(value >= Options::MIN_HORIZONTAL_SENSITIVITY,
		"Horizontal sensitivity cannot be less than " +
		String::fixedPrecision(Options::MIN_HORIZONTAL_SENSITIVITY, 1) + ".");
	DebugAssert(value <= Options::MAX_HORIZONTAL_SENSITIVITY,
		"Horizontal sensitivity cannot be greater than " +
		String::fixedPrecision(Options::MAX_HORIZONTAL_SENSITIVITY, 1) + ".");
}

void Options::checkVerticalSensitivity(double value) const
{
	DebugAssert(value >= Options::MIN_VERTICAL_SENSITIVITY,
		"Vertical sensitivity cannot be less than " +
		String::fixedPrecision(Options::MIN_VERTICAL_SENSITIVITY, 1) + ".");
	DebugAssert(value <= Options::MAX_VERTICAL_SENSITIVITY,
		"Vertical sensitivity cannot be greater than " +
		String::fixedPrecision(Options::MAX_VERTICAL_SENSITIVITY, 1) + ".");
}

void Options::checkMusicVolume(double value) const
{
	DebugAssert(value >= Options::MIN_VOLUME, "Music volume cannot be negative.");
	DebugAssert(value <= Options::MAX_VOLUME, "Music volume cannot be greater than " +
		String::fixedPrecision(Options::MAX_VOLUME, 1) + ".");
}

void Options::checkSoundVolume(double value) const
{
	DebugAssert(value >= Options::MIN_VOLUME, "Sound volume cannot be negative.");
	DebugAssert(value <= Options::MAX_VOLUME, "Sound volume cannot be greater than " +
		String::fixedPrecision(Options::MAX_VOLUME, 1) + ".");
}

void Options::checkSoundChannels(int value) const
{
	DebugAssert(value >= 1, "Sound channel count must be positive.");
}

void Options::load(const std::string &filename)
{
	DebugMention("Reading \"" + filename + "\".");

	// Read the key-value pairs from the given options file.
	const KeyValueMap keyValueMap(filename);

	for (const auto &pair : keyValueMap.getAll())
	{
		const std::string &key = pair.first;

		// See if the key is recognized, and if so, see what type the value should be, 
		// convert it, and place it in the changed map.
		const auto mapIter = OptionMappings.find(key);

		if (mapIter != OptionMappings.end())
		{
			const OptionName name = mapIter->second.first;
			const OptionType type = mapIter->second.second;

			// (Using KeyValueMap's getter code here for convenience, despite it doing an
			// unnecessary look-up).
			if (type == OptionType::Bool)
			{
				this->changedBools.insert(std::make_pair(name, keyValueMap.getBoolean(key)));
			}
			else if (type == OptionType::Int)
			{
				this->changedInts.insert(std::make_pair(name, keyValueMap.getInteger(key)));
			}
			else if (type == OptionType::Double)
			{
				this->changedDoubles.insert(std::make_pair(name, keyValueMap.getDouble(key)));
			}
			else if (type == OptionType::String)
			{
				this->changedStrings.insert(std::make_pair(name, keyValueMap.getString(key)));
			}
		}
		else
		{
			DebugMention("Key \"" + key + "\" not recognized in \"" + filename + "\".");
		}
	}
}

void Options::saveChanges()
{
	const std::string filename(Platform::getOptionsPath() + Options::CHANGES_FILENAME);

	std::ofstream ofs(filename);

	if (ofs.is_open())
	{
		ofs << "# \"Changed\" options file for OpenTESArena. This is where the program" << '\n' <<
			"# saves options that differ from the defaults." << '\n';

		ofs << '\n';

		// Save each option to the "changes" options file.
		// - To do: order these by their appearance in options-default.txt.
		for (const auto &pair : this->changedBools)
		{
			const std::string &nameString = getOptionsNameString(pair.first);
			ofs << nameString << '=' << (pair.second ? "true" : "false") << '\n';
		}

		ofs << '\n';

		for (const auto &pair : this->changedInts)
		{
			const std::string &nameString = getOptionsNameString(pair.first);
			ofs << nameString << '=' << std::to_string(pair.second) << '\n';
		}

		ofs << '\n';

		for (const auto &pair : this->changedDoubles)
		{
			const std::string &nameString = getOptionsNameString(pair.first);
			ofs << nameString << '=' << String::fixedPrecision(pair.second, 4) << '\n';
		}

		ofs << '\n';

		for (const auto &pair : this->changedStrings)
		{
			const std::string &nameString = getOptionsNameString(pair.first);
			ofs << nameString << '=' << pair.second << '\n';
		}

		ofs << '\n';

		DebugMention("Saved settings in \"" + filename + "\".");
	}
	else
	{
		throw std::runtime_error("Could not save to \"" + filename + "\".");
	}
}
