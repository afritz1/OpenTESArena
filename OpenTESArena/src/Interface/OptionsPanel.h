#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include <optional>
#include <vector>

#include "OptionsUiModel.h"
#include "Panel.h"
#include "../Math/Vector2.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class AudioManager;
class Options;
class Renderer;

enum class PlayerInterface;

struct Player;

class OptionsPanel : public Panel
{
private:
	TextBox descriptionTextBox, backButtonTextBox;
	std::vector<TextBox> tabTextBoxes;
	std::vector<TextBox> optionTextBoxes;
	Button<Game&> backButton;
	Button<OptionsPanel&, OptionsUiModel::Tab*, OptionsUiModel::Tab> tabButton;
	std::vector<OptionsUiModel::OptionGroup> optionGroups;
	OptionsUiModel::Tab tab;
	std::optional<int> hoveredOptionIndex;
	ScopedUiTextureRef backgroundTextureRef, tabButtonTextureRef, backButtonTextureRef,
		highlightTextureRef, cursorTextureRef;

	// Gets the visible options group based on the current tab.
	OptionsUiModel::OptionGroup &getVisibleOptions();

	std::optional<int> getHoveredOptionIndex() const;

	void updateOptionText(int index);
public:
	OptionsPanel(Game &game);
	~OptionsPanel() override = default;

	bool init();

	// Regenerates all options in the current tab (public for UiController function).
	void updateVisibleOptions();
};

#endif
