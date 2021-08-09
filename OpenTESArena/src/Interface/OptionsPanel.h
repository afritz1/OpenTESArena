#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "OptionsUiModel.h"
#include "Panel.h"
#include "../Math/Vector2.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class AudioManager;
class Options;
class Player;
class Renderer;

enum class PlayerInterface;

class OptionsPanel : public Panel
{
private:
	TextBox titleTextBox, backToPauseMenuTextBox, graphicsTextBox, audioTextBox, inputTextBox, miscTextBox, devTextBox;
	Button<Game&> backToPauseMenuButton;
	Button<OptionsPanel&, OptionsUiModel::Tab*, OptionsUiModel::Tab> tabButton;
	std::vector<std::unique_ptr<OptionsUiModel::Option>> graphicsOptions, audioOptions,
		inputOptions, miscOptions, devOptions;
	std::vector<TextBox> currentTabTextBoxes;
	OptionsUiModel::Tab tab;

	// Gets the visible options group based on the current tab.
	std::vector<std::unique_ptr<OptionsUiModel::Option>> &getVisibleOptions();

	// Initializes an option text box.
	void initOptionTextBox(int index);

	// Regenerates option text for one option.
	void updateOptionTextBoxText(int index);

	// Draws return buttons and tabs.
	void drawReturnButtonsAndTabs(Renderer &renderer);

	// Draws the text of the buttons.
	void drawText(Renderer &renderer);

	// Draw each option's text.
	void drawTextOfOptions(Renderer &renderer);

	// Draws description for an option. Not using mouse tooltips because they
	// get in the way of seeing what an option's value is.
	void drawDescription(const std::string &text, Renderer &renderer);
public:
	OptionsPanel(Game &game);
	~OptionsPanel() override = default;

	bool init();

	// Regenerates all options in the current tab (public for UiController function).
	void updateVisibleOptions();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void render(Renderer &renderer) override;
};

#endif
