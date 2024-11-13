#ifndef TEXT_CINEMATIC_PANEL_H
#define TEXT_CINEMATIC_PANEL_H

#include <functional>
#include <string>
#include <vector>

#include "Panel.h"
#include "TextCinematicUiModel.h"
#include "../Assets/TextureManager.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class Game;
class Renderer;

// Very similar to a cinematic panel, only now it's designed for cinematics with
// subtitles at the bottom (a.k.a., "text").
//
// If speech is used, a vector of doubles should be used for timing the text box 
// changes, because they depend on speech, which may or may not take the same time 
// each "block" of text.
//
// The text is split across frames by allowing a maximum of three newlines per
// paragraph. The text argument does not need any special formatting other than
// newlines built in as usual.
class TextCinematicPanel : public Panel
{
public:
	using OnFinishedFunction = std::function<void(Game&)>;
private:
	TextBox textBox;
	Button<Game&> skipButton;
	OnFinishedFunction onFinished;
	std::vector<std::string> textPages; // One string per page of text.
	std::vector<ScopedUiTextureRef> animTextureRefs; // One per animation image.
	TextCinematicUiModel::SpeechState speechState;
	double secondsPerImage, currentImageSeconds;
	int animImageIndex, textIndex, textCinematicDefIndex;

	void updateSubtitles();
public:
	TextCinematicPanel(Game &game);
	~TextCinematicPanel() override;

	bool init(int textCinematicDefIndex, double secondsPerImage, const OnFinishedFunction &onFinished);

	void tick(double dt) override;
};

#endif
