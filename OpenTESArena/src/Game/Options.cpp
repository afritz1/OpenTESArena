#include <cassert>
#include <map>

#include "Options.h"

#include "../Media/MusicFormat.h"
#include "../Media/SoundFormat.h"

const auto OptionsMusicFormats = std::map<std::string, MusicFormat>
{
	{ "MIDI", MusicFormat::MIDI },
	{ "MP3", MusicFormat::MP3 },
	{ "Ogg", MusicFormat::Ogg }
};

// Technically, "WAV" is "wave", but oh well.
const auto OptionsSoundFormats = std::map<std::string, SoundFormat>
{
	{ "Ogg", SoundFormat::Ogg },
	{ "WAV", SoundFormat::WAV }
};

const std::string Options::PATH = "options/";
const std::string Options::FILENAME = "options.txt";

Options::Options()
{
	// These values will be loaded from an "options.txt" file eventually.
	this->hSensitivity = 800.0;
	this->vSensitivity = 800.0;
	this->verticalFOV = 90.0;
	this->screenWidth = 1280;
	this->screenHeight = 720;
	this->soundChannels = 32;
	this->musicFormat = MusicFormat::MIDI;
	this->soundFormat = SoundFormat::Ogg;
	this->fullscreen = false;
}

Options::~Options()
{

}

const double &Options::getHorizontalSensitivity() const
{
	return this->hSensitivity;
}

const double &Options::getVerticalSensitivity() const
{
	return this->vSensitivity;
}

const double &Options::getVerticalFOV() const
{
	return this->verticalFOV;
}

const int &Options::getScreenWidth() const
{
	return this->screenWidth;
}

const int &Options::getScreenHeight() const
{
	return this->screenHeight;
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

const bool &Options::isFullscreen() const
{
	return this->fullscreen;
}

void Options::setHorizontalSensitivity(double hSensitivity)
{
	this->hSensitivity = hSensitivity;
}

void Options::setVerticalSensitivity(double vSensitivity)
{
	this->vSensitivity = vSensitivity;
}

void Options::setVerticalFOV(double fov)
{
	assert(fov > 0.0);
	assert(fov < 180.0);

	this->verticalFOV = fov;
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

void Options::setFullscreen(bool fullscreen)
{
	this->fullscreen = fullscreen;
}

void Options::saveToFile()
{
	// The options menu should have an "Apply" button.
	auto fullPath = Options::PATH + Options::FILENAME;

	// ...not done!
}
