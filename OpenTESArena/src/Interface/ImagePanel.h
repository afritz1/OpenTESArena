#ifndef IMAGE_PANEL_H
#define IMAGE_PANEL_H

#include <functional>

#include "Panel.h"
#include "../Media/TextureName.h"

// For rendering still images in a similar fashion to a cinematic, only now
// it's one image.

class Button;

class ImagePanel : public Panel
{
private:
	std::unique_ptr<Button> skipButton;
	TextureName textureName;
	double secondsToDisplay, currentSeconds;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ImagePanel(GameState *gameState, TextureName textureName, double secondsToDisplay,
		const std::function<void()> &endingAction);
	virtual ~ImagePanel();
	
	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif