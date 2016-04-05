#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include "Panel.h"

// Splitting up character creation panels would be better for being able to go 
// back and change something. Let's do that.

class Button;
class Surface;
class TextBox;

class ChooseGenderPanel : public Panel
{
private:
	std::unique_ptr<Surface> parchment;
	std::unique_ptr<TextBox> genderTextBox, maleTextBox, femaleTextBox;
	std::unique_ptr<Button> backToMainMenuButton, maleButton, femaleButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseGenderPanel(GameState *gameState);
	virtual ~ChooseGenderPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
