#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include "Button.h"
#include "Panel.h"

// Arena doesn't have an options menu, so I made one up!

class Options;
class Player;
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
	static const std::string CURSOR_SCALE_TEXT;
	static const std::string LETTERBOX_ASPECT_TEXT;
	static const std::string HORIZONTAL_SENSITIVITY_TEXT;
	static const std::string VERTICAL_SENSITIVITY_TEXT;
	static const std::string COLLISION_TEXT;
	static const std::string SKIP_INTRO_TEXT;
	static const std::string FULLSCREEN_TEXT;

	std::unique_ptr<TextBox> titleTextBox, backToPauseTextBox, fpsTextBox,
		resolutionScaleTextBox, playerInterfaceTextBox, verticalFOVTextBox,
		cursorScaleTextBox, letterboxAspectTextBox, hSensitivityTextBox, vSensitivityTextBox,
		collisionTextBox, skipIntroTextBox, fullscreenTextBox;
	Button<Game&> backToPauseButton;
	Button<OptionsPanel&, Options&> fpsUpButton, fpsDownButton,
		verticalFOVUpButton, verticalFOVDownButton, cursorScaleUpButton, cursorScaleDownButton,
		hSensitivityUpButton, hSensitivityDownButton, vSensitivityUpButton, vSensitivityDownButton,
		collisionButton, skipIntroButton;
	Button<OptionsPanel&, Options&, Renderer&> resolutionScaleUpButton,
		resolutionScaleDownButton, letterboxAspectUpButton, letterboxAspectDownButton,
		fullscreenButton;
	Button<OptionsPanel&, Options&, Player&, Renderer&> playerInterfaceButton;

	static std::string getPlayerInterfaceString(bool modernInterface);

	void updateFPSText(int fps);
	void updateResolutionScaleText(double resolutionScale);
	void updatePlayerInterfaceText(PlayerInterface playerInterface);
	void updateVerticalFOVText(double verticalFOV);
	void updateCursorScaleText(double cursorScale);
	void updateLetterboxAspectText(double letterboxAspect);
	void updateHorizontalSensitivityText(double hSensitivity);
	void updateVerticalSensitivityText(double vSensitivity);
	void updateCollisionText(bool collision);
	void updateSkipIntroText(bool skip);
	void updateFullscreenText(bool fullscreen);

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	OptionsPanel(Game &game);
	virtual ~OptionsPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
