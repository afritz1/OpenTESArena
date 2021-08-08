#ifndef CINEMATIC_PANEL_H
#define CINEMATIC_PANEL_H

#include <functional>
#include <string>

#include "Panel.h"
#include "../Assets/TextureAssetReference.h"

// Designed for sets of images (i.e., videos) that play one after another and
// eventually lead to another panel. Skipping is available, too.

class Game;
class Renderer;

class CinematicPanel : public Panel
{
public:
	using OnFinishedFunction = std::function<void(Game&)>;
private:
	Button<Game&> skipButton;
	TextureAssetReference paletteTextureAssetRef;
	std::string sequenceFilename;
	double secondsPerImage, currentSeconds;
	int imageIndex;

	TextureAssetReference getCurrentSequenceTextureAssetRef();
public:
	CinematicPanel(Game &game);
	~CinematicPanel() override = default;

	bool init(const std::string &paletteName, const std::string &sequenceName, double secondsPerImage,
		const OnFinishedFunction &onFinished);

	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
