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
	static const std::string PATH;
	static const std::string FILENAME;

	double hSensitivity, vSensitivity;
	double verticalFOV; // In degrees.
	int screenWidth, screenHeight, soundChannels;
	MusicFormat musicFormat;
	SoundFormat soundFormat;
	bool fullscreen;
public:
	// Initialize values from the options file.
	Options();
	~Options();

	const double &getHorizontalSensitivity() const;
	const double &getVerticalSensitivity() const;
	const double &getVerticalFOV() const;
	const int &getScreenWidth() const;
	const int &getScreenHeight() const;
	const int &getSoundChannelCount() const;
	const MusicFormat &getMusicFormat() const;
	const SoundFormat &getSoundFormat() const;
	const bool &isFullscreen() const;

	void setHorizontalSensitivity(double hSensitivity);
	void setVerticalSensitivity(double vSensitivity);
	void setVerticalFOV(double fov);
	void setScreenWidth(int width);
	void setScreenHeight(int height);
	void setSoundChannelCount(int count);
	void setMusicFormat(MusicFormat musicFormat);
	void setSoundFormat(SoundFormat soundFormat);
	void setFullscreen(bool fullscreen);

	void saveToFile();
};

#endif
