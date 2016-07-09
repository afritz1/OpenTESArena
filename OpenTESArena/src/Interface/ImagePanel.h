#ifndef IMAGE_PANEL_H
#define IMAGE_PANEL_H

#include <functional>

#include "Panel.h"

// For rendering still images in a similar fashion to a cinematic, only now
// it's one image.

class Button;
class GameState;
class Renderer;

enum class PaletteName;
enum class TextureName;

class ImagePanel : public Panel
{
private:
	std::unique_ptr<Button> skipButton;
	PaletteName paletteName;
	TextureName textureName;
	double secondsToDisplay, currentSeconds;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ImagePanel(GameState *gameState, PaletteName paletteName, TextureName textureName, 
		double secondsToDisplay, const std::function<void(GameState*)> &endingAction);
	virtual ~ImagePanel();
	
	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
