#ifndef PROVINCE_MAP_UI_CONTROLLER_H
#define PROVINCE_MAP_UI_CONTROLLER_H

#include "ProvinceMapUiModel.h"

class Game;
class ListBox;
class ProvinceMapPanel;
class ProvinceSearchSubPanel;

namespace ProvinceMapUiController
{
	void onSearchButtonSelected(Game &game, ProvinceMapPanel &panel, int provinceID);
	void onTravelButtonSelected(Game &game, ProvinceMapPanel &panel);
	void onBackToWorldMapButtonSelected(Game &game);

	void onTextPopUpSelected(Game &game);	
}

namespace ProvinceSearchUiController
{
	void onTextAccepted(Game &game, ProvinceSearchSubPanel &panel);
	void onListLocationSelected(Game &game, ProvinceSearchSubPanel &panel, int locationID);
	void onListUpButtonSelected(ListBox &listBox);
	void onListDownButtonSelected(ListBox &listBox);
}

#endif
