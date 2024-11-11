#ifndef IMAGE_PANEL_H
#define IMAGE_PANEL_H

#include <functional>
#include <string>

#include "Panel.h"
#include "../UI/Button.h"

class Game;
class Renderer;

// For rendering still images in a similar fashion to a cinematic, only now it's one image.
class ImagePanel : public Panel
{
public:
	using OnFinishedFunction = std::function<void(Game&)>;
private:
	Button<Game&> skipButton;
	ScopedUiTextureRef textureRef;
	double secondsToDisplay, currentSeconds;
public:
	ImagePanel(Game &game);
	~ImagePanel() override = default;

	bool init(const std::string &paletteName, const std::string &textureName, double secondsToDisplay,
		const OnFinishedFunction &onFinished);

	virtual void tick(double dt) override;
};

#endif
