#ifndef OPTIONS_H
#define OPTIONS_H

#include <limits>
#include <string>
#include <unordered_map>

enum class PlayerInterface;

// Supported value types by the parser.
enum class OptionType { Bool, Int, Double, String };

// Options menu settings are saved in this. Persists for the lifetime of the program.
class Options
{
private:
	using BoolMap = std::unordered_map<std::string, bool>;
	using IntegerMap = std::unordered_map<std::string, int>;
	using DoubleMap = std::unordered_map<std::string, double>;
	using StringMap = std::unordered_map<std::string, std::string>;

	struct MapGroup
	{
		BoolMap bools;
		IntegerMap integers;
		DoubleMap doubles;
		StringMap strings;
	};

	// Default values come from the default options file. Changed values come from
	// changes at runtime, and those are written to the changed options file. Each
	// section in the options file has its own map of values.
	std::unordered_map<std::string, MapGroup> defaultMaps, changedMaps;

	// Opens the given file and reads its key-value pairs into the given maps.
	static void load(const char *filename, std::unordered_map<std::string, Options::MapGroup> &maps);

	int clampInt(int value, int minValue, int maxValue, const char *name) const;
	double clampDouble(double value, double minValue, double maxValue, const char *name) const;

	bool getBool(const std::string &section, const std::string &key) const;
	int getInt(const std::string &section, const std::string &key) const;
	double getDouble(const std::string &section, const std::string &key) const;
	const std::string &getString(const std::string &section, const std::string &key) const;

	void setBool(const std::string &section, const std::string &key, bool value);
	void setInt(const std::string &section, const std::string &key, int value);
	void setDouble(const std::string &section, const std::string &key, double value);
	void setString(const std::string &section, const std::string &key, const std::string &value);
public:
	// Filename of the default options file.
	static const std::string DEFAULT_FILENAME;

	// Filename of the "changes" options file, the one that tracks runtime changes.
	static const std::string CHANGES_FILENAME;

	// Section names for each group of related values.
	static const std::string SECTION_GRAPHICS;
	static const std::string SECTION_AUDIO;
	static const std::string SECTION_INPUT;
	static const std::string SECTION_MISC;

	// Min/max/allowed values for the application.
	static constexpr int MIN_FPS = 15;
	static constexpr int MIN_WINDOW_MODE = 0;
	static constexpr int MAX_WINDOW_MODE = 2;
	static constexpr double MIN_RESOLUTION_SCALE = 0.10;
	static constexpr double MAX_RESOLUTION_SCALE = 1.0;
	static constexpr double MIN_VERTICAL_FOV = 40.0;
	static constexpr double MAX_VERTICAL_FOV = 120.0;
	static constexpr double MIN_CURSOR_SCALE = 0.50;
	static constexpr double MAX_CURSOR_SCALE = 12.0;
	static constexpr int MIN_LETTERBOX_MODE = 0;
	static constexpr int MAX_LETTERBOX_MODE = 2;
	static constexpr int MIN_GRAPHICS_API = 0;
	static constexpr int MAX_GRAPHICS_API = 1;
	static constexpr int MIN_RENDER_THREADS_MODE = 0;
	static constexpr int MAX_RENDER_THREADS_MODE = 5;
	static constexpr int MIN_DITHERING_MODE = 0;
	static constexpr int MAX_DITHERING_MODE = 2;
	static constexpr double MIN_HORIZONTAL_SENSITIVITY = 0.50;
	static constexpr double MAX_HORIZONTAL_SENSITIVITY = 50.0;
	static constexpr double MIN_VERTICAL_SENSITIVITY = 0.50;
	static constexpr double MAX_VERTICAL_SENSITIVITY = 50.0;
	static constexpr double MIN_CAMERA_PITCH_LIMIT = 0.0;
	static constexpr double MAX_CAMERA_PITCH_LIMIT = 90.0;
	static constexpr double MIN_VOLUME = 0.0;
	static constexpr double MAX_VOLUME = 1.0;
	static constexpr int MIN_SOUND_CHANNELS = 1;
	static constexpr int MIN_RESAMPLING_MODE = 0;
	static constexpr int MAX_RESAMPLING_MODE = 3;
	static constexpr int MIN_CHUNK_DISTANCE = 1;
	static constexpr int MIN_STAR_DENSITY_MODE = 0;
	static constexpr int MAX_STAR_DENSITY_MODE = 2;
	static constexpr int MIN_PROFILER_LEVEL = 0;
	static constexpr int MAX_PROFILER_LEVEL = 3;

#define OPTION_BOOL(section, name) \
static constexpr const char Key_##section##_##name[] = #name; \
static constexpr OptionType OptionType_##section##_##name = OptionType::Bool; \
bool get##section##_##name() const \
{ \
	return this->getBool(#section, #name); \
} \
void set##section##_##name(bool value) \
{ \
	this->setBool(#section, #name, value); \
}

#define OPTION_INT(section, name, minValue, maxValue) \
static constexpr const char Key_##section##_##name[] = #name; \
static constexpr OptionType OptionType_##section##_##name = OptionType::Int; \
int clamp##section##_##name(int value) const \
{ \
	return this->clampInt(value, minValue, maxValue, #name); \
} \
int get##section##_##name() const \
{ \
	const int value = this->getInt(#section, #name); \
	return this->clamp##section##_##name(value); \
} \
void set##section##_##name(int value) \
{ \
	const int clampedValue = this->clamp##section##_##name(value); \
	this->setInt(#section, #name, clampedValue); \
}

#define OPTION_DOUBLE(section, name, minValue, maxValue) \
static constexpr const char Key_##section##_##name[] = #name; \
static constexpr OptionType OptionType_##section##_##name = OptionType::Double; \
double clamp##section##_##name(double value) const \
{ \
	return this->clampDouble(value, minValue, maxValue, #name); \
} \
double get##section##_##name() const \
{ \
	const double value = this->getDouble(#section, #name); \
	return this->clamp##section##_##name(value); \
} \
void set##section##_##name(double value) \
{ \
	const double clampedValue = this->clamp##section##_##name(value); \
	this->setDouble(#section, #name, clampedValue); \
}

#define OPTION_STRING(section, name) \
static constexpr const char Key_##section##_##name[] = #name; \
static constexpr OptionType OptionType_##section##_##name = OptionType::String; \
const std::string &get##section##_##name() const \
{ \
	return this->getString(#section, #name); \
} \
void set##section##_##name(const std::string &value) \
{ \
	this->setString(#section, #name, value); \
}

	// Getter, setter, and optional checker methods.
	OPTION_INT(Graphics, ScreenWidth, 1, std::numeric_limits<int>::max())
	OPTION_INT(Graphics, ScreenHeight, 1, std::numeric_limits<int>::max())
	OPTION_INT(Graphics, WindowMode, MIN_WINDOW_MODE, MAX_WINDOW_MODE)
	OPTION_INT(Graphics, GraphicsAPI, MIN_GRAPHICS_API, MAX_GRAPHICS_API)
	OPTION_INT(Graphics, TargetFPS, MIN_FPS, std::numeric_limits<int>::max())
	OPTION_DOUBLE(Graphics, ResolutionScale, MIN_RESOLUTION_SCALE, MAX_RESOLUTION_SCALE)
	OPTION_DOUBLE(Graphics, VerticalFOV, MIN_VERTICAL_FOV, MAX_VERTICAL_FOV)
	OPTION_INT(Graphics, LetterboxMode, MIN_LETTERBOX_MODE, MAX_LETTERBOX_MODE)
	OPTION_DOUBLE(Graphics, CursorScale, MIN_CURSOR_SCALE, MAX_CURSOR_SCALE)
	OPTION_BOOL(Graphics, ModernInterface)
	OPTION_BOOL(Graphics, TallPixelCorrection)
	OPTION_INT(Graphics, RenderThreadsMode, MIN_RENDER_THREADS_MODE, MAX_RENDER_THREADS_MODE)
	OPTION_INT(Graphics, DitheringMode, MIN_DITHERING_MODE, MAX_DITHERING_MODE)

	OPTION_DOUBLE(Audio, MusicVolume, MIN_VOLUME, MAX_VOLUME)
	OPTION_DOUBLE(Audio, SoundVolume, MIN_VOLUME, MAX_VOLUME)
	OPTION_STRING(Audio, MidiConfig)
	OPTION_INT(Audio, SoundChannels, MIN_SOUND_CHANNELS, std::numeric_limits<int>::max())
	OPTION_INT(Audio, SoundResampling, MIN_RESAMPLING_MODE, MAX_RESAMPLING_MODE)
	OPTION_BOOL(Audio, Is3DAudio)

	OPTION_DOUBLE(Input, HorizontalSensitivity, MIN_HORIZONTAL_SENSITIVITY, MAX_HORIZONTAL_SENSITIVITY)
	OPTION_DOUBLE(Input, VerticalSensitivity, MIN_VERTICAL_SENSITIVITY, MAX_VERTICAL_SENSITIVITY)
	OPTION_BOOL(Input, InvertVerticalAxis)
	OPTION_DOUBLE(Input, CameraPitchLimit, MIN_CAMERA_PITCH_LIMIT, MAX_CAMERA_PITCH_LIMIT)

	OPTION_STRING(Misc, ArenaPaths)
	OPTION_STRING(Misc, ArenaSavesPath)
	OPTION_BOOL(Misc, GhostMode)
	OPTION_INT(Misc, ProfilerLevel, MIN_PROFILER_LEVEL, MAX_PROFILER_LEVEL)
	OPTION_BOOL(Misc, ShowIntro)
	OPTION_BOOL(Misc, ShowCompass)
	OPTION_INT(Misc, ChunkDistance, MIN_CHUNK_DISTANCE, std::numeric_limits<int>::max())
	OPTION_INT(Misc, StarDensity, MIN_STAR_DENSITY_MODE, MAX_STAR_DENSITY_MODE)
	OPTION_BOOL(Misc, PlayerHasLight)
	OPTION_BOOL(Misc, EnableValidationLayers)

	// Reads all the key-values pairs from the given absolute path into the default members.
	void loadDefaults(const std::string &filename);

	// Reads all the key-value pairs from the given absolute path into the changes members,
	// overwriting any existing values.
	void loadChanges(const std::string &filename);

	// Saves all key-value pairs that differ from the defaults to the changed options file.
	void saveChanges();
};

#endif
