#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>

// Settings found in the options menu are saved in this object, which should live in
// the game state object since it persists for the lifetime of the program.

class Options
{
private:
    // Root data path
    std::string dataPath;

	// Graphics.
	int screenWidth, screenHeight;
	bool fullscreen;
	double verticalFOV; // In degrees.

	// Input.
	double hSensitivity, vSensitivity;

	// Sound.
	double musicVolume, soundVolume;
	int soundChannels;

	// Miscellaneous.
	bool skipIntro;
public:
	Options(std::string&& dataPath, int screenWidth, int screenHeight, bool fullscreen,
        double verticalFOV, double hSensitivity, double vSensitivity, double musicVolume,
		double soundVolume, int soundChannels, bool skipIntro);
	~Options();

    const std::string &getDataPath() const { return dataPath; };
	int getScreenWidth() const;
	int getScreenHeight() const;
	bool isFullscreen() const;
	double getVerticalFOV() const;
	double getHorizontalSensitivity() const;
	double getVerticalSensitivity() const;
	double getMusicVolume() const;
	double getSoundVolume() const;
	int getSoundChannelCount() const;
	bool introIsSkipped() const;

    void setDataPath(const std::string &path);
	void setScreenWidth(int width);
	void setScreenHeight(int height);
	void setFullscreen(bool fullscreen);
	void setVerticalFOV(double fov);
	void setHorizontalSensitivity(double hSensitivity);
	void setVerticalSensitivity(double vSensitivity);
	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
	void setSoundChannelCount(int count);
	void setSkipIntro(bool skip);
};

#endif
