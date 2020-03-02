#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <unordered_map>

// Settings found in the options menu are saved in this object, which should live in
// the game state object since it persists for the lifetime of the program.

enum class PlayerInterface;

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
	static void load(const char *filename,
		std::unordered_map<std::string, Options::MapGroup> &maps);

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
	static const int MIN_FPS;
	static const int MIN_WINDOW_MODE;
	static const int MAX_WINDOW_MODE;
	static const double MIN_RESOLUTION_SCALE;
	static const double MAX_RESOLUTION_SCALE;
	static const double MIN_VERTICAL_FOV;
	static const double MAX_VERTICAL_FOV;
	static const double MIN_CURSOR_SCALE;
	static const double MAX_CURSOR_SCALE;
	static const int MIN_LETTERBOX_MODE;
	static const int MAX_LETTERBOX_MODE;
	static const int MIN_RENDER_THREADS_MODE;
	static const int MAX_RENDER_THREADS_MODE;
	static const double MIN_HORIZONTAL_SENSITIVITY;
	static const double MAX_HORIZONTAL_SENSITIVITY;
	static const double MIN_VERTICAL_SENSITIVITY;
	static const double MAX_VERTICAL_SENSITIVITY;
	static const double MIN_CAMERA_PITCH_LIMIT;
	static const double MAX_CAMERA_PITCH_LIMIT;
	static const double MIN_VOLUME;
	static const double MAX_VOLUME;
	static const int MIN_SOUND_CHANNELS;
	static const int RESAMPLING_OPTION_COUNT;
	static const double MIN_TIME_SCALE;
	static const double MAX_TIME_SCALE;
	static const int MIN_STAR_DENSITY_MODE;
	static const int MAX_STAR_DENSITY_MODE;
	static const int MIN_PROFILER_LEVEL;
	static const int MAX_PROFILER_LEVEL;

#define OPTION_BOOL(section, name) \
bool get##section##_##name() const \
{ \
	return this->getBool(#section, #name); \
} \
void set##section##_##name(bool value) \
{ \
	this->setBool(#section, #name, value); \
}

#define OPTION_INT(section, name) \
void check##section##_##name(int value) const; \
int get##section##_##name() const \
{ \
	const int value = this->getInt(#section, #name); \
	this->check##section##_##name(value); \
	return value; \
} \
void set##section##_##name(int value) \
{ \
	this->check##section##_##name(value); \
	this->setInt(#section, #name, value); \
}

#define OPTION_DOUBLE(section, name) \
void check##section##_##name(double value) const; \
double get##section##_##name() const \
{ \
	const double value = this->getDouble(#section, #name); \
	this->check##section##_##name(value); \
	return value; \
} \
void set##section##_##name(double value) \
{ \
	this->check##section##_##name(value); \
	this->setDouble(#section, #name, value); \
}

#define OPTION_STRING(section, name) \
const std::string &get##section##_##name() const \
{ \
	return this->getString(#section, #name); \
} \
void set##section##_##name(const std::string &value) \
{ \
	this->setString(#section, #name, value); \
}

	// Getter, setter, and optional checker methods.
	OPTION_INT(Graphics, ScreenWidth)
	OPTION_INT(Graphics, ScreenHeight)
	OPTION_INT(Graphics, WindowMode)
	OPTION_INT(Graphics, TargetFPS)
	OPTION_DOUBLE(Graphics, ResolutionScale)
	OPTION_DOUBLE(Graphics, VerticalFOV)
	OPTION_BOOL(Graphics, ParallaxSky)
	OPTION_INT(Graphics, LetterboxMode)
	OPTION_DOUBLE(Graphics, CursorScale)
	OPTION_BOOL(Graphics, ModernInterface)
	OPTION_INT(Graphics, RenderThreadsMode)

	OPTION_DOUBLE(Audio, MusicVolume)
	OPTION_DOUBLE(Audio, SoundVolume)
	OPTION_STRING(Audio, MidiConfig)
	OPTION_INT(Audio, SoundChannels)
	OPTION_INT(Audio, SoundResampling)
	OPTION_BOOL(Audio, Is3DAudio)

	OPTION_DOUBLE(Input, HorizontalSensitivity)
	OPTION_DOUBLE(Input, VerticalSensitivity)
	OPTION_DOUBLE(Input, CameraPitchLimit)
	OPTION_BOOL(Input, PixelPerfectSelection)

	OPTION_STRING(Misc, ArenaPath)
	OPTION_STRING(Misc, ArenaSavesPath)
	OPTION_BOOL(Misc, Collision)
	OPTION_INT(Misc, ProfilerLevel)
	OPTION_BOOL(Misc, ShowIntro)
	OPTION_BOOL(Misc, ShowCompass)
	OPTION_DOUBLE(Misc, TimeScale)
	OPTION_INT(Misc, StarDensity)

	// Reads all the key-values pairs from the given absolute path into the default members.
	void loadDefaults(const std::string &filename);

	// Reads all the key-value pairs from the given absolute path into the changes members,
	// overwriting any existing values.
	void loadChanges(const std::string &filename);

	// Saves all key-value pairs that differ from the defaults to the changed options file.
	void saveChanges();
};

#endif
