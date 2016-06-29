#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <map>
#include <vector>

#include "Panel.h"

// The original class list design in Arena is pretty bad. It's an alphabetical 
// list that says nothing about the classes (thus requiring the manual for 
// information). I think it would be better to have tooltips.

class Button;
class CharacterClass;
class ListBox;
class Surface;
class TextBox;

struct SDL_Texture;

class ChooseClassPanel : public Panel
{
private:
	static const int MAX_TOOLTIP_LINE_LENGTH;

	std::unique_ptr<Surface> parchment;
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<ListBox> classesListBox;
	std::unique_ptr<Button> backToClassCreationButton, upButton, downButton, acceptButton;
	std::map<int, SDL_Texture*> tooltipTextures;
	std::vector<std::unique_ptr<CharacterClass>> charClasses;
	std::unique_ptr<CharacterClass> charClass; // Chosen class for "accept" button.

	std::string getClassArmors(const CharacterClass &characterClass) const;
	std::string getClassShields(const CharacterClass &characterClass) const;
	std::string getClassWeapons(const CharacterClass &characterClass) const;

	void createTooltip(int tooltipIndex, SDL_Renderer *renderer);
	void drawClassTooltip(int tooltipIndex, SDL_Renderer *renderer);
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseClassPanel(GameState *gameState);
	virtual ~ChooseClassPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Renderer *renderer, const SDL_Rect *letterbox) override;
};

#endif
