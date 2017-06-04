#include <cassert>

#include "Options.h"

#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

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

Options::Options(std::string &&arenaPath, int screenWidth, int screenHeight, bool fullscreen,
	int targetFPS, double resolutionScale, double verticalFOV, double letterboxAspect,
	double cursorScale, double hSensitivity, double vSensitivity, std::string &&soundfont,
	double musicVolume, double soundVolume, int soundChannels, bool skipIntro,
	PlayerInterface playerInterface, bool showDebug)
	: arenaPath(std::move(arenaPath)), soundfont(std::move(soundfont))
{
	// Make sure each of the values is in a valid range.
	DebugAssert(screenWidth > 0, "Screen width must be positive.");
	DebugAssert(screenHeight > 0, "Screen height must be positive.");
	DebugAssert(targetFPS >= Options::MIN_FPS, "Target FPS must be at least " +
		std::to_string(Options::MIN_FPS) + ".");
	DebugAssert((resolutionScale >= Options::MIN_RESOLUTION_SCALE) &&
		(resolutionScale <= Options::MAX_RESOLUTION_SCALE), "Resolution scale must be between " +
		String::fixedPrecision(Options::MIN_RESOLUTION_SCALE, 2) + " and " +
		String::fixedPrecision(Options::MAX_RESOLUTION_SCALE, 2) + ".");
	DebugAssert((verticalFOV >= Options::MIN_VERTICAL_FOV) &&
		(verticalFOV <= Options::MAX_VERTICAL_FOV), "Field of view must be between " +
		String::fixedPrecision(Options::MIN_VERTICAL_FOV, 1) + " and " +
		String::fixedPrecision(Options::MAX_VERTICAL_FOV, 1) + ".");
	DebugAssert((letterboxAspect >= Options::MIN_LETTERBOX_ASPECT) &&
		(letterboxAspect <= Options::MAX_LETTERBOX_ASPECT),
		"Letterbox aspect must be between " +
		String::fixedPrecision(Options::MIN_LETTERBOX_ASPECT, 2) + " and " +
		String::fixedPrecision(Options::MAX_LETTERBOX_ASPECT, 2) + ".");
	DebugAssert((cursorScale >= Options::MIN_CURSOR_SCALE) &&
		(cursorScale <= Options::MAX_CURSOR_SCALE),
		"Cursor scale must be between " +
		String::fixedPrecision(Options::MIN_CURSOR_SCALE, 1) + " and " +
		String::fixedPrecision(Options::MAX_CURSOR_SCALE, 1) + ".");
	DebugAssert((hSensitivity >= Options::MIN_HORIZONTAL_SENSITIVITY) &&
		(hSensitivity <= Options::MAX_HORIZONTAL_SENSITIVITY),
		"Horizontal sensitivity must be between " +
		String::fixedPrecision(Options::MIN_HORIZONTAL_SENSITIVITY, 1) + " and " +
		String::fixedPrecision(Options::MAX_HORIZONTAL_SENSITIVITY, 1) + ".");
	DebugAssert((vSensitivity >= Options::MIN_VERTICAL_SENSITIVITY) &&
		(vSensitivity <= Options::MAX_VERTICAL_SENSITIVITY),
		"Vertical sensitivity must be between " +
		String::fixedPrecision(Options::MIN_VERTICAL_SENSITIVITY, 1) + " and " +
		String::fixedPrecision(Options::MAX_VERTICAL_SENSITIVITY, 1) + ".");
	DebugAssert((musicVolume >= 0.0) && (musicVolume <= 1.0),
		"Music volume must be between 0.0 and 1.0.");
	DebugAssert((soundVolume >= 0.0) && (soundVolume <= 1.0),
		"Sound volume must be between 0.0 and 1.0.");
	DebugAssert(soundChannels >= 1, "Must have at least one sound channel.");

	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;
	this->fullscreen = fullscreen;
	this->targetFPS = targetFPS;
	this->resolutionScale = resolutionScale;
	this->verticalFOV = verticalFOV;
	this->letterboxAspect = letterboxAspect;
	this->cursorScale = cursorScale;
	this->hSensitivity = hSensitivity;
	this->vSensitivity = vSensitivity;
	this->musicVolume = musicVolume;
	this->soundVolume = soundVolume;
	this->soundChannels = soundChannels;
	this->skipIntro = skipIntro;
	this->playerInterface = playerInterface;
	this->showDebug = showDebug;
}

Options::~Options()
{

}

int Options::getScreenWidth() const
{
	return this->screenWidth;
}

int Options::getScreenHeight() const
{
	return this->screenHeight;
}

bool Options::isFullscreen() const
{
	return this->fullscreen;
}

int Options::getTargetFPS() const
{
	return this->targetFPS;
}

double Options::getResolutionScale() const
{
	return this->resolutionScale;
}

double Options::getVerticalFOV() const
{
	return this->verticalFOV;
}

double Options::getLetterboxAspect() const
{
	return this->letterboxAspect;
}

double Options::getCursorScale() const
{
	return this->cursorScale;
}

double Options::getHorizontalSensitivity() const
{
	return this->hSensitivity;
}

double Options::getVerticalSensitivity() const
{
	return this->vSensitivity;
}

const std::string &Options::getSoundfont() const
{
	return this->soundfont;
}

double Options::getMusicVolume() const
{
	return this->musicVolume;
}

double Options::getSoundVolume() const
{
	return this->soundVolume;
}

int Options::getSoundChannelCount() const
{
	return this->soundChannels;
}

const std::string &Options::getArenaPath() const
{
	return this->arenaPath;
}

bool Options::introIsSkipped() const
{
	return this->skipIntro;
}

PlayerInterface Options::getPlayerInterface() const
{
	return this->playerInterface;
}

bool Options::debugIsShown() const
{
	return this->showDebug;
}

void Options::setScreenWidth(int width)
{
	assert(width > 0);

	this->screenWidth = width;
}

void Options::setScreenHeight(int height)
{
	assert(height > 0);

	this->screenHeight = height;
}

void Options::setFullscreen(bool fullscreen)
{
	this->fullscreen = fullscreen;
}

void Options::setTargetFPS(int targetFPS)
{
	assert(targetFPS >= Options::MIN_FPS);

	this->targetFPS = targetFPS;
}

void Options::setResolutionScale(double percent)
{
	assert(percent >= Options::MIN_RESOLUTION_SCALE);
	assert(percent <= Options::MAX_RESOLUTION_SCALE);

	this->resolutionScale = percent;
}

void Options::setVerticalFOV(double fov)
{
	assert(fov >= Options::MIN_VERTICAL_FOV);
	assert(fov <= Options::MAX_VERTICAL_FOV);

	this->verticalFOV = fov;
}

void Options::setLetterboxAspect(double aspect)
{
	this->letterboxAspect = aspect;
}

void Options::setCursorScale(double cursorScale)
{
	this->cursorScale = cursorScale;
}

void Options::setHorizontalSensitivity(double hSensitivity)
{
	this->hSensitivity = hSensitivity;
}

void Options::setVerticalSensitivity(double vSensitivity)
{
	this->vSensitivity = vSensitivity;
}

void Options::setSoundfont(std::string sfont)
{
	this->soundfont = std::move(sfont);
}

void Options::setMusicVolume(double percent)
{
	assert(percent >= 0.0);
	assert(percent <= 1.0);

	this->musicVolume = percent;
}

void Options::setSoundVolume(double percent)
{
	assert(percent >= 0.0);
	assert(percent <= 1.0);

	this->soundVolume = percent;
}

void Options::setSoundChannelCount(int count)
{
	this->soundChannels = count;
}

void Options::setArenaPath(std::string path)
{
	this->arenaPath = std::move(path);
}

void Options::setSkipIntro(bool skip)
{
	this->skipIntro = skip;
}

void Options::setPlayerInterface(PlayerInterface playerInterface)
{
	this->playerInterface = playerInterface;
}

void Options::setShowDebug(bool debug)
{
	this->showDebug = debug;
}
