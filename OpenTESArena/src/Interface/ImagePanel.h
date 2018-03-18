#ifndef IMAGE_PANEL_H
#define IMAGE_PANEL_H

#include <functional>
#include <string>

#include "Button.h"
#include "Panel.h"

// For rendering still images in a similar fashion to a cinematic, only now
// it's one image.

class Game;
class Renderer;

class ImagePanel : public Panel
{
private:
	Button<Game&> skipButton;
	std::string paletteName;
	std::string textureName;
	double secondsToDisplay, currentSeconds;
public:
	ImagePanel(Game &game, const std::string &paletteName, 
		const std::string &textureName, double secondsToDisplay, 
		const std::function<void(Game&)> &endingAction);
	virtual ~ImagePanel() = default;

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
