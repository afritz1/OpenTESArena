#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <unordered_map>
#include <vector>

#include "Panel.h"

// The original class list design in Arena is pretty bad. It's an alphabetical 
// list that says nothing about the classes (thus requiring the manual for 
// information). I think it's better to have tooltips.

class Button;
class CharacterClass;
class ListBox;
class Renderer;
class Surface;
class TextBox;
class Texture;

class ChooseClassPanel : public Panel
{
private:
	static const int MAX_TOOLTIP_LINE_LENGTH;

	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<ListBox> classesListBox;
	std::unique_ptr<Button> backToClassCreationButton, upButton, downButton, acceptButton;
	std::unordered_map<int, Texture> tooltipTextures;
	std::vector<std::unique_ptr<CharacterClass>> charClasses;
	std::unique_ptr<CharacterClass> charClass; // Chosen class for "accept" button.

	std::string getClassArmors(const CharacterClass &characterClass) const;
	std::string getClassShields(const CharacterClass &characterClass) const;
	std::string getClassWeapons(const CharacterClass &characterClass) const;

	void drawClassTooltip(int tooltipIndex, Renderer &renderer);
public:
	ChooseClassPanel(Game *game);
	virtual ~ChooseClassPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
