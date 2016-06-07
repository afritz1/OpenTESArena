#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <vector>

#include "Panel.h"

// The original class list design in Arena is pretty bad. It's an alphabetical 
// list that says nothing about the classes (thus requiring the manual for 
// information). I think it would be better to show them all at the same time as 
// buttons, and have tooltips when hovered over.

// I think having a hardcoded number of classes isn't good design now that the
// character classes are being put into a text file to be parsed at runtime.
// Maybe a list would be best, but make sure to have tooltips!

class Button;
class CharacterClass;
class ListBox;
class Surface;
class TextBox;

enum class CharacterGenderName;

class ChooseClassPanel : public Panel
{
private:
	static const int MAX_TOOLTIP_LINE_LENGTH;

	std::unique_ptr<Surface> parchment, upDownSurface;
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<ListBox> classesListBox;
	std::unique_ptr<Button> backToGenderButton, upButton, downButton, acceptButton;
	std::unique_ptr<CharacterGenderName> gender;
	std::vector<std::unique_ptr<CharacterClass>> charClasses;
	std::unique_ptr<CharacterClass> charClass; // Chosen class for "accept" button.

	std::string getClassArmors(const CharacterClass &characterClass) const;
	std::string getClassShields(const CharacterClass &characterClass) const;
	std::string getClassWeapons(const CharacterClass &characterClass) const;
	void drawClassTooltip(const CharacterClass &characterClass, SDL_Surface *dst);
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
