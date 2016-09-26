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
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ImageSequencePanel(GameState *gameState,
		const std::vector<std::string> &paletteNames,
		const std::vector<std::string> &textureNames,
		const std::vector<double> &imageDurations,
		const std::function<void(GameState*)> &endingAction);
	virtual ~ImageSequencePanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
