#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <unordered_map>
#include <vector>

#include "Panel.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../UI/Button.h"
#include "../UI/ListBox.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

// The original class list design in Arena is pretty bad. It's an alphabetical 
// list that says nothing about the classes (thus requiring the manual for 
// information). I think it's better to have tooltips.

class ExeData;
class Rect;
class Renderer;
class Surface;

class ChooseClassPanel : public Panel
{
private:
	TextBox titleTextBox;
	ListBox classesListBox;
	Button<ListBox&> upButton, downButton;
	std::unordered_map<int, Texture> tooltipTextures;
	std::vector<CharacterClassDefinition> charClasses;

	void drawClassTooltip(int tooltipIndex, Renderer &renderer);
public:
	ChooseClassPanel(Game &game);
	~ChooseClassPanel() override = default;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void render(Renderer &renderer) override;
};

#endif
