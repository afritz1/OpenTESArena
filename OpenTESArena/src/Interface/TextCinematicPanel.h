#ifndef TEXT_CINEMATIC_PANEL_H
#define TEXT_CINEMATIC_PANEL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Button.h"
#include "Panel.h"

// Very similar to a cinematic panel, only now it's designed for cinematics with
// subtitles at the bottom (a.k.a., "text").

// If speech is used, a vector of doubles should be used for timing the text box 
// changes, because they depend on speech, which may or may not take the same time 
// each "block" of text.

// The text is split across frames by allowing a maximum of three newlines per
// paragraph. The text argument does not need any special formatting other than
// newlines built in as usual.

class Game;
class Renderer;
class TextBox;

class TextCinematicPanel : public Panel
{
private:
	std::vector<std::unique_ptr<TextBox>> textBoxes; // One for every three new lines.
	std::unique_ptr<Button<Game&>> skipButton;
	std::string sequenceName;
	double secondsPerImage, currentImageSeconds;
	int imageIndex, textIndex;
public:
	TextCinematicPanel(Game &game, const std::string &sequenceName,
		const std::string &text, double secondsPerImage,
		const std::function<void(Game&)> &endingAction);
	virtual ~TextCinematicPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
