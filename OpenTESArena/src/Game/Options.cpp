#include <algorithm>
#include <fstream>
#include <sstream>

#include "Options.h"
#include "../Utilities/Debug.h"
#include "../Utilities/KeyValueMap.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

namespace
{
	// Supported value types by the parser.
	enum class OptionType { Bool, Int, Double, String };

	// Mappings of key names to their associated type for each section. These use vectors
	// of pairs instead of hash tables to maintain ordering.
	const std::vector<std::pair<std::string, OptionType>> GraphicsMappings =
	{
		{ "ScreenWidth", OptionType::Int },
		{ "ScreenHeight", OptionType::Int },
		{ "Fullscreen", OptionType::Bool },
		{ "TargetFPS", OptionType::Int },
		{ "ResolutionScale", OptionType::Double },
		{ "VerticalFOV", OptionType::Double },
		{ "LetterboxMode", OptionType::Int },
		{ "CursorScale", OptionType::Double },
		{ "ModernInterface", OptionType::Bool },
		{ "RenderThreadsMode", OptionType::Int }
	};

	const std::vector<std::pair<std::string, OptionType>> AudioMappings =
	{
		{ "MusicVolume", OptionType::Double },
		{ "SoundVolume", OptionType::Double },
		{ "MidiConfig", OptionType::String },
		{ "SoundChannels", OptionType::Int },
		{ "SoundResampling", OptionType::Int }
	};

	const std::vector<std::pair<std::string, OptionType>> InputMappings =
	{
		{ "HorizontalSensitivity", OptionType::Double },
		{ "VerticalSensitivity", OptionType::Double },
		{ "CameraPitchLimit", OptionType::Double }
	};

	const std::vector<std::pair<std::string, OptionType>> MiscMappings =
	{
		{ "ArenaPath", OptionType::String },
		{ "ArenaSavesPath", OptionType::String },
		{ "Collision", OptionType::Bool },
		{ "SkipIntro", OptionType::Bool },
		{ "ShowDebug", OptionType::Bool },
		{ "ShowCompass", OptionType::Bool },
		{ "TimeScale", OptionType::Double }
	};
}

// The "default" options file is shipped with releases, and it resides in the options 
// folder on the base path. The "changes" options file is copied to the user's options 
// folder (preferably by a wizard, otherwise by the user), and it contains changes to 
// settings in the default file.
const std::string Options::DEFAULT_FILENAME = "options-default.txt";
const std::string Options::CHANGES_FILENAME = "options-changes.txt";

const std::string Options::SECTION_GRAPHICS = "Graphics";
const std::string Options::SECTION_INPUT = "Input";
const std::string Options::SECTION_AUDIO = "Audio";
const std::string Options::SECTION_MISC = "Misc";

const int Options::MIN_FPS = 15;
const double Options::MIN_RESOLUTION_SCALE = 0.10;
const double Options::MAX_RESOLUTION_SCALE = 1.0;
const double Options::MIN_VERTICAL_FOV = 40.0;
const double Options::MAX_VERTICAL_FOV = 150.0;
const double Options::MIN_CURSOR_SCALE = 0.50;
const double Options::MAX_CURSOR_SCALE = 8.0;
const int Options::MIN_LETTERBOX_MODE = 0;
const int Options::MAX_LETTERBOX_MODE = 2;
const int Options::MIN_RENDER_THREADS_MODE = 0;
const int Options::MAX_RENDER_THREADS_MODE = 3;
const double Options::MIN_HORIZONTAL_SENSITIVITY = 0.50;
const double Options::MAX_HORIZONTAL_SENSITIVITY = 50.0;
const double Options::MIN_VERTICAL_SENSITIVITY = 0.50;
const double Options::MAX_VERTICAL_SENSITIVITY = 50.0;
const double Options::MIN_CAMERA_PITCH_LIMIT = 0.0;
const double Options::MAX_CAMERA_PITCH_LIMIT = 85.0;
const double Options::MIN_VOLUME = 0.0;
const double Options::MAX_VOLUME = 1.0;
const int Options::MIN_SOUND_CHANNELS = 1;
const int Options::RESAMPLING_OPTION_COUNT = 4;
const double Options::MIN_TIME_SCALE = 0.50;
const double Options::MAX_TIME_SCALE = 1.0;

void Options::load(const std::string &filename,
	std::unordered_map<std::string, Options::MapGroup> &maps)
{
	// Read the key-value pairs from each section in the given options file.
	const KeyValueMap keyValueMap(filename);

	for (const auto &sectionPair : keyValueMap.getAll())
	{
		const std::string &section = sectionPair.first;
		const KeyValueMap::SectionMap &sectionMap = sectionPair.second;

		// Get the list of key-type pairs to pull from.
		const auto &keyList = [&filename, &section]()
		{
			if (section == Options::SECTION_GRAPHICS)
			{
				return GraphicsMappings;
			}
			else if (section == Options::SECTION_INPUT)
			{
				return InputMappings;
			}
			else if (section == Options::SECTION_AUDIO)
			{
				return AudioMappings;
			}
			else if (section == Options::SECTION_MISC)
			{
				return MiscMappings;
			}
			else
			{
				throw DebugException("Unrecognized section \"" +
					section + "\" in " + filename);
			}
		}();

		for (const auto &pair : sectionMap)
		{
			// See if the key is recognized, and if so, see what type the value should be, 
			// convert it, and place it in the changed map.
			const std::string &key = pair.first;
			const auto keyListIter = std::find_if(keyList.begin(), keyList.end(),
				[&key](const std::pair<std::string, OptionType> &keyTypePair)
			{
				return keyTypePair.first == key;
			});

			if (keyListIter != keyList.end())
			{
				const OptionType type = keyListIter->second;
				auto groupIter = maps.find(section);

				// Add an empty map group if the section is new.
				if (groupIter == maps.end())
				{
					groupIter = maps.insert(std::make_pair(
						section, Options::MapGroup())).first;
				}

				Options::MapGroup &mapGroup = groupIter->second;

				// Using KeyValueMap's getter code here for convenience, despite it doing
				// an unnecessary look-up.
				if (type == OptionType::Bool)
				{
					mapGroup.bools.insert(std::make_pair(
						key, keyValueMap.getBoolean(section, key)));
				}
				else if (type == OptionType::Int)
				{
					mapGroup.integers.insert(std::make_pair(
						key, keyValueMap.getInteger(section, key)));
				}
				else if (type == OptionType::Double)
				{
					mapGroup.doubles.insert(std::make_pair(
						key, keyValueMap.getDouble(section, key)));
				}
				else if (type == OptionType::String)
				{
					mapGroup.strings.insert(std::make_pair(
						key, keyValueMap.getString(section, key)));
				}
			}
			else
			{
				DebugWarning("Key \"" + key + "\" not recognized in " + filename + ".");
			}
		}
	}
}

bool Options::getBool(const std::string &section, const std::string &key) const
{
	auto getValuePtr = [](const std::string &section, const std::string &key,
		const std::unordered_map<std::string, Options::MapGroup> &sectionMap) -> const bool*
	{
		// Check that the section map exists.
		const auto sectionIter = sectionMap.find(section);
		if (sectionIter != sectionMap.end())
		{
			// Check that the key exists.
			const Options::BoolMap &boolMap = sectionIter->second.bools;
			const auto valueIter = boolMap.find(key);
			return (valueIter != boolMap.end()) ? &valueIter->second : nullptr;
		}
		else
		{
			return nullptr;
		}
	};

	// Check the changed map first, then the default map.
	const bool *changedValue = getValuePtr(section, key, this->changedMaps);
	if (changedValue != nullptr)
	{
		return *changedValue;
	}
	else
	{
		const bool *defaultValue = getValuePtr(section, key, this->defaultMaps);
		if (defaultValue != nullptr)
		{
			return *defaultValue;
		}
		else
		{
			throw DebugException("Boolean \"" + key +
				"\" (section \"" + section + "\") not in options.");
		}
	}
}

int Options::getInt(const std::string &section, const std::string &key) const
{
	auto getValuePtr = [](const std::string &section, const std::string &key,
		const std::unordered_map<std::string, Options::MapGroup> &sectionMap) -> const int*
	{
		// Check that the section map exists.
		const auto sectionIter = sectionMap.find(section);
		if (sectionIter != sectionMap.end())
		{
			// Check that the key exists.
			const Options::IntegerMap &integerMap = sectionIter->second.integers;
			const auto valueIter = integerMap.find(key);
			return (valueIter != integerMap.end()) ? &valueIter->second : nullptr;
		}
		else
		{
			return nullptr;
		}
	};

	// Check the changed map first, then the default map.
	const int *changedValue = getValuePtr(section, key, this->changedMaps);
	if (changedValue != nullptr)
	{
		return *changedValue;
	}
	else
	{
		const int *defaultValue = getValuePtr(section, key, this->defaultMaps);
		if (defaultValue != nullptr)
		{
			return *defaultValue;
		}
		else
		{
			throw DebugException("Integer \"" + key +
				"\" (section \"" + section + "\") not in options.");
		}
	}
}

double Options::getDouble(const std::string &section, const std::string &key) const
{
	auto getValuePtr = [](const std::string &section, const std::string &key,
		const std::unordered_map<std::string, Options::MapGroup> &sectionMap) -> const double*
	{
		// Check that the section map exists.
		const auto sectionIter = sectionMap.find(section);
		if (sectionIter != sectionMap.end())
		{
			// Check that the key exists.
			const Options::DoubleMap &doubleMap = sectionIter->second.doubles;
			const auto valueIter = doubleMap.find(key);
			return (valueIter != doubleMap.end()) ? &valueIter->second : nullptr;
		}
		else
		{
			return nullptr;
		}
	};

	// Check the changed map first, then the default map.
	const double *changedValue = getValuePtr(section, key, this->changedMaps);
	if (changedValue != nullptr)
	{
		return *changedValue;
	}
	else
	{
		const double *defaultValue = getValuePtr(section, key, this->defaultMaps);
		if (defaultValue != nullptr)
		{
			return *defaultValue;
		}
		else
		{
			throw DebugException("Double \"" + key +
				"\" (section \"" + section + "\") not in options.");
		}
	}
}

const std::string &Options::getString(const std::string &section, const std::string &key) const
{
	auto getValuePtr = [](const std::string &section, const std::string &key,
		const std::unordered_map<std::string, Options::MapGroup> &sectionMap) -> const std::string*
	{
		// Check that the section map exists.
		const auto sectionIter = sectionMap.find(section);
		if (sectionIter != sectionMap.end())
		{
			// Check that the key exists.
			const Options::StringMap &stringMap = sectionIter->second.strings;
			const auto valueIter = stringMap.find(key);
			return (valueIter != stringMap.end()) ? &valueIter->second : nullptr;
		}
		else
		{
			return nullptr;
		}
	};

	// Check the changed map first, then the default map.
	const std::string *changedValue = getValuePtr(section, key, this->changedMaps);
	if (changedValue != nullptr)
	{
		return *changedValue;
	}
	else
	{
		const std::string *defaultValue = getValuePtr(section, key, this->defaultMaps);
		if (defaultValue != nullptr)
		{
			return *defaultValue;
		}
		else
		{
			throw DebugException("String \"" + key +
				"\" (section \"" + section + "\") not in options.");
		}
	}
}

void Options::setBool(const std::string &section, const std::string &key, bool value)
{
	// Check that the section map exists. If not, add it.
	auto sectionIter = this->changedMaps.find(section);
	if (sectionIter == this->changedMaps.end())
	{
		sectionIter = this->changedMaps.insert(
			std::make_pair(section, Options::MapGroup())).first;
	}

	Options::BoolMap &sectionMap = sectionIter->second.bools;
	
	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.insert(std::make_pair(key, value)).first;
	}

	iter->second = value;
}

void Options::setInt(const std::string &section, const std::string &key, int value)
{
	// Check that the section map exists. If not, add it.
	auto sectionIter = this->changedMaps.find(section);
	if (sectionIter == this->changedMaps.end())
	{
		sectionIter = this->changedMaps.insert(
			std::make_pair(section, Options::MapGroup())).first;
	}

	Options::IntegerMap &sectionMap = sectionIter->second.integers;

	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.insert(std::make_pair(key, value)).first;
	}

	iter->second = value;
}

void Options::setDouble(const std::string &section, const std::string &key, double value)
{
	// Check that the section map exists. If not, add it.
	auto sectionIter = this->changedMaps.find(section);
	if (sectionIter == this->changedMaps.end())
	{
		sectionIter = this->changedMaps.insert(
			std::make_pair(section, Options::MapGroup())).first;
	}

	Options::DoubleMap &sectionMap = sectionIter->second.doubles;

	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.insert(std::make_pair(key, value)).first;
	}

	iter->second = value;
}

void Options::setString(const std::string &section, const std::string &key,
	const std::string &value)
{
	// Check that the section map exists. If not, add it.
	auto sectionIter = this->changedMaps.find(section);
	if (sectionIter == this->changedMaps.end())
	{
		sectionIter = this->changedMaps.insert(
			std::make_pair(section, Options::MapGroup())).first;
	}

	Options::StringMap &sectionMap = sectionIter->second.strings;

	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.insert(std::make_pair(key, value)).first;
	}

	iter->second = value;
}

void Options::checkGraphics_ScreenWidth(int value) const
{
	DebugAssert(value > 0, "Screen width must be positive.");
}

void Options::checkGraphics_ScreenHeight(int value) const
{
	DebugAssert(value > 0, "Screen height must be positive.");
}

void Options::checkGraphics_TargetFPS(int value) const
{
	DebugAssert(value >= Options::MIN_FPS, "Target FPS cannot be less than " +
		std::to_string(Options::MIN_FPS) + ".");
}

void Options::checkGraphics_ResolutionScale(double value) const
{
	DebugAssert(value >= Options::MIN_RESOLUTION_SCALE,
		"Resolution scale cannot be less than " +
		String::fixedPrecision(Options::MIN_RESOLUTION_SCALE, 2) + ".");
	DebugAssert(value <= Options::MAX_RESOLUTION_SCALE,
		"Resolution scale cannot be greater than " +
		String::fixedPrecision(Options::MAX_RESOLUTION_SCALE, 2) + ".");
}

void Options::checkGraphics_VerticalFOV(double value) const
{
	DebugAssert(value >= Options::MIN_VERTICAL_FOV, "Vertical FOV cannot be less than " +
		String::fixedPrecision(Options::MIN_VERTICAL_FOV, 1) + ".");
	DebugAssert(value <= Options::MAX_VERTICAL_FOV, "Vertical FOV cannot be greater than " +
		String::fixedPrecision(Options::MAX_VERTICAL_FOV, 1) + ".");
}

void Options::checkGraphics_LetterboxMode(int value) const
{
	DebugAssert(value >= Options::MIN_LETTERBOX_MODE, "Letterbox mode cannot be less than " +
		std::to_string(Options::MIN_LETTERBOX_MODE) + ".");
	DebugAssert(value <= Options::MAX_LETTERBOX_MODE, "Letterbox mode cannot be greater than " +
		std::to_string(Options::MAX_LETTERBOX_MODE) + ".");
}

void Options::checkGraphics_CursorScale(double value) const
{
	DebugAssert(value >= Options::MIN_CURSOR_SCALE,
		"Cursor scale cannot be less than " +
		String::fixedPrecision(Options::MIN_CURSOR_SCALE, 1) + ".");
	DebugAssert(value <= Options::MAX_CURSOR_SCALE,
		"Cursor scale cannot be greater than " +
		String::fixedPrecision(Options::MAX_CURSOR_SCALE, 1) + ".");
}

void Options::checkGraphics_RenderThreadsMode(int value) const
{
	DebugAssert(value >= Options::MIN_RENDER_THREADS_MODE,
		"Render threads mode cannot be less than " +
		std::to_string(Options::MIN_RENDER_THREADS_MODE) + ".");
	DebugAssert(value <= Options::MAX_RENDER_THREADS_MODE,
		"Render threads mode cannot be greater than " +
		std::to_string(Options::MAX_RENDER_THREADS_MODE) + ".");
}

void Options::checkAudio_MusicVolume(double value) const
{
	DebugAssert(value >= Options::MIN_VOLUME, "Music volume cannot be negative.");
	DebugAssert(value <= Options::MAX_VOLUME, "Music volume cannot be greater than " +
		String::fixedPrecision(Options::MAX_VOLUME, 1) + ".");
}

void Options::checkAudio_SoundVolume(double value) const
{
	DebugAssert(value >= Options::MIN_VOLUME, "Sound volume cannot be negative.");
	DebugAssert(value <= Options::MAX_VOLUME, "Sound volume cannot be greater than " +
		String::fixedPrecision(Options::MAX_VOLUME, 1) + ".");
}

void Options::checkAudio_SoundChannels(int value) const
{
	DebugAssert(value >= Options::MIN_SOUND_CHANNELS, "Sound channel count cannot be less than " +
		std::to_string(Options::MIN_SOUND_CHANNELS) + ".");
}

void Options::checkAudio_SoundResampling(int value) const
{
	DebugAssert(value >= 0, "Sound resampling value cannot be negative.");
	DebugAssert(value < Options::RESAMPLING_OPTION_COUNT,
		"Sound resampling value cannot be greater than " +
		std::to_string(Options::RESAMPLING_OPTION_COUNT - 1) + ".");
}

void Options::checkInput_HorizontalSensitivity(double value) const
{
	DebugAssert(value >= Options::MIN_HORIZONTAL_SENSITIVITY,
		"Horizontal sensitivity cannot be less than " +
		String::fixedPrecision(Options::MIN_HORIZONTAL_SENSITIVITY, 1) + ".");
	DebugAssert(value <= Options::MAX_HORIZONTAL_SENSITIVITY,
		"Horizontal sensitivity cannot be greater than " +
		String::fixedPrecision(Options::MAX_HORIZONTAL_SENSITIVITY, 1) + ".");
}

void Options::checkInput_VerticalSensitivity(double value) const
{
	DebugAssert(value >= Options::MIN_VERTICAL_SENSITIVITY,
		"Vertical sensitivity cannot be less than " +
		String::fixedPrecision(Options::MIN_VERTICAL_SENSITIVITY, 1) + ".");
	DebugAssert(value <= Options::MAX_VERTICAL_SENSITIVITY,
		"Vertical sensitivity cannot be greater than " +
		String::fixedPrecision(Options::MAX_VERTICAL_SENSITIVITY, 1) + ".");
}

void Options::checkInput_CameraPitchLimit(double value) const
{
	DebugAssert(value >= Options::MIN_CAMERA_PITCH_LIMIT,
		"Camera pitch limit cannot be less than " +
		String::fixedPrecision(Options::MIN_CAMERA_PITCH_LIMIT, 1) + ".");
	DebugAssert(value <= Options::MAX_CAMERA_PITCH_LIMIT,
		"Camera pitch limit cannot be greater than " +
		String::fixedPrecision(Options::MAX_CAMERA_PITCH_LIMIT, 1) + ".");
}

void Options::checkMisc_TimeScale(double value) const
{
	DebugAssert(value >= Options::MIN_TIME_SCALE,
		"Time scale cannot be less than " +
		String::fixedPrecision(Options::MIN_TIME_SCALE, 1) + ".");
	DebugAssert(value <= Options::MAX_TIME_SCALE,
		"Time scale cannot be greater than " +
		String::fixedPrecision(Options::MAX_TIME_SCALE, 1) + ".");
}

void Options::loadDefaults(const std::string &filename)
{
	DebugMention("Reading defaults \"" + filename + "\".");

	Options::load(filename, this->defaultMaps);
}

void Options::loadChanges(const std::string &filename)
{
	DebugMention("Reading changes \"" + filename + "\".");

	Options::load(filename, this->changedMaps);
}

void Options::saveChanges()
{
	const std::string optionsPath = Platform::getOptionsPath();
	const std::string filename(optionsPath + Options::CHANGES_FILENAME);

	// Create options directory if it doesn't exist.
	if (!Platform::directoryExists(optionsPath))
	{
		Platform::createDirectoryRecursively(optionsPath);
	}

	std::ofstream ofs(filename);

	if (ofs.is_open())
	{
		// Writes out all key-value pairs in a section if it exists.
		auto tryWriteSection = [this, &ofs](const std::string &section,
			const std::vector<std::pair<std::string, OptionType>> &keyList)
		{
			const auto sectionIter = this->changedMaps.find(section);
			if (sectionIter != this->changedMaps.end())
			{
				const auto &mapGroup = sectionIter->second;

				// Print section line.
				ofs << KeyValueMap::SECTION_FRONT << section << KeyValueMap::SECTION_BACK << '\n';

				// Write all pairs present in the current section.
				for (const auto &pair : keyList)
				{
					const std::string &key = pair.first;
					const OptionType type = pair.second;

					auto writePair = [&ofs, &key](const std::string &value)
					{
						ofs << key << KeyValueMap::PAIR_SEPARATOR << value << '\n';
					};

					// If the associated changed map has the key, print the key-value pair.
					if (type == OptionType::Bool)
					{
						const auto iter = mapGroup.bools.find(key);
						if (iter != mapGroup.bools.end())
						{
							const bool value = iter->second;
							writePair(value ? "true" : "false");
						}
					}
					else if (type == OptionType::Int)
					{
						const auto iter = mapGroup.integers.find(key);
						if (iter != mapGroup.integers.end())
						{
							const int value = iter->second;
							writePair(std::to_string(value));
						}
					}
					else if (type == OptionType::Double)
					{
						const auto iter = mapGroup.doubles.find(key);
						if (iter != mapGroup.doubles.end())
						{
							const double value = iter->second;
							std::stringstream ss;
							ss << value;
							writePair(ss.str());
						}
					}
					else if (type == OptionType::String)
					{
						const auto iter = mapGroup.strings.find(key);
						if (iter != mapGroup.strings.end())
						{
							const std::string &value = iter->second;
							writePair(value);
						}
					}
					else
					{
						throw DebugException("Bad option type \"" +
							std::to_string(static_cast<int>(type)) + "\".");
					}
				}

				ofs << '\n';
			}
		};

		ofs << "# Changed options file for OpenTESArena. This is where the program" << '\n' <<
			"# saves options that differ from the defaults." << '\n';

		ofs << '\n';

		// Write out each section in a strict order.
		tryWriteSection(Options::SECTION_GRAPHICS, GraphicsMappings);
		tryWriteSection(Options::SECTION_AUDIO, AudioMappings);
		tryWriteSection(Options::SECTION_INPUT, InputMappings);
		tryWriteSection(Options::SECTION_MISC, MiscMappings);

		DebugMention("Saved settings in \"" + filename + "\".");
	}
	else
	{
		throw DebugException("Could not save to \"" + filename + "\".");
	}
}
