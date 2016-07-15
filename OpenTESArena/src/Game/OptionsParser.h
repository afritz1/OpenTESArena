#ifndef OPTIONS_PARSER_H
#define OPTIONS_PARSER_H

#include <memory>
#include <string>

// The options parser uses the options text file to generate an options object.

class Options;

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
	static const std::string CURSOR_SCALE_KEY;

	// Input.
	static const std::string H_SENSITIVITY_KEY;
	static const std::string V_SENSITIVITY_KEY;

    // Sound.
    static const std::string MUSIC_VOLUME_KEY;
    static const std::string SOUND_VOLUME_KEY;
	static const std::string SOUNDFONT_KEY;
    static const std::string SOUND_CHANNELS_KEY;

	// Miscellaneous.
	static const std::string DATA_PATH_KEY;
	static const std::string SKIP_INTRO_KEY;

	OptionsParser() = delete;
	OptionsParser(const OptionsParser&) = delete;
	~OptionsParser() = delete;
public:
	static std::unique_ptr<Options> parse();

	// Overwrite the options text file with a new options object.
	static void save(const Options &options);
};

#endif
