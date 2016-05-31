#ifndef OPTIONS_PARSER_H
#define OPTIONS_PARSER_H

#include <map>
#include <memory>
#include <sstream>
#include <vector>

// The options parser reads each valid key value pair in the options file into a
// map. If a requested key doesn't exist, the parser explodes.

// Keys can be listed in the file in any order.

class Options;

enum class MusicFormat;
enum class SoundFormat;

class OptionsParser
{
private:
	static const std::string PATH;
	static const std::string FILENAME;

	// Graphics.
	static const std::string SCREEN_WIDTH_KEY;
	static const std::string SCREEN_HEIGHT_KEY;
	static const std::string FULLSCREEN_KEY;
	static const std::string VERTICAL_FOV_KEY;

	// Input.
	static const std::string H_SENSITIVITY_KEY;
	static const std::string V_SENSITIVITY_KEY;

	// Sound.
	static const std::string MUSIC_VOLUME_KEY;
	static const std::string SOUND_VOLUME_KEY;
	static const std::string SOUND_CHANNELS_KEY;
	static const std::string MUSIC_FORMAT_KEY;
	static const std::string SOUND_FORMAT_KEY;

	// Miscellaneous.
	static const std::string SKIP_INTRO_KEY;

	OptionsParser() = delete;
	OptionsParser(const OptionsParser&) = delete;
	~OptionsParser() = delete;

	static std::map<std::string, std::string> getPairs(const std::string &text);
	static std::string getValue(const std::map<std::string, std::string> &pairs,
		const std::string &key);
	static int getInteger(const std::map<std::string, std::string> &pairs,
		const std::string &key);
	static double getDouble(const std::map<std::string, std::string> &pairs,
		const std::string &key);
	static bool getBoolean(const std::map<std::string, std::string> &pairs,
		const std::string &key);
	static MusicFormat getMusicFormat(const std::map<std::string, std::string> &pairs);
	static SoundFormat getSoundFormat(const std::map<std::string, std::string> &pairs);
public:
	static std::unique_ptr<Options> parse();

	// Overwrite the options text file with a new options object.
	static void save(const Options &options);
};

#endif
