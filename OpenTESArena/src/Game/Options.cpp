#include <algorithm>
#include <fstream>
#include <sstream>

#include "Options.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/Directory.h"
#include "components/utilities/KeyValueFile.h"
#include "components/utilities/String.h"

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
		{ "WindowMode", OptionType::Int },
		{ "TargetFPS", OptionType::Int },
		{ "ResolutionScale", OptionType::Double },
		{ "VerticalFOV", OptionType::Double },
		{ "LetterboxMode", OptionType::Int },
		{ "CursorScale", OptionType::Double },
		{ "ModernInterface", OptionType::Bool },
		{ "TallPixelCorrection", OptionType::Bool },
		{ "RenderThreadsMode", OptionType::Int },
		{ "DitheringMode", OptionType::Int }
	};

	const std::vector<std::pair<std::string, OptionType>> AudioMappings =
	{
		{ "MusicVolume", OptionType::Double },
		{ "SoundVolume", OptionType::Double },
		{ "MidiConfig", OptionType::String },
		{ "SoundChannels", OptionType::Int },
		{ "SoundResampling", OptionType::Int },
		{ "Is3DAudio", OptionType::Bool }
	};

	const std::vector<std::pair<std::string, OptionType>> InputMappings =
	{
		{ "HorizontalSensitivity", OptionType::Double },
		{ "VerticalSensitivity", OptionType::Double },
		{ "CameraPitchLimit", OptionType::Double }
	};

	const std::vector<std::pair<std::string, OptionType>> MiscMappings =
	{
		{ "ArenaPaths", OptionType::String },
		{ "ArenaSavesPath", OptionType::String },
		{ "GhostMode", OptionType::Bool },
		{ "ProfilerLevel", OptionType::Int },
		{ "ShowIntro", OptionType::Bool },
		{ "ShowCompass", OptionType::Bool },
		{ "ChunkDistance", OptionType::Int },
		{ "StarDensity", OptionType::Int },
		{ "PlayerHasLight", OptionType::Bool }
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

void Options::load(const char *filename,
	std::unordered_map<std::string, Options::MapGroup> &maps)
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
		const KeyValueFile::Section &section = keyValueFile.getSection(sectionIndex);
		const std::string &sectionName = section.getName();

		// Get the list of key-type pairs to pull from.
		const auto &keyList = [&filename, &sectionName]()
		{
			if (sectionName == Options::SECTION_GRAPHICS)
			{
				return GraphicsMappings;
			}
			else if (sectionName == Options::SECTION_INPUT)
			{
				return InputMappings;
			}
			else if (sectionName == Options::SECTION_AUDIO)
			{
				return AudioMappings;
			}
			else if (sectionName == Options::SECTION_MISC)
			{
				return MiscMappings;
			}
			else
			{
				throw DebugException("Unrecognized section \"" + sectionName + "\" in " + filename + ".");
			}
		}();

		for (int pairIndex = 0; pairIndex < section.getPairCount(); pairIndex++)
		{
			const auto &pair = section.getPair(pairIndex);

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
			DebugLogWarning("Boolean \"" + key + "\" (section \"" + section + "\") not in options, defaulting to false.");
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
			DebugLogWarning("Integer \"" + key + "\" (section \"" + section + "\") not in options, defaulting to 0.");
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
			DebugLogWarning("Double \"" + key + "\" (section \"" + section + "\") not in options, defaulting to 0.");
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
			DebugLogWarning("String \"" + key + "\" (section \"" + section + "\") not in options, defaulting to \"\".");
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

int Options::clampGraphics_ScreenWidth(int value) const
{
	if (value <= 0)
	{
		value = 1280;
		DebugLogWarning("Screen width must be positive, defaulting to " + std::to_string(value) + ".");
	}
	
	return value;
}

int Options::clampGraphics_ScreenHeight(int value) const
{
	if (value <= 0)
	{
		value = 720;
		DebugLogWarning("Screen height must be positive, defaulting to " + std::to_string(value) + ".");
	}

	return value;
}

int Options::clampGraphics_WindowMode(int value) const
{
	if (value < Options::MIN_WINDOW_MODE)
	{
		value = Options::MIN_WINDOW_MODE;
		DebugLogWarning("Window mode must be at least " + std::to_string(value) + ".");
	}
	else if (value > Options::MAX_WINDOW_MODE)
	{
		value = Options::MAX_WINDOW_MODE;
		DebugLogWarning("Window mode must be no greater than " + std::to_string(value) + ".");
	}

	return value;
}

int Options::clampGraphics_TargetFPS(int value) const
{
	if (value < Options::MIN_FPS)
	{
		value = Options::MIN_FPS;
		DebugLogWarning("Target FPS must be at least " + std::to_string(value) + ".");
	}

	return value;
}

double Options::clampGraphics_ResolutionScale(double value) const
{
	if (value < Options::MIN_RESOLUTION_SCALE)
	{
		value = Options::MIN_RESOLUTION_SCALE;
		DebugLogWarning("Resolution scale must be at least " + String::fixedPrecision(value, 2) + ".");
	}
	else if (value > Options::MAX_RESOLUTION_SCALE)
	{
		value = Options::MAX_RESOLUTION_SCALE;
		DebugLogWarning("Resolution scale must be no greater than " + String::fixedPrecision(value, 2) + ".");
	}

	return value;
}

double Options::clampGraphics_VerticalFOV(double value) const
{
	if (value < Options::MIN_VERTICAL_FOV)
	{
		value = Options::MIN_VERTICAL_FOV;
		DebugLogWarning("Vertical FOV must be at least " + String::fixedPrecision(value, 1) + ".");
	}
	else if (value > Options::MAX_VERTICAL_FOV)
	{
		value = Options::MAX_VERTICAL_FOV;
		DebugLogWarning("Vertical FOV must be no greater than " + String::fixedPrecision(value, 1) + ".");
	}

	return value;
}

int Options::clampGraphics_LetterboxMode(int value) const
{
	if (value < Options::MIN_LETTERBOX_MODE)
	{
		value = Options::MIN_LETTERBOX_MODE;
		DebugLogWarning("Letterbox mode must be at least " + std::to_string(value) + ".");
	}
	else if (value > Options::MAX_LETTERBOX_MODE)
	{
		value = Options::MAX_LETTERBOX_MODE;
		DebugLogWarning("Letterbox mode must be no greater than " + std::to_string(value) + ".");
	}

	return value;
}

double Options::clampGraphics_CursorScale(double value) const
{
	if (value < Options::MIN_CURSOR_SCALE)
	{
		value = Options::MIN_CURSOR_SCALE;
		DebugLogWarning("Cursor scale must be at least " + String::fixedPrecision(value, 1) + ".");
	}
	else if (value > Options::MAX_CURSOR_SCALE)
	{
		value = Options::MAX_CURSOR_SCALE;
		DebugLogWarning("Cursor scale must be no greater than " + String::fixedPrecision(value, 1) + ".");
	}

	return value;
}

int Options::clampGraphics_RenderThreadsMode(int value) const
{
	if (value < Options::MIN_RENDER_THREADS_MODE)
	{
		value = Options::MIN_RENDER_THREADS_MODE;
		DebugLogWarning("Render threads mode must be at least " + std::to_string(value) + ".");
	}
	else if (value > Options::MAX_RENDER_THREADS_MODE)
	{
		value = Options::MAX_RENDER_THREADS_MODE;
		DebugLogWarning("Render threads mode must be no greater than " + std::to_string(value) + ".");
	}

	return value;
}

int Options::clampGraphics_DitheringMode(int value) const
{
	if (value < Options::MIN_DITHERING_MODE)
	{
		value = Options::MIN_DITHERING_MODE;
		DebugLogWarning("Dithering mode must be at least " + std::to_string(value) + ".");
	}
	else if (value > Options::MAX_DITHERING_MODE)
	{
		value = Options::MAX_DITHERING_MODE;
		DebugLogWarning("Dithering mode must be no greater than " + std::to_string(value) + ".");
	}

	return value;
}

double Options::clampAudio_MusicVolume(double value) const
{
	if (value < Options::MIN_VOLUME)
	{
		value = Options::MIN_VOLUME;
		DebugLogWarning("Music volume must be at least " + String::fixedPrecision(value, 1) + ".");
	}
	else if (value > Options::MAX_VOLUME)
	{
		value = Options::MAX_VOLUME;
		DebugLogWarning("Music volume must be no greater than " + String::fixedPrecision(value, 1) + ".");
	}

	return value;
}

double Options::clampAudio_SoundVolume(double value) const
{
	if (value < Options::MIN_VOLUME)
	{
		value = Options::MIN_VOLUME;
		DebugLogWarning("Sound volume must be at least " + String::fixedPrecision(value, 1) + ".");
	}
	else if (value > Options::MAX_VOLUME)
	{
		value = Options::MAX_VOLUME;
		DebugLogWarning("Sound volume must be no greater than " + String::fixedPrecision(value, 1) + ".");
	}

	return value;
}

int Options::clampAudio_SoundChannels(int value) const
{
	if (value < Options::MIN_SOUND_CHANNELS)
	{
		value = Options::MIN_SOUND_CHANNELS;
		DebugLogWarning("Sound channel count must be at least " + std::to_string(value) + ".");
	}

	return value;
}

int Options::clampAudio_SoundResampling(int value) const
{
	if (value < Options::MIN_RESAMPLING_MODE)
	{
		value = Options::MIN_RESAMPLING_MODE;
		DebugLogWarning("Sound resampling value must be at least " + std::to_string(value) + ".");
	}
	else if (value > Options::MAX_RESAMPLING_MODE)
	{
		value = Options::MAX_RESAMPLING_MODE;
		DebugLogWarning("Sound resampling value must be no greater than " + std::to_string(value) + ".");
	}

	return value;
}

double Options::clampInput_HorizontalSensitivity(double value) const
{
	if (value < Options::MIN_HORIZONTAL_SENSITIVITY)
	{
		value = Options::MIN_HORIZONTAL_SENSITIVITY;
		DebugLogWarning("Horizontal sensitivity must be at least " + String::fixedPrecision(value, 1) + ".");
	}
	else if (value > Options::MAX_HORIZONTAL_SENSITIVITY)
	{
		value = Options::MAX_HORIZONTAL_SENSITIVITY;
		DebugLogWarning("Horizontal sensitivity must be no greater than " + String::fixedPrecision(value, 1) + ".");
	}

	return value;
}

double Options::clampInput_VerticalSensitivity(double value) const
{
	if (value < Options::MIN_VERTICAL_SENSITIVITY)
	{
		value = Options::MIN_VERTICAL_SENSITIVITY;
		DebugLogWarning("Vertical sensitivity must be at least " + String::fixedPrecision(value, 1) + ".");
	}
	else if (value > Options::MAX_VERTICAL_SENSITIVITY)
	{
		value = Options::MAX_VERTICAL_SENSITIVITY;
		DebugLogWarning("Vertical sensitivity must be no greater than " + String::fixedPrecision(value, 1) + ".");
	}

	return value;
}

double Options::clampInput_CameraPitchLimit(double value) const
{
	if (value < Options::MIN_CAMERA_PITCH_LIMIT)
	{
		value = Options::MIN_CAMERA_PITCH_LIMIT;
		DebugLogWarning("Camera pitch limit must be at least " + String::fixedPrecision(value, 1) + ".");
	}
	else if (value > Options::MAX_CAMERA_PITCH_LIMIT)
	{
		value = Options::MAX_CAMERA_PITCH_LIMIT;
		DebugLogWarning("Camera pitch limit must be no greater than " + String::fixedPrecision(value, 1) + ".");
	}

	return value;
}

int Options::clampMisc_ChunkDistance(int value) const
{
	if (value < Options::MIN_CHUNK_DISTANCE)
	{
		value = Options::MIN_CHUNK_DISTANCE;
		DebugLogWarning("Chunk distance must be at least " + std::to_string(value) + ".");
	}

	return value;
}

int Options::clampMisc_StarDensity(int value) const
{
	if (value < Options::MIN_STAR_DENSITY_MODE)
	{
		value = Options::MIN_STAR_DENSITY_MODE;
		DebugLogWarning("Star density must be at least " + std::to_string(value) + ".");
	}
	else if (value > Options::MAX_STAR_DENSITY_MODE)
	{
		value = Options::MAX_STAR_DENSITY_MODE;
		DebugLogWarning("Star density must be no greater than " + std::to_string(value) + ".");
	}

	return value;
}

int Options::clampMisc_ProfilerLevel(int value) const
{
	if (value < Options::MIN_PROFILER_LEVEL)
	{
		value = Options::MIN_PROFILER_LEVEL;
		DebugLogWarning("Profiler level must be at least " + std::to_string(value) + ".");
	}
	else if (value > Options::MAX_PROFILER_LEVEL)
	{
		value = Options::MAX_PROFILER_LEVEL;
		DebugLogWarning("Profiler level must be no greater than " + std::to_string(value) + ".");
	}

	return value;
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
				ofs << KeyValueFile::SECTION_FRONT << section << KeyValueFile::SECTION_BACK << '\n';

				// Write all pairs present in the current section.
				for (const auto &pair : keyList)
				{
					const std::string &key = pair.first;
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

		DebugLog("Saved settings in \"" + filename + "\".");
	}
	else
	{
		// @todo: doesn't need to be an exception -- can be a warning/error instead.
		throw DebugException("Could not save to \"" + filename + "\".");
	}
}
