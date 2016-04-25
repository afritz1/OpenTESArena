#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <map>

#include "Panel.h"

// The original class list design in Arena is pretty bad. It's an alphabetical 
// list that says nothing about the classes (thus requiring the manual for 
// information). I think it would be better to show them all at the same time as 
// buttons, and have tooltips when hovered over.

// Replace the current three buttons with three columns holding six mini-buttons 
// each, like this:
//           [Choose thy class]
//   [Warrior]      [Mage]       [Thief]
//      w1            m1           t1
//     ...           ...          ...
//     ...           ...          ...
//     ...           ...          ...
//     ...           ...          ...
//     ...           ...          ...

class Button;
class Surface;
class TextBox;

enum class CharacterGenderName;

class ChooseClassPanel : public Panel
{
private:
	// Eventually a std::map<ClassName, Button> instead of individual class buttons, so 
	// then "for each class name, if class name's button is clicked, do..." can be used.
	// This helps avoid having 18 "if" branches.
	// std::map<CharacterClassName, std::unique_ptr<Button>> classButtons;

	std::unique_ptr<Surface> parchment;
	std::unique_ptr<TextBox> classTextBox, warriorTextBox, mageTextBox, thiefTextBox;
	std::unique_ptr<Button> backToGenderButton, warriorButton, mageButton, thiefButton;
	std::unique_ptr<CharacterGenderName> gender;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseClassPanel(GameState *gameState, CharacterGenderName gender);
	virtual ~ChooseClassPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
