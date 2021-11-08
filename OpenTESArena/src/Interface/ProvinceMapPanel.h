#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include <string>

#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Media/Palette.h"
#include "../UI/AnimationState.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class LocationDefinition;
class ProvinceDefinition;
class Renderer;
class Texture;
class TextureManager;

class ProvinceMapPanel : public Panel
{
private:
	struct LocationTextureRefGroup
	{
		ScopedUiTextureRef textureRef, playerCurrentTextureRef, travelDestinationTextureRef;

		void init(UiTextureID textureID, UiTextureID playerCurrentTextureID, UiTextureID travelDestinationTextureID, Renderer &renderer);
	};

	TextBox hoveredLocationTextBox;
	Button<Game&, ProvinceMapPanel&, int> searchButton;
	Button<Game&, ProvinceMapPanel&> travelButton;
	Button<Game&> backToWorldMapButton;
	LocationTextureRefGroup cityStateTextureRefs, townTextureRefs, villageTextureRefs, dungeonTextureRefs, staffDungeonTextureRefs;
	ScopedUiTextureRef backgroundTextureRef, cursorTextureRef;
	AnimationState blinkState;
	int provinceID;
	int hoveredLocationID;

	void initLocationIconUI(int provinceID);

	// Updates the ID of the location closest to the mouse in 320x200 space.
	void updateHoveredLocationID(const Int2 &originalPosition);

	// @todo: makeDiseasedWarningPopUp().
	// - Display when the player is diseased.
public:
	ProvinceMapPanel(Game &game);
	~ProvinceMapPanel() override = default;

	bool init(int provinceID);

	// Tries to set the given location ID as the selected one. If the player is already at
	// that location, then an error pop-up is displayed instead. This is a public method
	// so the province search sub-panel can call it, too.
	// @todo: the province sub-panel listbox elements should all have std::functions instead
	void trySelectLocation(int selectedLocationID);

	// Handles loading the target destination into the current game session and changing to the game world panel.
	// Public for UI controller.
	void handleFastTravel();

	void onPauseChanged(bool paused) override;
	void tick(double dt) override;
};

#endif
