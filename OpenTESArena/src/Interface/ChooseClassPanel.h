#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <unordered_map>
#include <vector>

#include "Button.h"
#include "ListBox.h"
#include "Panel.h"
#include "../Entities/CharacterClass.h"
#include "../Rendering/Texture.h"

// The original class list design in Arena is pretty bad. It's an alphabetical 
// list that says nothing about the classes (thus requiring the manual for 
// information). I think it's better to have tooltips.

class Renderer;
class Surface;
class TextBox;

class ChooseClassPanel : public Panel
{
private:
	static const int MAX_TOOLTIP_LINE_LENGTH;

	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<ListBox> classesListBox;
	Button<Game&> backToClassCreationButton;
	Button<ChooseClassPanel&> upButton, downButton;
	Button<Game&, const CharacterClass&> acceptButton;
	std::unordered_map<int, Texture> tooltipTextures;
	std::vector<CharacterClass> charClasses;

	std::string getClassArmors(const CharacterClass &characterClass) const;
	std::string getClassShields(const CharacterClass &characterClass) const;
	std::string getClassWeapons(const CharacterClass &characterClass) const;

	void drawClassTooltip(int tooltipIndex, Renderer &renderer);
public:
	ChooseClassPanel(Game &game);
	virtual ~ChooseClassPanel() = default;

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
