#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include <optional>
#include <vector>

#include "Panel.h"
#include "../Stats/CharacterClassDefinition.h"
#include "../UI/Button.h"
#include "../UI/ListBox.h"
#include "../UI/TextBox.h"

class ExeData;
class Rect;
class Renderer;
class Surface;

// The original class list design in Arena is pretty bad. It's an alphabetical list that says nothing
// about the classes (thus requiring the manual for information). I think it's better to have tooltips.
class ChooseClassPanel : public Panel
{
private:
	TextBox titleTextBox, classDescriptionTextBox;
	ListBox classesListBox;
	Button<ListBox&> upButton, downButton;
	std::vector<CharacterClassDefinition> charClasses;
	std::optional<int> hoveredClassIndex;
	ScopedUiTextureRef nightSkyTextureRef, popUpTextureRef, cursorTextureRef;
public:
	ChooseClassPanel(Game &game);
	~ChooseClassPanel() override = default;

	bool init();
};

#endif
