#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Assets/CIFFile.h"
#include "../Math/Vector2.h"
#include "../Media/Palette.h"

class LocationDefinition;
class ProvinceDefinition;
class Renderer;
class Texture;
class TextureManager;

enum class ProvinceButtonName;

class ProvinceMapPanel : public Panel
{
public:
	// Shared between WorldMapPanel and ProvinceMapPanel for remembering the
	// selected destination.
	struct TravelData
	{
		// @todo: change from 'ID' to 'index' to show it's not dependent on original game's 0-32 format.
		int locationID, provinceID, travelDays;

		TravelData(int locationID, int provinceID, int travelDays);
	};
private:
	// Current is where the player is. Selected is which location (if any) has been selected.
	enum class LocationHighlightType { Current, Selected };

	static constexpr double BLINK_PERIOD = 1.0 / 5.0; // Duration of blink period in seconds.
	static constexpr double BLINK_PERIOD_PERCENT_ON = 0.75; // Percentage of each period spent "on".

	Button<Game&, ProvinceMapPanel&> searchButton, travelButton;
	Button<Game&, std::unique_ptr<ProvinceMapPanel::TravelData>> backToWorldMapButton;
	CIFFile staffDungeonCif; // For obtaining palette indices.
	std::unique_ptr<TravelData> travelData;
	Palette provinceMapPalette;
	double blinkTimer;
	int provinceID;

	// Gets the .IMG filename of the background image.
	std::string getBackgroundFilename() const;

	// Gets the location ID of the location closest to the mouse in 320x200 space.
	int getClosestLocationID(const Int2 &originalPosition) const;

	// Generates the pop-up text for traveling to a given destination.
	std::string makeTravelText(const LocationDefinition &srcLocationDef,
		const ProvinceDefinition &srcProvinceDef, int dstLocationIndex,
		const ProvinceMapPanel::TravelData &travelData) const;

	// Generates a text sub-panel with a parchment message.
	std::unique_ptr<Panel> makeTextPopUp(const std::string &text) const;

	// @todo: makeDiseasedWarningPopUp().
	// - Display when the player is diseased.

	// Handles loading the target destination into the current game session
	// and changing to the game world panel.
	void handleFastTravel();

	// Draws an icon (i.e., location or highlight) centered at the given point.
	void drawCenteredIcon(const Texture &texture, const Int2 &point, Renderer &renderer);
	void drawCenteredIcon(TextureBuilderID textureBuilderID, PaletteID paletteID,
		const Int2 &point, Renderer &renderer);

	// Draws the icons of all visible locations in the province.
	void drawVisibleLocations(const std::string &backgroundFilename,
		TextureManager &textureManager, Renderer &renderer);

	// Draws a highlight icon over the given location. Useful for either the player's
	// current location or the currently selected location for fast travel.
	void drawLocationHighlight(const LocationDefinition &locationDef,
		LocationHighlightType highlightType, const std::string &backgroundFilename,
		TextureManager &textureManager, Renderer &renderer);

	// Draws the name of a location in the current province. Intended for the location
	// closest to the mouse cursor.
	void drawLocationName(int locationID, Renderer &renderer);

	// Draws a tooltip for one of the interface buttons (search, travel, back to world map).
	void drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer);
public:
	ProvinceMapPanel(Game &game, int provinceID,
		std::unique_ptr<ProvinceMapPanel::TravelData> travelData);
	virtual ~ProvinceMapPanel() = default;

	// Tries to set the given location ID as the selected one. If the player is already at
	// that location, then an error pop-up is displayed instead. This is a public method
	// so the province search sub-panel can call it, too.
	void trySelectLocation(int selectedLocationID);

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
	virtual void renderSecondary(Renderer &renderer) override;
};

#endif
