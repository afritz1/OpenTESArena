#ifndef TEXT_CINEMATIC_PANEL_H
#define TEXT_CINEMATIC_PANEL_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Panel.h"
#include "TextCinematicUiModel.h"
#include "../Media/TextureManager.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

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

class TextCinematicPanel : public Panel
{
public:
	using OnFinishedFunction = std::function<void(Game&)>;
private:
	std::vector<TextBox> textBoxes; // One for every three new lines.
	Button<Game&> skipButton;
	std::string animTextureFilename;
	TextCinematicUiModel::SpeechState speechState;
	double secondsPerImage, currentImageSeconds;
	int animImageIndex, textIndex, textCinematicDefIndex;
public:
	TextCinematicPanel(Game &game);
	~TextCinematicPanel() override;

	bool init(int textCinematicDefIndex, double secondsPerImage, const OnFinishedFunction &onFinished);

	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
