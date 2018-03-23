#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include "Button.h"
#include "Panel.h"
#include "../Assets/CIFFile.h"
#include "../Media/Palette.h"

class Renderer;

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

	void drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer);

	// Draws the name of a location in the province. Intended for the location
	// closest to the mouse cursor.
	void drawLocationName(const std::string &name, const Int2 &center,
		Renderer &renderer);
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
