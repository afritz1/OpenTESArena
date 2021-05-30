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

class AudioManager;
class Options;
class Player;
class Renderer;
class TextBox;

enum class PlayerInterface;

class OptionsPanel : public Panel
{
private:
	// Options panel tabs.
	enum class Tab { Graphics, Audio, Input, Misc, Dev };

	std::unique_ptr<TextBox> titleTextBox, backToPauseMenuTextBox, graphicsTextBox, audioTextBox,
		inputTextBox, miscTextBox, devTextBox;
	Button<Game&> backToPauseMenuButton;
	Button<OptionsPanel&, OptionsPanel::Tab> tabButton;
	std::vector<std::unique_ptr<OptionsUiModel::Option>> graphicsOptions, audioOptions,
		inputOptions, miscOptions, devOptions;
	std::vector<std::unique_ptr<TextBox>> currentTabTextBoxes;
	OptionsPanel::Tab tab;

	// Gets the visible options group based on the current tab.
	std::vector<std::unique_ptr<OptionsUiModel::Option>> &getVisibleOptions();

	// Regenerates option text for one option.
	void updateOptionTextBox(int index);

	// Regenerates all option text boxes in the current tab.
	void updateVisibleOptionTextBoxes();

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
	virtual ~OptionsPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
