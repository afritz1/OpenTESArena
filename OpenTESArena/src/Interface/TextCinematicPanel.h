#ifndef TEXT_CINEMATIC_PANEL_H
#define TEXT_CINEMATIC_PANEL_H

#include "Panel.h"

class TextCinematicPanel : public Panel
{
public:
	TextCinematicPanel(Game &game);
	~TextCinematicPanel() override;

	bool init();
};

#endif
