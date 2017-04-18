#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include "Button.h"
#include "Panel.h"

// Arena doesn't have an options menu, so I made one up!

class Options;
class Renderer;
class TextBox;

enum class PlayerInterface;

class OptionsPanel : public Panel
{
private:
	static const std::string FPS_TEXT;
	static const std::string RESOLUTION_SCALE_TEXT;
	static const std::string PLAYER_INTERFACE_TEXT;
	static const std::string VERTICAL_FOV_TEXT;

	std::unique_ptr<TextBox> titleTextBox, backToPauseTextBox, fpsTextBox, 
		resolutionScaleTextBox, playerInterfaceTextBox, verticalFOVTextBox;
	std::unique_ptr<Button<Game*>> backToPauseButton;
	std::unique_ptr<Button<OptionsPanel*, Options&>> fpsUpButton, fpsDownButton,
		verticalFOVUpButton, verticalFOVDownButton;
	std::unique_ptr<Button<OptionsPanel*, Options&, Renderer&>> resolutionScaleUpButton, 
		resolutionScaleDownButton, playerInterfaceButton;

	static std::string getPlayerInterfaceString(PlayerInterface playerInterface);

	void updateFPSText(int fps);
	void updateResolutionScaleText(double resolutionScale);
	void updatePlayerInterfaceText(PlayerInterface playerInterface);
	void updateVerticalFOVText(double verticalFOV);
public:
	OptionsPanel(Game *game);
	virtual ~OptionsPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
