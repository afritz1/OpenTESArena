#ifndef CINEMATIC_PANEL_H
#define CINEMATIC_PANEL_H

#include <functional>

#include "Panel.h"

// Designed for sets of images (i.e., videos) that play one after another and
// eventually lead to another panel. Skipping is available, too.

class Button;
class GameState;
class Renderer;

enum class PaletteName;
enum class TextureSequenceName;

class CinematicPanel : public Panel
{
private:
	std::unique_ptr<Button> skipButton;
	PaletteName paletteName;
	TextureSequenceName sequenceName;
	double secondsPerImage, currentSeconds;
	int32_t imageIndex;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	CinematicPanel(GameState *gameState, PaletteName paletteName, 
		TextureSequenceName name, double secondsPerImage, 
		const std::function<void(GameState*)> &endingAction);
	virtual ~CinematicPanel();

	static const double DEFAULT_MOVIE_SECONDS_PER_IMAGE;

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
