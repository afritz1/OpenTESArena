#include <cassert>

#include "Options.h"

#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

const int Options::MIN_FPS = 15;
const double Options::MIN_RESOLUTION_SCALE = 0.10;
const double Options::MAX_RESOLUTION_SCALE = 1.0;

Options::Options(std::string &&dataPath, int screenWidth, int screenHeight, bool fullscreen,
	int targetFPS, double resolutionScale, double verticalFOV, double letterboxAspect,
	double cursorScale, double hSensitivity, double vSensitivity, std::string &&soundfont,
	double musicVolume, double soundVolume, int soundChannels, bool skipIntro)
	: arenaPath(std::move(dataPath)), soundfont(std::move(soundfont))
{
	// Make sure each of the values is in a valid range.
	Debug::check(screenWidth > 0, "Options", "Screen width must be positive.");
	Debug::check(screenHeight > 0, "Options", "Screen height must be positive.");
	Debug::check(targetFPS >= Options::MIN_FPS, "Options", "Target FPS must be at least " +
		std::to_string(Options::MIN_FPS) + ".");
	Debug::check((resolutionScale >= Options::MIN_RESOLUTION_SCALE) && 
		(resolutionScale <= Options::MAX_RESOLUTION_SCALE), "Options",
		"Resolution scale must be between " + 
		String::fixedPrecision(Options::MIN_RESOLUTION_SCALE, 2) + " and " + 
		String::fixedPrecision(Options::MAX_RESOLUTION_SCALE, 2) + ".");
	Debug::check((verticalFOV > 0.0) && (verticalFOV < 180.0), "Options",
		"Field of view must be between 0.0 and 180.0 exclusive.");
	Debug::check(letterboxAspect > 0.0, "Options", "Letterbox aspect must be positive.");
	Debug::check(cursorScale > 0.0, "Options", "Cursor scale must be positive.");
	Debug::check(hSensitivity > 0.0, "Options", "Horizontal sensitivity must be positive.");
	Debug::check(vSensitivity > 0.0, "Options", "Vertical sensitivity must be positive.");
	Debug::check((musicVolume >= 0.0) && (musicVolume <= 1.0), "Options",
		"Music volume must be between 0.0 and 1.0.");
	Debug::check((soundVolume >= 0.0) && (soundVolume <= 1.0), "Options",
		"Sound volume must be between 0.0 and 1.0.");
	Debug::check(soundChannels >= 1, "Options", "Must have at least one sound channel.");

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
	assert(fov > 0.0);
	assert(fov < 180.0);

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
