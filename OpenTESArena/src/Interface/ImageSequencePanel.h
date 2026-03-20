#ifndef IMAGE_SEQUENCE_PANEL_H
#define IMAGE_SEQUENCE_PANEL_H

#include "Panel.h"

// Halfway between a CinematicPanel and an ImagePanel, this panel displays still images one at a time and
// allows only the escape button to fully skip until the end, unlike the CinematicPanel. Mouse clicks, etc.
// will skip one image.
class ImageSequencePanel : public Panel
{
public:
	ImageSequencePanel(Game &game);
	~ImageSequencePanel() override;

	bool init();
};

#endif
