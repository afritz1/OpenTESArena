#include <cassert>

#include "Options.h"

#include "../Media/MusicFormat.h"
#include "../Media/SoundFormat.h"

Options::Options(int screenWidth, int screenHeight, bool fullscreen, double verticalFOV, 
	double hSensitivity, double vSensitivity, double musicVolume, double soundVolume,
	int soundChannels, MusicFormat musicFormat, SoundFormat soundFormat, bool skipIntro)
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

const int &Options::getScreenWidth() const
{
	return this->screenWidth;
}

const int &Options::getScreenHeight() const
{
	return this->screenHeight;
}

const bool &Options::isFullscreen() const
{
	return this->fullscreen;
}

const double &Options::getVerticalFOV() const
{
	return this->verticalFOV;
}

const double &Options::getHorizontalSensitivity() const
{
	return this->hSensitivity;
}

const double &Options::getVerticalSensitivity() const
{
	return this->vSensitivity;
}

const double &Options::getMusicVolume() const
{
	return this->musicVolume;
}

const double &Options::getSoundVolume() const
{
	return this->soundVolume;
}

const int &Options::getSoundChannelCount() const
{
	return this->soundChannels;
}

const MusicFormat &Options::getMusicFormat() const
{
	return this->musicFormat;
}

const SoundFormat &Options::getSoundFormat() const
{
	return this->soundFormat;
}

const bool &Options::introIsSkipped() const
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
