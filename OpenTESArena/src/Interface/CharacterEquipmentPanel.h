#ifndef CHARACTER_EQUIPMENT_PANEL_H
#define CHARACTER_EQUIPMENT_PANEL_H

#include "Panel.h"

class CharacterEquipmentPanel : public Panel
{
public:
	CharacterEquipmentPanel(Game &game);
	~CharacterEquipmentPanel() override;

	bool init();
};

#endif
