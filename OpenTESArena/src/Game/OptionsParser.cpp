#include <cassert>

#include "OptionsParser.h"

#include "Options.h"
#include "PlayerInterface.h"
#include "../Utilities/Debug.h"
#include "../Utilities/KeyValueMap.h"

// The path to the options folder is platform-dependent (see SDL_GetPrefPath).
const std::string OptionsParser::FILENAME = "options.txt";

const std::string OptionsParser::SCREEN_WIDTH_KEY = "ScreenWidth";
const std::string OptionsParser::SCREEN_HEIGHT_KEY = "ScreenHeight";
const std::string OptionsParser::FULLSCREEN_KEY = "Fullscreen";
const std::string OptionsParser::TARGET_FPS_KEY = "TargetFPS";
const std::string OptionsParser::RESOLUTION_SCALE_KEY = "ResolutionScale";
const std::string OptionsParser::VERTICAL_FOV_KEY = "VerticalFieldOfView";
const std::string OptionsParser::LETTERBOX_ASPECT_KEY = "LetterboxAspect";
const std::string OptionsParser::CURSOR_SCALE_KEY = "CursorScale";
const std::string OptionsParser::MODERN_INTERFACE_KEY = "ModernInterface";
const std::string OptionsParser::H_SENSITIVITY_KEY = "HorizontalSensitivity";
const std::string OptionsParser::V_SENSITIVITY_KEY = "VerticalSensitivity";
const std::string OptionsParser::MUSIC_VOLUME_KEY = "MusicVolume";
const std::string OptionsParser::SOUND_VOLUME_KEY = "SoundVolume";
const std::string OptionsParser::SOUNDFONT_KEY = "Soundfont";
const std::string OptionsParser::SOUND_CHANNELS_KEY = "SoundChannels";
const std::string OptionsParser::ARENA_PATH_KEY = "ArenaPath";
const std::string OptionsParser::SKIP_INTRO_KEY = "SkipIntro";
const std::string OptionsParser::SHOW_DEBUG_KEY = "ShowDebug";

std::unique_ptr<Options> OptionsParser::parse(const std::string &filename)
{
	DebugMention("Reading \"" + filename + "\".");

	// Read in all the key-value pairs from the options file.
	KeyValueMap textMap(filename);
	
	// Graphics.
	int screenWidth = textMap.getInteger(OptionsParser::SCREEN_WIDTH_KEY);
	int screenHeight = textMap.getInteger(OptionsParser::SCREEN_HEIGHT_KEY);
	bool fullscreen = textMap.getBoolean(OptionsParser::FULLSCREEN_KEY);
	int targetFPS = textMap.getInteger(OptionsParser::TARGET_FPS_KEY);
	double resolutionScale = textMap.getDouble(OptionsParser::RESOLUTION_SCALE_KEY);
	double verticalFOV = textMap.getDouble(OptionsParser::VERTICAL_FOV_KEY);
	double letterboxAspect = textMap.getDouble(OptionsParser::LETTERBOX_ASPECT_KEY);
	double cursorScale = textMap.getDouble(OptionsParser::CURSOR_SCALE_KEY);
	bool modernInterface = textMap.getBoolean(OptionsParser::MODERN_INTERFACE_KEY);

	// Input.
	double hSensitivity = textMap.getDouble(OptionsParser::H_SENSITIVITY_KEY);
	double vSensitivity = textMap.getDouble(OptionsParser::V_SENSITIVITY_KEY);

    // Sound.
    double musicVolume = textMap.getDouble(OptionsParser::MUSIC_VOLUME_KEY);
    double soundVolume = textMap.getDouble(OptionsParser::SOUND_VOLUME_KEY);
	std::string soundfont = textMap.getString(OptionsParser::SOUNDFONT_KEY);
    int soundChannels = textMap.getInteger(OptionsParser::SOUND_CHANNELS_KEY);

	// Miscellaneous.
	std::string arenaPath = textMap.getString(OptionsParser::ARENA_PATH_KEY);
	bool skipIntro = textMap.getBoolean(OptionsParser::SKIP_INTRO_KEY);
	bool showDebug = textMap.getBoolean(OptionsParser::SHOW_DEBUG_KEY);
	
	return std::unique_ptr<Options>(new Options(std::move(arenaPath),
		screenWidth, screenHeight, fullscreen, targetFPS, resolutionScale, verticalFOV,
		letterboxAspect, cursorScale, hSensitivity, vSensitivity, std::move(soundfont),
		musicVolume, soundVolume, soundChannels, skipIntro,
		modernInterface ? PlayerInterface::Modern : PlayerInterface::Classic,
		showDebug));
}

void OptionsParser::save(const Options &options)
{
	// not done! 
	
	// Maybe make a "KeyValueMap::save(std::map, filename)" method?
	// What about custom comments per line? std::vector<std::string> would be better,
	// but that doesn't require KeyValueMap functionality then. There could be default
	// comments, like "# Graphics", etc..

	// Maybe it could just replace existing key values. No need for new comments.
}
