#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include <string>

#include "Panel.h"
#include "ProvinceMapUiModel.h"
#include "../Assets/CIFFile.h"
#include "../Math/Vector2.h"
#include "../Media/Palette.h"
#include "../UI/Button.h"

class LocationDefinition;
class ProvinceDefinition;
class Renderer;
class Texture;
class TextureManager;

class ProvinceMapPanel : public Panel
{
private:
	// Current is where the player is. Selected is which location (if any) has been selected.
	enum class LocationHighlightType { Current, Selected };

	Button<Game&, ProvinceMapPanel&, int> searchButton;
	Button<Game&, ProvinceMapPanel&, bool> travelButton;
	Button<Game&, std::unique_ptr<ProvinceMapUiModel::TravelData>> backToWorldMapButton;
	CIFFile staffDungeonCif; // For obtaining palette indices.
	std::unique_ptr<ProvinceMapUiModel::TravelData> travelData;
	Palette provinceMapPalette;
	double blinkTimer;
	int provinceID;

	// Gets the location ID of the location closest to the mouse in 320x200 space.
	int getClosestLocationID(const Int2 &originalPosition) const;

	// @todo: makeDiseasedWarningPopUp().
	// - Display when the player is diseased.

	// Draws an icon (i.e., location or highlight) centered at the given point.
	void drawCenteredIcon(const Texture &texture, const Int2 &point, Renderer &renderer);
	void drawCenteredIcon(TextureBuilderID textureBuilderID, PaletteID paletteID,
		const Int2 &point, Renderer &renderer);

	// Draws the icons of all visible locations in the province.
	void drawVisibleLocations(PaletteID backgroundPaletteID, TextureManager &textureManager, Renderer &renderer);

	// Draws a highlight icon over the given location. Useful for either the player's
	// current location or the currently selected location for fast travel.
	void drawLocationHighlight(const LocationDefinition &locationDef, LocationHighlightType highlightType,
		PaletteID highlightPaletteID, TextureManager &textureManager, Renderer &renderer);

	// Draws the name of a location in the current province. Intended for the location
	// closest to the mouse cursor.
	void drawLocationName(int locationID, Renderer &renderer);

	// Draws a tooltip for one of the interface buttons (search, travel, back to world map).
	void drawButtonTooltip(const std::string &text, Renderer &renderer);
public:
	ProvinceMapPanel(Game &game, int provinceID, std::unique_ptr<ProvinceMapUiModel::TravelData> travelData);
	virtual ~ProvinceMapPanel() = default;

	// Tries to set the given location ID as the selected one. If the player is already at
	// that location, then an error pop-up is displayed instead. This is a public method
	// so the province search sub-panel can call it, too.
	void trySelectLocation(int selectedLocationID);

	// Handles loading the target destination into the current game session and changing to the game world panel.
	// Public for UI controller.
	void handleFastTravel();

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
	virtual void renderSecondary(Renderer &renderer) override;
};

#endif
