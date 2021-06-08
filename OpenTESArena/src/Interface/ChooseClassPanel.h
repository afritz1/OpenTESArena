#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <unordered_map>
#include <vector>

#include "Panel.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../UI/Button.h"
#include "../UI/ListBox.h"
#include "../UI/Texture.h"

// The original class list design in Arena is pretty bad. It's an alphabetical 
// list that says nothing about the classes (thus requiring the manual for 
// information). I think it's better to have tooltips.

class ExeData;
class Rect;
class Renderer;
class Surface;
class TextBox;

class ChooseClassPanel : public Panel
{
private:
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<ListBox> classesListBox;
	Button<Game&> backToClassCreationButton;
	Button<ListBox&> upButton, downButton;
	Button<Game&, int> acceptButton;
	std::unordered_map<int, Texture> tooltipTextures;
	std::vector<CharacterClassDefinition> charClasses;

	void drawClassTooltip(int tooltipIndex, Renderer &renderer);
public:
	ChooseClassPanel(Game &game);
	virtual ~ChooseClassPanel() = default;

	bool init();

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
