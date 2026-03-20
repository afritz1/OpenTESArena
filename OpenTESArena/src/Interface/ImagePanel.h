#ifndef IMAGE_PANEL_H
#define IMAGE_PANEL_H

#include "Panel.h"

class Game;

class ImagePanel : public Panel
{
public:
	ImagePanel(Game &game);
	~ImagePanel() override;

	bool init();
};

#endif
