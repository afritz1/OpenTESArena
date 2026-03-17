#ifndef CHARACTER_PANEL_H
#define CHARACTER_PANEL_H

#include "Panel.h"

class CharacterPanel : public Panel
{
public:
	CharacterPanel(Game &game);
	~CharacterPanel() override;

	bool init();
};

#endif
