#ifndef IMAGE_SEQUENCE_PANEL_H
#define IMAGE_SEQUENCE_PANEL_H

#include <functional>
#include <string>
#include <vector>

#include "Panel.h"
#include "../UI/Button.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

// Halfway between a CinematicPanel and an ImagePanel, this panel displays still images one at a time and
// allows only the escape button to fully skip until the end, unlike the CinematicPanel. Mouse clicks, etc.
// will skip one image.

class ImageSequencePanel : public Panel
{
public:
	using OnFinishedFunction = std::function<void(Game&)>;
private:
	Button<Game&> skipButton;
	OnFinishedFunction onFinished;
	Buffer<ScopedUiTextureRef> textureRefs;
	Buffer<double> imageDurations;
	double currentSeconds;
	int imageIndex;
public:
	ImageSequencePanel(Game &game);
	~ImageSequencePanel() override = default;

	bool init(BufferView<const std::string> paletteNames, BufferView<const std::string> textureNames,
		BufferView<const double> imageDurations, const OnFinishedFunction &onFinished);

	virtual void tick(double dt) override;
};

#endif
