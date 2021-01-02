#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <unordered_map>
#include <vector>

#include "Button.h"
#include "ListBox.h"
#include "Panel.h"
#include "Texture.h"
#include "../Entities/CharacterClassDefinition.h"

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
	static constexpr int MAX_TOOLTIP_LINE_LENGTH = 14;

	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<ListBox> classesListBox;
	Button<Game&> backToClassCreationButton;
	Button<ChooseClassPanel&> upButton, downButton;
	Button<Game&, int> acceptButton;
	std::unordered_map<int, Texture> tooltipTextures;
	std::vector<CharacterClassDefinition> charClasses;

	std::string getClassArmors(const CharacterClassDefinition &charClassDef) const;
	std::string getClassShields(const CharacterClassDefinition &charClassDef) const;
	std::string getClassWeapons(const CharacterClassDefinition &charClassDef) const;

	// Gets the rectangle for the class list's area.
	static Rect getClassListRect(const ExeData &exeData);

	void drawClassTooltip(int tooltipIndex, Renderer &renderer);
public:
	ChooseClassPanel(Game &game);
	virtual ~ChooseClassPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
