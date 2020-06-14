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

class ExeData;
class Rect;
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

	std::string getClassArmors(const CharacterClass &charClass) const;
	std::string getClassShields(const CharacterClass &charClass) const;
	std::string getClassWeapons(const CharacterClass &charClass) const;

	// Gets the rectangle for the class list's area.
	static Rect getClassListRect(const ExeData &exeData);

	void drawClassTooltip(int tooltipIndex, Renderer &renderer);
public:
	ChooseClassPanel(Game &game);
	virtual ~ChooseClassPanel() = default;

	virtual Panel::CursorData getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
