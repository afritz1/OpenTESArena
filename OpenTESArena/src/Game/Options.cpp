#include <cassert>

#include "Options.h"

#include "../Media/MusicFormat.h"
#include "../Media/SoundFormat.h"

Options::Options(std::string&& dataPath, int screenWidth, int screenHeight, bool fullscreen,
    double verticalFOV, double hSensitivity, double vSensitivity, double musicVolume,
    double soundVolume, int soundChannels, MusicFormat musicFormat, SoundFormat soundFormat,
    bool skipIntro)
    : dataPath(std::move(dataPath))
{
	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;
	this->fullscreen = fullscreen;
	this->verticalFOV = verticalFOV;
	this->hSensitivity = hSensitivity;
	this->vSensitivity = vSensitivity;
	this->musicVolume = musicVolume;
	this->soundVolume = soundVolume;
	this->soundChannels = soundChannels;
	this->musicFormat = musicFormat;
	this->soundFormat = soundFormat;
	this->skipIntro = skipIntro;

	assert(this->screenWidth > 0);
	assert(this->screenHeight > 0);
	assert(this->verticalFOV > 0.0);
	assert(this->verticalFOV < 180.0);
	assert(this->hSensitivity > 0.0);
	assert(this->vSensitivity > 0.0);
	assert(this->musicVolume >= 0.0);
	assert(this->musicVolume <= 1.0);
	assert(this->soundVolume >= 0.0);
	assert(this->soundVolume <= 1.0);
	assert(this->soundChannels >= 1);
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

double Options::getVerticalFOV() const
{
	return this->verticalFOV;
}

double Options::getHorizontalSensitivity() const
{
	return this->hSensitivity;
}

double Options::getVerticalSensitivity() const
{
	return this->vSensitivity;
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

MusicFormat Options::getMusicFormat() const
{
	return this->musicFormat;
}

SoundFormat Options::getSoundFormat() const
{
	return this->soundFormat;
}

bool Options::introIsSkipped() const
{
	return this->skipIntro;
}

void Options::setDataPath(const std::string &path)
{
    assert(!path.empty());

    this->dataPath = path;
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

void Options::setVerticalFOV(double fov)
{
	assert(fov > 0.0);
	assert(fov < 180.0);

	this->verticalFOV = fov;
}

void Options::setHorizontalSensitivity(double hSensitivity)
{
	this->hSensitivity = hSensitivity;
}

void Options::setVerticalSensitivity(double vSensitivity)
{
	this->vSensitivity = vSensitivity;
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

void Options::setMusicFormat(MusicFormat musicFormat)
{
	this->musicFormat = musicFormat;
}

void Options::setSoundFormat(SoundFormat soundFormat)
{
	this->soundFormat = soundFormat;
}

void Options::setSkipIntro(bool skip)
{
	this->skipIntro = skip;
}
