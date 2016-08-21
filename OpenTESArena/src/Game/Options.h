#ifndef OPTIONS_H
#define OPTIONS_H

#include <cstdint>
#include <string>

// Settings found in the options menu are saved in this object, which should live in
// the game state object since it persists for the lifetime of the program.

class Options
{
private:
	// Graphics.
	int32_t screenWidth, screenHeight;
	bool fullscreen;
	double renderQuality; // Percent.
	double verticalFOV; // In degrees.
	double letterboxAspect;
	double cursorScale;

	// Input.
	double hSensitivity, vSensitivity;

    // Sound.
    std::string soundfont; // .cfg file.
    double musicVolume, soundVolume;
    int32_t soundChannels;

	// Miscellaneous.
	std::string arenaPath; // "ARENA" data path.
	bool skipIntro;
public:
	Options(std::string &&arenaPath, int32_t screenWidth, int32_t screenHeight, bool fullscreen,
        double renderQuality, double verticalFOV, double zetterboxAspect, double cursorScale, 
		double hSensitivity, double vSensitivity, std::string &&soundfont, double musicVolume, 
		double soundVolume, int32_t soundChannels, bool skipIntro);
	~Options();

	int32_t getScreenWidth() const;
	int32_t getScreenHeight() const;
	bool isFullscreen() const;
	double getRenderQuality() const;
	double getVerticalFOV() const;
	double getLetterboxAspect() const;
	double getCursorScale() const;
	double getHorizontalSensitivity() const;
	double getVerticalSensitivity() const;
	const std::string &getSoundfont() const;
	double getMusicVolume() const;
	double getSoundVolume() const;
	int32_t getSoundChannelCount() const;
	const std::string &getArenaPath() const;
	bool introIsSkipped() const;

	void setScreenWidth(int32_t width);
	void setScreenHeight(int32_t height);
	void setFullscreen(bool fullscreen);
	void setRenderQuality(double percent);
	void setVerticalFOV(double fov);
	void setLetterboxAspect(double aspect);
	void setCursorScale(double cursorScale);
	void setHorizontalSensitivity(double hSensitivity);
	void setVerticalSensitivity(double vSensitivity);
    void setSoundfont(std::string sfont);
	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
	void setSoundChannelCount(int32_t count);
	void setArenaPath(std::string path);
	void setSkipIntro(bool skip);
};

#endif
