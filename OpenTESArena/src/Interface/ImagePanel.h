#ifndef IMAGE_PANEL_H
#define IMAGE_PANEL_H

#include <functional>
#include <string>

#include "Panel.h"

// For rendering still images in a similar fashion to a cinematic, only now
// it's one image.

class Button;
class GameState;
class Renderer;

class ImagePanel : public Panel
{
private:
	std::unique_ptr<Button> skipButton;
	std::string paletteName;
	std::string textureName;
	double secondsToDisplay, currentSeconds;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ImagePanel(GameState *gameState, const std::string &paletteName, 
		const std::string &textureName, double secondsToDisplay, 
		const std::function<void(GameState*)> &endingAction);
	virtual ~ImagePanel();
	
	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
