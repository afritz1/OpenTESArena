#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include "Button.h"
#include "Panel.h"

// Arena doesn't have an options menu, so I made one up!

class Options;
class Renderer;
class TextBox;

class OptionsPanel : public Panel
{
private:
	static const std::string FPS_TEXT;
	static const std::string RESOLUTION_SCALE_TEXT;

	std::unique_ptr<TextBox> titleTextBox, backToPauseTextBox, fpsTextBox, 
		resolutionScaleTextBox;
	std::unique_ptr<Button<Game*>> backToPauseButton;
	std::unique_ptr<Button<OptionsPanel*, Options&>> fpsUpButton, fpsDownButton;
	std::unique_ptr<Button<OptionsPanel*, Options&, Renderer&>> resolutionScaleUpButton, 
		resolutionScaleDownButton;

	void updateFPSText(int fps);
	void updateResolutionScaleText(double resolutionScale);
public:
	OptionsPanel(Game *game);
	virtual ~OptionsPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
