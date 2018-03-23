#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Assets/CIFFile.h"
#include "../Math/Vector2.h"
#include "../Media/Palette.h"

class Location;
class Renderer;
class Texture;
class TextureManager;

enum class ProvinceButtonName;

class ProvinceMapPanel : public Panel
{
private:
	Button<> searchButton, travelButton;
	Button<Game&> backToWorldMapButton;
	std::unique_ptr<CIFFile> staffDungeonCif; // For obtaining palette indices.
	Palette provinceMapPalette;
	int provinceID;

	// Gets the .IMG filename of the background image.
	std::string getBackgroundFilename() const;

	// Gets the location ID of the location closest to the mouse in 320x200 space.
	int getClosestLocationID(const Int2 &originalPosition) const;

	// Draws an icon (i.e., location or highlight) centered at the given point.
	void drawCenteredIcon(const Texture &texture, const Int2 &point, Renderer &renderer);

	// Draws the icons of all visible locations in the province.
	void drawVisibleLocations(const std::string &backgroundFilename,
		TextureManager &textureManager, Renderer &renderer);

	// Draws a highlight icon over the player's current location. This method should
	// only be called if the player's current location is in the current province.
	void drawCurrentLocationHighlight(const Location &location,
		const std::string &backgroundFilename, TextureManager &textureManager, Renderer &renderer);

	// Draws the name of a location in the current province. Intended for the location
	// closest to the mouse cursor.
	void drawLocationName(int locationID, Renderer &renderer);

	// Draws a tooltip for one of the interface buttons (search, travel, back to world map).
	void drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer);
public:
	ProvinceMapPanel(Game &game, int provinceID);
	virtual ~ProvinceMapPanel() = default;

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
	virtual void renderSecondary(Renderer &renderer) override;
};

#endif
