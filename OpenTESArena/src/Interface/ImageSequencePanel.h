#ifndef IMAGE_SEQUENCE_PANEL_H
#define IMAGE_SEQUENCE_PANEL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Panel.h"

// Halfway between a CinematicPanel and an ImagePanel, this panel displays still 
// images one at a time and allows only the escape button to fully skip until the
// end, unlike the CinematicPanel. Mouse clicks, etc. will skip one image.

class Button;

class ImageSequencePanel : public Panel
{
private:
	std::unique_ptr<Button> skipButton;
	std::vector<std::string> paletteNames;
	std::vector<std::string> textureNames;
	std::vector<double> imageDurations;
	double currentSeconds;
	int imageIndex;
public:
	ImageSequencePanel(Game *game,
		const std::vector<std::string> &paletteNames,
		const std::vector<std::string> &textureNames,
		const std::vector<double> &imageDurations,
		const std::function<void(Game*)> &endingAction);
	virtual ~ImageSequencePanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
