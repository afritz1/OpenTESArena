#ifndef PROVINCE_MAP_UI_MODEL_H
#define PROVINCE_MAP_UI_MODEL_H

#include <memory>
#include <string>

class Game;
class LocationDefinition;
class Panel;
class ProvinceDefinition;

namespace ProvinceMapUiModel
{
	// Shared between WorldMapPanel and ProvinceMapPanel for remembering the selected destination.
	struct TravelData
	{
		// @todo: change from 'ID' to 'index' to show it's not dependent on original game's 0-32 format.
		int locationID, provinceID, travelDays;

		TravelData(int locationID, int provinceID, int travelDays);
	};

	const std::string SearchButtonTooltip = "Search";
	const std::string TravelButtonTooltip = "Travel";
	const std::string BackToWorldMapButtonTooltip = "Back to World Map";

	std::string makeTravelText(Game &game, int srcProvinceIndex, const LocationDefinition &srcLocationDef,
		const ProvinceDefinition &srcProvinceDef, int dstLocationIndex, const TravelData &travelData);
	std::string makeAlreadyAtLocationText(Game &game, const std::string &locationName);

	std::string getLocationName(Game &game, int provinceID, int locationID);

	// Generates a text sub-panel with a parchment message.
	std::unique_ptr<Panel> makeTextPopUp(Game &game, const std::string &text);
}

#endif
