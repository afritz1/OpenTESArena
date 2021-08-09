#ifndef PROVINCE_MAP_UI_MODEL_H
#define PROVINCE_MAP_UI_MODEL_H

#include <memory>
#include <string>
#include <vector>

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
		const ProvinceDefinition &srcProvinceDef, int dstLocationIndex);
	std::string makeAlreadyAtLocationText(Game &game, const std::string &locationName);

	std::string getLocationName(Game &game, int provinceID, int locationID);

	// Generates a text sub-panel with a parchment message.
	std::unique_ptr<Panel> makeTextPopUp(Game &game, const std::string &text);
}

namespace ProvinceSearchUiModel
{
	enum class Mode { TextEntry, List };

	constexpr int MaxNameLength = 20;

	bool isCharAllowed(char c);

	std::string getTitleText(Game &game);

	// Returns a list of all visible location indices in the given province that have a match with
	// the given location name. Technically, this should only return up to one index, but returning
	// a list allows functionality for approximate matches. The exact location index points into
	// the vector if there is an exact match, or null otherwise.
	std::vector<int> getMatchingLocations(Game &game, const std::string &locationName,
		int provinceIndex, const int **exactLocationIndex);
}

#endif
