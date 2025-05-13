#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "Options.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/Directory.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/String.h"

namespace
{
	// Key and parsing type pairs. These use arrays of pairs instead of hash tables to maintain ordering.
	constexpr std::pair<const char*, OptionType> GraphicsMappings[] =
	{
		{ Options::Key_Graphics_ScreenWidth, Options::OptionType_Graphics_ScreenWidth },
		{ Options::Key_Graphics_ScreenHeight, Options::OptionType_Graphics_ScreenHeight },
		{ Options::Key_Graphics_WindowMode, Options::OptionType_Graphics_WindowMode },
		{ Options::Key_Graphics_TargetFPS, Options::OptionType_Graphics_TargetFPS },
		{ Options::Key_Graphics_ResolutionScale, Options::OptionType_Graphics_ResolutionScale },
		{ Options::Key_Graphics_VerticalFOV, Options::OptionType_Graphics_VerticalFOV },
		{ Options::Key_Graphics_LetterboxMode, Options::OptionType_Graphics_LetterboxMode },
		{ Options::Key_Graphics_CursorScale, Options::OptionType_Graphics_CursorScale },
		{ Options::Key_Graphics_ModernInterface, Options::OptionType_Graphics_ModernInterface },
		{ Options::Key_Graphics_TallPixelCorrection, Options::OptionType_Graphics_TallPixelCorrection },
		{ Options::Key_Graphics_RenderThreadsMode, Options::OptionType_Graphics_RenderThreadsMode },
		{ Options::Key_Graphics_DitheringMode, Options::OptionType_Graphics_DitheringMode }
	};

	constexpr std::pair<const char*, OptionType> AudioMappings[] =
	{
		{ Options::Key_Audio_MusicVolume, Options::OptionType_Audio_MusicVolume },
		{ Options::Key_Audio_SoundVolume, Options::OptionType_Audio_SoundVolume },
		{ Options::Key_Audio_MidiConfig, Options::OptionType_Audio_MidiConfig },
		{ Options::Key_Audio_SoundChannels, Options::OptionType_Audio_SoundChannels },
		{ Options::Key_Audio_SoundResampling, Options::OptionType_Audio_SoundResampling },
		{ Options::Key_Audio_Is3DAudio, Options::OptionType_Audio_Is3DAudio }
	};

	constexpr std::pair<const char*, OptionType> InputMappings[] =
	{
		{ Options::Key_Input_HorizontalSensitivity, Options::OptionType_Input_HorizontalSensitivity },
		{ Options::Key_Input_VerticalSensitivity, Options::OptionType_Input_VerticalSensitivity },
		{ Options::Key_Input_CameraPitchLimit, Options::OptionType_Input_CameraPitchLimit }
	};

	constexpr std::pair<const char*, OptionType> MiscMappings[] =
	{
		{ Options::Key_Misc_ArenaPaths, Options::OptionType_Misc_ArenaPaths },
		{ Options::Key_Misc_ArenaSavesPath, Options::OptionType_Misc_ArenaSavesPath },
		{ Options::Key_Misc_GhostMode, Options::OptionType_Misc_GhostMode },
		{ Options::Key_Misc_ProfilerLevel, Options::OptionType_Misc_ProfilerLevel },
		{ Options::Key_Misc_ShowIntro, Options::OptionType_Misc_ShowIntro },
		{ Options::Key_Misc_ShowCompass, Options::OptionType_Misc_ShowCompass },
		{ Options::Key_Misc_ChunkDistance, Options::OptionType_Misc_ChunkDistance },
		{ Options::Key_Misc_StarDensity, Options::OptionType_Misc_StarDensity },
		{ Options::Key_Misc_PlayerHasLight, Options::OptionType_Misc_PlayerHasLight }
	};

	std::unordered_set<std::string> s_loggedMissingOptions; // Reduces log spam.

	std::string MakeLoggingKey(const std::string &section, const std::string &key)
	{
		return section + "_" + key;
	}
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

void Options::load(const char *filename, std::unordered_map<std::string, Options::MapGroup> &maps)
{
	// Read the key-value pairs from each section in the given options file.
	KeyValueFile keyValueFile;
	if (!keyValueFile.init(filename))
	{
		DebugCrash("Couldn't load \"" + std::string(filename) + "\".");
		// @todo: return false
	}

	for (int sectionIndex = 0; sectionIndex < keyValueFile.getSectionCount(); sectionIndex++)
	{
		const KeyValueFileSection &section = keyValueFile.getSection(sectionIndex);
		const std::string &sectionName = section.getName();

		// Get the list of key-type pairs to pull from.
		const std::pair<const char*, OptionType> *mappingsListPtr = nullptr;
		size_t mappingsListLength = 0;
		if (sectionName == Options::SECTION_GRAPHICS)
		{
			mappingsListPtr = GraphicsMappings;
			mappingsListLength = std::size(GraphicsMappings);
		}
		else if (sectionName == Options::SECTION_INPUT)
		{
			mappingsListPtr = InputMappings;
			mappingsListLength = std::size(InputMappings);
		}
		else if (sectionName == Options::SECTION_AUDIO)
		{
			mappingsListPtr = AudioMappings;
			mappingsListLength = std::size(AudioMappings);
		}
		else if (sectionName == Options::SECTION_MISC)
		{
			mappingsListPtr = MiscMappings;
			mappingsListLength = std::size(MiscMappings);
		}
		else
		{
			DebugLogError("Unrecognized section \"" + sectionName + "\" in " + filename + ".");
			continue;
		}

		const std::pair<const char*, OptionType> *mappingsListEnd = mappingsListPtr + mappingsListLength;

		for (int pairIndex = 0; pairIndex < section.getPairCount(); pairIndex++)
		{
			const auto &pair = section.getPair(pairIndex);

			// See if the key is recognized, and if so, see what type the value should be, 
			// convert it, and place it in the changed map.
			const std::string &key = pair.first;
			const auto keyListIter = std::find_if(mappingsListPtr, mappingsListEnd,
				[&key](const std::pair<const char*, OptionType> &keyTypePair)
			{
				return keyTypePair.first == key;
			});

			if (keyListIter != mappingsListEnd)
			{
				const OptionType type = keyListIter->second;
				auto groupIter = maps.find(sectionName);

				// Add an empty map group if the section is new.
				if (groupIter == maps.end())
				{
					groupIter = maps.emplace(sectionName, Options::MapGroup()).first;
				}

				Options::MapGroup &mapGroup = groupIter->second;
				if (type == OptionType::Bool)
				{
					bool value;
					if (!section.tryGetBoolean(key, value))
					{
						DebugCrash("Couldn't get boolean \"" + key + "\" (section \"" + sectionName + "\").");
					}

					mapGroup.bools.emplace(key, value);
				}
				else if (type == OptionType::Int)
				{
					int value;
					if (!section.tryGetInteger(key, value))
					{
						DebugCrash("Couldn't get integer \"" + key + "\" (section \"" + sectionName + "\").");
					}

					mapGroup.integers.emplace(key, value);
				}
				else if (type == OptionType::Double)
				{
					double value;
					if (!section.tryGetDouble(key, value))
					{
						DebugCrash("Couldn't get double \"" + key + "\" (section \"" + sectionName + "\").");
					}

					mapGroup.doubles.emplace(key, value);
				}
				else if (type == OptionType::String)
				{
					std::string_view value;
					if (!section.tryGetString(key, value))
					{
						DebugCrash("Couldn't get string \"" + key + "\" (section \"" + sectionName + "\").");
					}

					mapGroup.strings.emplace(key, std::string(value));
				}
			}
			else
			{
				DebugLogWarning("Key \"" + key + "\" not recognized in " + filename + ".");
			}
		}
	}
}

int Options::clampInt(int value, int minValue, int maxValue, const char *name) const
{
	if (value < minValue)
	{
		value = minValue;
		DebugLogWarningFormat("%s (%d) must be at least %d.", name, value, minValue);
	}
	else if (value > maxValue)
	{
		value = maxValue;
		DebugLogWarningFormat("%s (%d) must be less than %d.", name, value, maxValue);
	}

	return value;
}

double Options::clampDouble(double value, double minValue, double maxValue, const char *name) const
{
	if (value < minValue)
	{
		value = minValue;
		DebugLogWarningFormat("%s (%.2f) must be at least %.2f.", name, value, minValue);
	}
	else if (value > maxValue)
	{
		value = maxValue;
		DebugLogWarningFormat("%s (%.2f) must be less than %.2f.", name, value, maxValue);
	}

	return value;
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
			std::string loggingKey = MakeLoggingKey(section, key);
			if (!s_loggedMissingOptions.contains(loggingKey))
			{
				s_loggedMissingOptions.emplace(loggingKey);
				DebugLogWarning("Expected \"" + key + "\" boolean under [" + section + "] in defaults or changes, defaulting to false and silencing warning.");
			}
			
			return false;
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
			std::string loggingKey = MakeLoggingKey(section, key);
			if (!s_loggedMissingOptions.contains(loggingKey))
			{
				s_loggedMissingOptions.emplace(loggingKey);
				DebugLogWarning("Expected \"" + key + "\" integer under [" + section + "] in defaults or changes, defaulting to 0 and silencing warning.");
			}

			return 0;
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
			std::string loggingKey = MakeLoggingKey(section, key);
			if (!s_loggedMissingOptions.contains(loggingKey))
			{
				s_loggedMissingOptions.emplace(loggingKey);
				DebugLogWarning("Expected \"" + key + "\" decimal value under [" + section + "] in defaults or changes, defaulting to 0 and silencing warning.");
			}

			return 0.0;
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
			std::string loggingKey = MakeLoggingKey(section, key);
			if (!s_loggedMissingOptions.contains(loggingKey))
			{
				s_loggedMissingOptions.emplace(loggingKey);
				DebugLogWarning("Expected \"" + key + "\" string under [" + section + "] in defaults or changes, defaulting to \"\" and silencing warning.");
			}

			static const std::string fallbackString;
			return fallbackString;
		}
	}
}

void Options::setBool(const std::string &section, const std::string &key, bool value)
{
	// Check that the section map exists. If not, add it.
	auto sectionIter = this->changedMaps.find(section);
	if (sectionIter == this->changedMaps.end())
	{
		sectionIter = this->changedMaps.emplace(section, Options::MapGroup()).first;
	}

	Options::BoolMap &sectionMap = sectionIter->second.bools;

	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.emplace(key, value).first;
	}

	iter->second = value;
}

void Options::setInt(const std::string &section, const std::string &key, int value)
{
	// Check that the section map exists. If not, add it.
	auto sectionIter = this->changedMaps.find(section);
	if (sectionIter == this->changedMaps.end())
	{
		sectionIter = this->changedMaps.emplace(section, Options::MapGroup()).first;
	}

	Options::IntegerMap &sectionMap = sectionIter->second.integers;

	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.emplace(key, value).first;
	}

	iter->second = value;
}

void Options::setDouble(const std::string &section, const std::string &key, double value)
{
	// Check that the section map exists. If not, add it.
	auto sectionIter = this->changedMaps.find(section);
	if (sectionIter == this->changedMaps.end())
	{
		sectionIter = this->changedMaps.emplace(section, Options::MapGroup()).first;
	}

	Options::DoubleMap &sectionMap = sectionIter->second.doubles;

	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.emplace(key, value).first;
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
		sectionIter = this->changedMaps.emplace(section, Options::MapGroup()).first;
	}

	Options::StringMap &sectionMap = sectionIter->second.strings;

	// Check that the key exists. If not, add it.
	auto iter = sectionMap.find(key);
	if (iter == sectionMap.end())
	{
		iter = sectionMap.emplace(key, value).first;
	}

	iter->second = value;
}

void Options::loadDefaults(const std::string &filename)
{
	DebugLog("Reading defaults \"" + filename + "\".");

	Options::load(filename.c_str(), this->defaultMaps);
}

void Options::loadChanges(const std::string &filename)
{
	DebugLog("Reading changes \"" + filename + "\".");

	Options::load(filename.c_str(), this->changedMaps);
}

void Options::saveChanges()
{
	const std::string optionsPath = Platform::getOptionsPath();
	const char *optionsPathPtr = optionsPath.c_str();
	if (!Directory::exists(optionsPathPtr))
	{
		Directory::createRecursively(optionsPathPtr);
	}

	const std::string filename(optionsPath + Options::CHANGES_FILENAME);
	std::ofstream ofs(filename);
	if (!ofs.is_open())
	{
		// @todo: doesn't need to be an exception -- can be a warning/error instead.
		DebugLogError("Could not save to \"" + filename + "\".");
		return;
	}

	// Writes out all key-value pairs in a section if it exists.
	auto tryWriteSection = [this, &ofs](const std::string &section, BufferView<const std::pair<const char*, OptionType>> keyList)
	{
		const auto sectionIter = this->changedMaps.find(section);
		if (sectionIter != this->changedMaps.end())
		{
			const auto &mapGroup = sectionIter->second;

			// Print section line.
			ofs << KeyValueFile::SECTION_FRONT << section << KeyValueFile::SECTION_BACK << '\n';

			// Write all pairs present in the current section.
			for (const auto &pair : keyList)
			{
				const char *key = pair.first;
				const OptionType type = pair.second;

				auto writePair = [&ofs, &key](const std::string &value)
				{
					ofs << key << KeyValueFile::PAIR_SEPARATOR << value << '\n';
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
					throw DebugException("Unrecognized option type \"" + std::to_string(static_cast<int>(type)) + "\".");
				}
			}

			ofs << '\n';
		}
	};

	ofs << "# The engine saves options here that differ from the defaults." << '\n';
	ofs << '\n';

	// Write out each section in a strict order.
	tryWriteSection(Options::SECTION_GRAPHICS, GraphicsMappings);
	tryWriteSection(Options::SECTION_AUDIO, AudioMappings);
	tryWriteSection(Options::SECTION_INPUT, InputMappings);
	tryWriteSection(Options::SECTION_MISC, MiscMappings);

	DebugLog("Saved settings in \"" + filename + "\".");
}
