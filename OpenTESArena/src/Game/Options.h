#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>

// Settings found in the options menu are saved in this object, which should live in
// the game state object since it persists for the lifetime of the program.

enum class MusicFormat;
enum class SoundFormat;

class Options
{
private:
	// Graphics.
	int screenWidth, screenHeight;
	bool fullscreen;
	double verticalFOV; // In degrees.

	// Input.
	double hSensitivity, vSensitivity;

	// Sound.
	double musicVolume, soundVolume;
	int soundChannels;
	MusicFormat musicFormat;
	SoundFormat soundFormat;

	// Miscellaneous.
	bool skipIntro;
public:
	Options(int screenWidth, int screenHeight, bool fullscreen, double verticalFOV,
		double hSensitivity, double vSensitivity, double musicVolume,
		double soundVolume, int soundChannels, MusicFormat musicFormat, 
		SoundFormat soundFormat, bool skipIntro);
	~Options();

	const int &getScreenWidth() const;
	const int &getScreenHeight() const;
	const bool &isFullscreen() const;
	const double &getVerticalFOV() const;
	const double &getHorizontalSensitivity() const;
	const double &getVerticalSensitivity() const;
	const double &getMusicVolume() const;
	const double &getSoundVolume() const;
	const int &getSoundChannelCount() const;
	const MusicFormat &getMusicFormat() const;
	const SoundFormat &getSoundFormat() const;
	const bool &introIsSkipped() const;

	void setScreenWidth(int width);
	void setScreenHeight(int height);
	void setFullscreen(bool fullscreen);
	void setVerticalFOV(double fov);
	void setHorizontalSensitivity(double hSensitivity);
	void setVerticalSensitivity(double vSensitivity);
	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
	void setSoundChannelCount(int count);
	void setMusicFormat(MusicFormat musicFormat);
	void setSoundFormat(SoundFormat soundFormat);
	void setSkipIntro(bool skip);
};

#endif
