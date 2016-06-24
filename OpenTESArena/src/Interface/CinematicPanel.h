#ifndef CINEMATIC_PANEL_H
#define CINEMATIC_PANEL_H

#include <functional>

#include "Panel.h"

// Designed for sets of images (i.e., videos) that play one after another and
// eventually lead to another panel. Skipping is available, too.

class Button;

enum class TextureSequenceName;

class CinematicPanel : public Panel
{
private:
	std::unique_ptr<Button> skipButton;
	TextureSequenceName sequenceName;
	double secondsPerImage, currentSeconds;
	int imageIndex;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	CinematicPanel(GameState *gameState, TextureSequenceName name, 
		double secondsPerImage, const std::function<void()> &endingAction);
	virtual ~CinematicPanel();

	static const double DEFAULT_MOVIE_SECONDS_PER_IMAGE;

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Renderer *renderer, const SDL_Rect *letterbox) override;
};

#endif
